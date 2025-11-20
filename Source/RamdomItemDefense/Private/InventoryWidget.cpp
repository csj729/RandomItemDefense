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
	if (InventoryComp.IsValid())
	{
		InventoryComp->OnInventoryUpdated.RemoveDynamic(this, &UInventoryWidget::HandleInventoryUpdated);
	}
}

void UInventoryWidget::BindDataSources()
{
	// 1. 인벤토리 컴포넌트 찾기
	if (!InventoryComp.IsValid())
	{
		if (ARamdomItemDefenseCharacter* Character = GetOwningPlayerPawn<ARamdomItemDefenseCharacter>())
		{
			InventoryComp = Character->GetInventoryComponent();
			if (InventoryComp.IsValid())
			{
				// 델리게이트 중복 방지 체크 후 연결
				if (!InventoryComp->OnInventoryUpdated.IsAlreadyBound(this, &UInventoryWidget::HandleInventoryUpdated))
				{
					InventoryComp->OnInventoryUpdated.AddDynamic(this, &UInventoryWidget::HandleInventoryUpdated);
				}

				// 연결 성공 로그
				UE_LOG(LogRamdomItemDefense, Log, TEXT("InventoryWidget: InventoryComponent Bound Successfully!"));

				// 연결 즉시 초기화면 갱신 (이미 들어있는 아이템 표시)
				HandleInventoryUpdated();
			}
		}
	}

	// 2. [핵심] 인벤토리 컴포넌트를 아직 못 찾았다면 다음 프레임에 다시 시도 (무한 재시도)
	if (!InventoryComp.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UInventoryWidget::BindDataSources);
	}
}

void UInventoryWidget::HandleInventoryUpdated()
{
	// BP 이벤트 실행 (아이템 슬롯 갱신)
	OnInventoryUpdated();
}