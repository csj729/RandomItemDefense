#include "MainHUDWidget.h"
#include "MyPlayerState.h"
#include "RamdomItemDefensePlayerController.h"
#include "MonsterSpawner.h"
#include "Kismet/GameplayStatics.h"

bool UMainHUDWidget::Initialize()
{
	bool bSuccess = Super::Initialize();
	if (!bSuccess) return false;

	// 이 위젯을 소유한 플레이어의 PlayerState와 Controller를 가져옵니다.
	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	MyPlayerController = GetOwningPlayer<ARamdomItemDefensePlayerController>();

	// PlayerState가 유효하다면 이벤트를 바인딩합니다.
	if (MyPlayerState)
	{
		BindPlayerStateEvents();
	}
	else
	{
		// 만약 PlayerState가 아직 준비되지 않았다면, 잠시 후 다시 시도합니다.
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UMainHUDWidget::BindPlayerStateEvents);
	}

	if (MyPlayerState && MyPlayerState->MySpawner)
	{
		BindSpawnerEvents();
	}

	return true;
}

void UMainHUDWidget::BindPlayerStateEvents()
{
	// PlayerState가 여전히 없는지 다시 확인합니다 (예: 재시도)
	if (MyPlayerState == nullptr)
	{
		MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	}

	if (MyPlayerState)
	{
		// PlayerState의 C++ 델리게이트에 이 위젯의 UFUNCTION을 바인딩합니다.
		MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UMainHUDWidget::OnGoldChanged);
		MyPlayerState->OnSpawnerAssignedDelegate.AddDynamic(this, &UMainHUDWidget::HandleSpawnerAssigned);

		// 바인딩 직후, 현재 값으로 UI를 즉시 업데이트하도록 이벤트를 호출합니다.
		OnGoldChanged(MyPlayerState->GetGold());

		if (MyPlayerState->MySpawner)
		{
			BindSpawnerEvents();
		}
	}
}

void UMainHUDWidget::BindSpawnerEvents()
{
	if (MyPlayerState && MyPlayerState->MySpawner)
	{
		// 스포너의 몬스터 수 변경 델리게이트에 C++ 함수를 연결
		MyPlayerState->MySpawner->OnMonsterCountChangedDelegate.AddDynamic(this, &UMainHUDWidget::HandleMonsterCountChanged);

		// 현재 값으로 즉시 업데이트
		HandleMonsterCountChanged(MyPlayerState->MySpawner->GetCurrentMonsterCount());
	}
}

// --- 버튼 클릭 핸들러 ---
void UMainHUDWidget::HandleStatUpgradeClicked()
{
	 if (MyPlayerController)
	 {
	 	// (추후 구현) 컨트롤러의 스탯창 토글 함수 호출
	  MyPlayerController->ToggleStatUpgradeWidget();
	 }
}

void UMainHUDWidget::HandleInventoryClicked()
{
	 if (MyPlayerController)
	 {
	 	// (추후 구현) 컨트롤러의 인벤토리 토글 함수 호출
	  MyPlayerController->ToggleInventoryWidget();
	 }
}

void UMainHUDWidget::HandleSpawnerAssigned(int32 Dummy)
{
	// 스포너 델리게이트에 바인딩합니다.
	BindSpawnerEvents();
}

void UMainHUDWidget::HandleMonsterCountChanged(int32 NewCount)
{
	const int32 MaxCount = GAMEOVER_MONSTER_NUM; // 기획서 고정값

	// 블루프린트(UMG) 이벤트 호출
	OnMonsterCountChanged(NewCount, MaxCount);
}