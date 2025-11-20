#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "RamdomItemDefenseCharacter.h"
#include "RamdomItemDefense.h" // 로그용

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// 위젯이 켜지면 바인딩 시도 시작
	BindDataSources();
}

void UInventoryWidget::NativeDestruct()
{
	Super::NativeDestruct();
	// 위젯이 꺼질 때 안전하게 연결 해제
	if (InventoryComp)
	{
		InventoryComp->OnInventoryUpdated.RemoveDynamic(this, &UInventoryWidget::HandleInventoryUpdated);
	}
}

void UInventoryWidget::BindDataSources()
{
    // 1. 인벤토리 컴포넌트 참조가 없다면 찾기 시도
    if (!InventoryComp)
    {
        if (ARamdomItemDefenseCharacter* Character = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>())
        {
            InventoryComp = Character->GetInventoryComponent();
        }
    }

    // 2. 인벤토리 컴포넌트가 유효하다면 (새로 찾았든, 예전부터 알고 있었든)
    if (InventoryComp)
    {
        // "이미 알고 있다"고 넘어가는 게 아니라, "연결이 안 되어 있으면 다시 연결"합니다.
        if (!InventoryComp->OnInventoryUpdated.IsAlreadyBound(this, &UInventoryWidget::HandleInventoryUpdated))
        {
            InventoryComp->OnInventoryUpdated.AddDynamic(this, &UInventoryWidget::HandleInventoryUpdated);

            // 연결 직후 UI를 강제로 한 번 갱신해 줍니다. (창을 켤 때 최신 상태 반영)
            HandleInventoryUpdated();

            UE_LOG(LogRamdomItemDefense, Log, TEXT("InventoryWidget: Delegate Re-Bound Successfully!"));
        }
    }
    // 3. 아직도 못 찾았다면 다음 틱에 재시도
    else
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UInventoryWidget::BindDataSources);
    }
}

void UInventoryWidget::HandleInventoryUpdated()
{
	// BP 이벤트 실행 (아이템 슬롯 갱신)
	OnInventoryUpdated();
}