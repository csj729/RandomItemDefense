#include "MyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "MonsterSpawner.h"
#include "RamdomItemDefenseCharacter.h"
#include "InventoryComponent.h"

AMyPlayerState::AMyPlayerState()
{
	Gold = 0;
	ChoiceCount = 0;
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyPlayerState, Gold);
	DOREPLIFETIME(AMyPlayerState, ChoiceCount);
	DOREPLIFETIME(AMyPlayerState, MySpawner);
}

// --- ��� ���� (������) ---
void AMyPlayerState::AddGold(int32 Amount)
{
	if (HasAuthority())
	{
		Gold += Amount;
		OnRep_Gold(); // ���������� ��������Ʈ ȣ��
	}
}

void AMyPlayerState::OnRep_Gold()
{
	// ��� ���� ��������Ʈ�� ����մϴ�.
	OnGoldChangedDelegate.Broadcast(Gold);
}

void AMyPlayerState::OnRep_MySpawner()
{
	// �����ʰ� ��ȿ�ϰ� �Ҵ�Ǿ����� UI(MainHUD)�� �˸��ϴ�.
	OnSpawnerAssignedDelegate.Broadcast(0); // 0�� �ǹ� ���� ��
}

// --- [�ڵ� �߰�] ���� ���� ���� ---

/**
 * @brief (���� ����) ���� ���� Ƚ���� �����մϴ�.
 */
void AMyPlayerState::AddChoiceCount(int32 Count)
{
	if (HasAuthority())
	{
		ChoiceCount += Count;
		OnRep_ChoiceCount(); // ���������� ��������Ʈ ȣ��
	}
}

/**
 * @brief Ŭ���̾�Ʈ���� ChoiceCount�� �����Ǿ��� �� ȣ��˴ϴ�.
 */
void AMyPlayerState::OnRep_ChoiceCount()
{
	// ���� Ƚ�� ���� ��������Ʈ�� ����մϴ�.
	OnChoiceCountChangedDelegate.Broadcast(ChoiceCount);
}

/**
 * @brief (UI���� ȣ��) ���� ����(������/���)�� ����մϴ�.
 */
void AMyPlayerState::Server_UseRoundChoice_Implementation(bool bChoseItemGacha)
{
	// (�������� �����)
	if (ChoiceCount <= 0)
	{
		return; // ���� Ƚ���� ������ ����
	}

	ChoiceCount--;
	OnRep_ChoiceCount(); // �������� ��� ��������Ʈ ȣ��

	if (bChoseItemGacha)
	{
		// �� PlayerState�� ������ ��(ĳ����)�� �����ɴϴ�.
		ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
		if (Character && Character->GetInventoryComponent())
		{
			// ĳ������ �κ��丮 ������Ʈ���� ������ �������� �߰��϶�� ����մϴ�.
			Character->GetInventoryComponent()->AddRandomItem();
		}

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("Player chose: Item Gacha"));
	}
	else
	{
		// TODO: ��� ���� ���� (���� ��� AddGold)
		const int32 GambleAmount = FMath::RandRange(100, 500); // ��: 100~500 ���
		AddGold(GambleAmount);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("Player chose: Gold Gamble (+%d)"), GambleAmount));
	}
}
// ------------------------------------