#include "MainHUDWidget.h"
#include "MyPlayerState.h"
#include "RamdomItemDefensePlayerController.h"
#include "MonsterSpawner.h"
#include "Kismet/GameplayStatics.h"

bool UMainHUDWidget::Initialize()
{
	bool bSuccess = Super::Initialize();
	if (!bSuccess) return false;

	// �� ������ ������ �÷��̾��� PlayerState�� Controller�� �����ɴϴ�.
	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	MyPlayerController = GetOwningPlayer<ARamdomItemDefensePlayerController>();

	// PlayerState�� ��ȿ�ϴٸ� �̺�Ʈ�� ���ε��մϴ�.
	if (MyPlayerState)
	{
		BindPlayerStateEvents();
	}
	else
	{
		// ���� PlayerState�� ���� �غ���� �ʾҴٸ�, ��� �� �ٽ� �õ��մϴ�.
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
	// PlayerState�� ������ ������ �ٽ� Ȯ���մϴ� (��: ��õ�)
	if (MyPlayerState == nullptr)
	{
		MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	}

	if (MyPlayerState)
	{
		// PlayerState�� C++ ��������Ʈ�� �� ������ UFUNCTION�� ���ε��մϴ�.
		MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UMainHUDWidget::OnGoldChanged);
		MyPlayerState->OnSpawnerAssignedDelegate.AddDynamic(this, &UMainHUDWidget::HandleSpawnerAssigned);

		// ���ε� ����, ���� ������ UI�� ��� ������Ʈ�ϵ��� �̺�Ʈ�� ȣ���մϴ�.
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
		// �������� ���� �� ���� ��������Ʈ�� C++ �Լ��� ����
		MyPlayerState->MySpawner->OnMonsterCountChangedDelegate.AddDynamic(this, &UMainHUDWidget::HandleMonsterCountChanged);

		// ���� ������ ��� ������Ʈ
		HandleMonsterCountChanged(MyPlayerState->MySpawner->GetCurrentMonsterCount());
	}
}

// --- ��ư Ŭ�� �ڵ鷯 ---
void UMainHUDWidget::HandleStatUpgradeClicked()
{
	 if (MyPlayerController)
	 {
	 	// (���� ����) ��Ʈ�ѷ��� ����â ��� �Լ� ȣ��
	  MyPlayerController->ToggleStatUpgradeWidget();
	 }
}

void UMainHUDWidget::HandleInventoryClicked()
{
	 if (MyPlayerController)
	 {
	 	// (���� ����) ��Ʈ�ѷ��� �κ��丮 ��� �Լ� ȣ��
	  MyPlayerController->ToggleInventoryWidget();
	 }
}

void UMainHUDWidget::HandleSpawnerAssigned(int32 Dummy)
{
	// ������ ��������Ʈ�� ���ε��մϴ�.
	BindSpawnerEvents();
}

void UMainHUDWidget::HandleMonsterCountChanged(int32 NewCount)
{
	const int32 MaxCount = GAMEOVER_MONSTER_NUM; // ��ȹ�� ������

	// �������Ʈ(UMG) �̺�Ʈ ȣ��
	OnMonsterCountChanged(NewCount, MaxCount);
}