// Source/RamdomItemDefense/Private/CommonItemChoiceWidget.cpp (수정)

#include "CommonItemChoiceWidget.h"
#include "Components/TextBlock.h"
#include "MyPlayerState.h"
#include "RamdomItemDefenseCharacter.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h"

void UCommonItemChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindDataSources();
}

// [수정] 재귀적 바인딩 함수
void UCommonItemChoiceWidget::BindDataSources()
{
	// 1. PlayerState 바인딩 시도
	if (!MyPlayerState)
	{
		MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
		if (MyPlayerState)
		{
			if (!MyPlayerState->OnCommonItemChoiceCountChangedDelegate.IsAlreadyBound(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged))
			{
				MyPlayerState->OnCommonItemChoiceCountChangedDelegate.AddDynamic(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged);
			}
			// [1차 시도] 텍스트 갱신 (만약 인벤토리가 아직 없으면 슬롯 생성은 실패함)
			HandleCommonItemChoiceCountChanged(MyPlayerState->GetCommonItemChoiceCount());
		}
	}

	// 2. InventoryComponent 바인딩 시도
	if (!InventoryComp.IsValid())
	{
		if (ARamdomItemDefenseCharacter* Character = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>())
		{
			InventoryComp = Character->GetInventoryComponent();
		}
	}

	// 3. 하나라도 준비되지 않았다면 다음 틱에 재시도
	if (!MyPlayerState || !InventoryComp.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UCommonItemChoiceWidget::BindDataSources);
	}
	else
	{
		UE_LOG(LogRamdomItemDefense, Log, TEXT("CommonItemChoiceWidget: All Data Sources Bound Successfully!"));

		// 모든 데이터 소스가 연결된 시점에 UI를 강제로 다시 갱신합니다.
		// (1차 시도에서 InventoryComp가 없어 슬롯 생성이 실패했을 경우를 복구)
		if (MyPlayerState)
		{
			HandleCommonItemChoiceCountChanged(MyPlayerState->GetCommonItemChoiceCount());
		}
	}
}

void UCommonItemChoiceWidget::NativeDestruct()
{
	Super::NativeDestruct();

	if (MyPlayerState)
	{
		MyPlayerState->OnCommonItemChoiceCountChangedDelegate.RemoveDynamic(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged);
	}
}

void UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged(int32 NewCount)
{
	if (ChoiceCountText)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Count"), FText::AsNumber(NewCount));
		ChoiceCountText->SetText(FText::Format(FText::FromString(TEXT("남은 선택 횟수: {Count}")), Args));
	}

	if (NewCount > 0)
	{
		PopulateChoices();
	}
	else
	{
		OnHideChoices();
	}
}

void UCommonItemChoiceWidget::PopulateChoices()
{
	// 인벤토리 컴포넌트 유효성 체크
	if (!InventoryComp.IsValid())
	{
		// 이 로그가 뜨면 바인딩 순서 문제임 (위의 수정으로 해결됨)
		RID_LOG(FColor::Red, TEXT("CommonItemChoiceWidget: InventoryComp is invalid!"));
		return;
	}

	// 1. 모든 흔함 아이템 ID 가져오기
	TArray<FName> ItemIDs = InventoryComp->GetAllCommonItemIDs();

	if (ItemIDs.Num() == 0)
	{
		RID_LOG(FColor::Yellow, TEXT("CommonItemChoiceWidget: No common items found in DataTable."));
		return;
	}

	// 2. FItemData로 변환
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

	// 3. BP에 UI 표시 요청
	OnShowChoices(ItemChoices);
}

void UCommonItemChoiceWidget::MakeChoice(FName ChosenItemID)
{
	if (MyPlayerState)
	{
		MyPlayerState->Server_UseCommonItemChoice(ChosenItemID);
	}
}