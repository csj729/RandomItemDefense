// Source/RamdomItemDefense/Private/CommonItemChoiceWidget.cpp (수정)

#include "CommonItemChoiceWidget.h"
#include "Components/TextBlock.h"
#include "MyPlayerState.h"
#include "RamdomItemDefenseCharacter.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h"

/** 위젯이 뷰포트에 추가될 때 (델리게이트 바인딩) */
void UCommonItemChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// PlayerState 참조 가져오기
	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();

	// InventoryComponent 참조 가져오기 (캐릭터로부터)
	if (ARamdomItemDefenseCharacter* Character = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>())
	{
		InventoryComp = Character->GetInventoryComponent();
	}

	// PlayerState 유효성 검사 및 '델리게이트' 바인딩
	if (MyPlayerState)
	{
		// [중요] '흔함 아이템 선택권' 델리게이트에 바인딩
		MyPlayerState->OnCommonItemChoiceCountChangedDelegate.AddDynamic(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged);
		// 현재 값으로 텍스트 즉시 업데이트
		HandleCommonItemChoiceCountChanged(MyPlayerState->GetCommonItemChoiceCount());
	}
	else
	{
		// PlayerState가 아직 준비되지 않았다면 다음 틱에 다시 시도
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
			if (MyPlayerState)
			{
				MyPlayerState->OnCommonItemChoiceCountChangedDelegate.AddDynamic(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged);
				HandleCommonItemChoiceCountChanged(MyPlayerState->GetCommonItemChoiceCount());
			}
			if (ARamdomItemDefenseCharacter* Character = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>())
			{
				InventoryComp = Character->GetInventoryComponent();
			}
			});
	}
}

/** 위젯이 뷰포트에서 제거될 때 (델리게이트 해제) */
void UCommonItemChoiceWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// 델리게이트 바인딩 해제
	if (MyPlayerState)
	{
		MyPlayerState->OnCommonItemChoiceCountChangedDelegate.RemoveDynamic(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged);
	}
}

/** PlayerState의 '흔함 아이템 선택권' 횟수가 변경될 때 호출됩니다. */
void UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged(int32 NewCount)
{
	if (ChoiceCountText)
	{
		// 텍스트 포맷 설정 (예: "남은 선택 횟수: 1")
		FFormatNamedArguments Args;
		Args.Add(TEXT("Count"), FText::AsNumber(NewCount));
		ChoiceCountText->SetText(FText::Format(FText::FromString(TEXT("남은 선택 횟수: {Count}")), Args));
	}

	if (NewCount > 0)
	{
		// 선택권이 1개 이상 생겼으므로, 아이템 목록을 새로고침하여 BP에 표시 요청
		PopulateChoices();
	}
	else
	{
		// 선택권이 0개가 되었으므로, BP에게 아이템 슬롯 숨김 요청
		OnHideChoices();
	}
}

/**
 * @brief (C++ 내부용) 선택권이 생겼을 때, BP에게 표시할 아이템 목록을 요청합니다.
 */
void UCommonItemChoiceWidget::PopulateChoices()
{
	// 인벤토리 컴포넌트가 유효한지 확인
	if (!InventoryComp.IsValid())
	{
		RID_LOG(FColor::Red, TEXT("CommonItemChoiceWidget: InventoryComp is invalid!"));
		return;
	}

	// 1. 인벤토리 컴포넌트로부터 '모든' 흔함 아이템 ID를 가져옵니다.
	TArray<FName> ItemIDs = InventoryComp->GetAllCommonItemIDs();

	if (ItemIDs.Num() == 0)
	{
		RID_LOG(FColor::Yellow, TEXT("CommonItemChoiceWidget: No common items found to populate choices."));
		return;
	}

	// 2. ID 배열을 실제 FItemData 배열로 변환합니다. (UI에 아이콘/이름 표시용)
	TArray<FItemData> ItemChoices;
	for (const FName& ID : ItemIDs)
	{
		bool bSuccess = false;
		FItemData Data = InventoryComp->GetItemData(ID, bSuccess);
		if (bSuccess)
		{
			ItemChoices.Add(Data);
		}
	}

	// 3. 블루프린트(WBP)의 OnShowChoices 이벤트를 호출하여 UI를 채우도록 합니다.
	OnShowChoices(ItemChoices);
}


/**
 * @brief (블루프린트 호출용) WBP의 아이템 버튼 클릭 시 이 함수를 호출해야 합니다.
 */
void UCommonItemChoiceWidget::MakeChoice(FName ChosenItemID)
{
	if (MyPlayerState)
	{
		// 1. 서버에 이 아이템을 선택했다고 알림 (선택권 1개 소모)
		MyPlayerState->Server_UseCommonItemChoice(ChosenItemID);

		// --- [ ★★★ 수정 ★★★ ] ---
		// 2. [클라이언트 즉시 반응]
		// OnHideChoices(); // <--- 이 라인을 삭제하거나 주석 처리합니다.
		// --- [ ★★★ 수정 끝 ★★★ ] ---
	}
}