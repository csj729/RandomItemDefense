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

void UCommonItemChoiceWidget::BindDataSources()
{
	// 1. PlayerState 포인터 확보 시도
	if (!MyPlayerState)
	{
		MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	}

	// 2. [핵심 수정] PlayerState가 유효하면 항상 바인딩 체크 (포인터가 있어도 바인딩이 풀려있을 수 있음)
	if (MyPlayerState)
	{
		if (!MyPlayerState->OnCommonItemChoiceCountChangedDelegate.IsAlreadyBound(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged))
		{
			MyPlayerState->OnCommonItemChoiceCountChangedDelegate.AddDynamic(this, &UCommonItemChoiceWidget::HandleCommonItemChoiceCountChanged);
		}
	}

	// 3. InventoryComponent 확보 시도
	if (!InventoryComp.IsValid())
	{
		if (ARamdomItemDefenseCharacter* Character = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>())
		{
			InventoryComp = Character->GetInventoryComponent();
		}
	}

	// 4. 하나라도 준비되지 않았다면 다음 틱에 재시도
	if (!MyPlayerState || !InventoryComp.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UCommonItemChoiceWidget::BindDataSources);
	}
	else
	{
		// (Delegate 바인딩 직후 초기값을 화면에 표시하기 위함)
		HandleCommonItemChoiceCountChanged(MyPlayerState->GetCommonItemChoiceCount());
	}
}

void UCommonItemChoiceWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// 위젯이 제거될 때 델리게이트 해제
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

	// 횟수가 있으면 목록 갱신, 없으면 숨김 요청
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
		// BindDataSources의 재시도 로직 덕분에 여기 도달할 확률은 낮음
		return;
	}

	// 1. 모든 흔함 아이템 ID 가져오기
	TArray<FName> ItemIDs = InventoryComp->GetAllCommonItemIDs();

	if (ItemIDs.Num() == 0)
	{
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

	// 3. BP에 UI 표시 요청 (아이템 데이터 배열 전달)
	OnShowChoices(ItemChoices);
}

void UCommonItemChoiceWidget::MakeChoice(FName ChosenItemID)
{
	if (MyPlayerState)
	{
		MyPlayerState->Server_UseCommonItemChoice(ChosenItemID);
	}
}