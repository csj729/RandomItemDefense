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

	// ��� ��ȭ ������ ������ �ʱ� ������ 0���� ����
	AttackDamageLevel = 0;
	AttackSpeedLevel = 0;
	CritDamageLevel = 0;
	ArmorReductionLevel = 0;
	SkillActivationChanceLevel = 0;
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyPlayerState, Gold);
	DOREPLIFETIME(AMyPlayerState, ChoiceCount);
	DOREPLIFETIME(AMyPlayerState, MySpawner);
	
	DOREPLIFETIME(AMyPlayerState, AttackDamageLevel);
	DOREPLIFETIME(AMyPlayerState, AttackSpeedLevel);
	DOREPLIFETIME(AMyPlayerState, CritDamageLevel);
	DOREPLIFETIME(AMyPlayerState, ArmorReductionLevel);
	DOREPLIFETIME(AMyPlayerState, SkillActivationChanceLevel);
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

// --- ��� �Ҹ� �Լ� ---
bool AMyPlayerState::SpendGold(int32 Amount)
{
	if (!HasAuthority()) return false; // ���������� ����
	if (Amount <= 0) return true; // 0 ���� �Ҹ�� �׻� ����

	if (Gold >= Amount)
	{
		Gold -= Amount;
		OnRep_Gold(); // ���� UI ��� ������Ʈ �� Ŭ���̾�Ʈ ���� Ʈ����
		return true; // �Ҹ� ����
	}
	return false; // ��� ����
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

/** Ŭ���̾�Ʈ���� StatLevels ���� �� ȣ�� */
// �� OnRep �Լ��� �ش� ���� Ÿ�԰� �� ������ ��������Ʈ�� ȣ���մϴ�.
void AMyPlayerState::OnRep_AttackDamageLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::AttackDamage, AttackDamageLevel); }
void AMyPlayerState::OnRep_AttackSpeedLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::AttackSpeed, AttackSpeedLevel); }
void AMyPlayerState::OnRep_CritDamageLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::CritDamage, CritDamageLevel); }
void AMyPlayerState::OnRep_ArmorReductionLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::ArmorReduction, ArmorReductionLevel); }
void AMyPlayerState::OnRep_SkillActivationChanceLevel() { OnStatLevelChangedDelegate.Broadcast(EItemStatType::SkillActivationChance, SkillActivationChanceLevel); }

/** Ư�� ���� ���� ��ȯ */
int32 AMyPlayerState::GetStatLevel(EItemStatType StatType) const
{
	// --- [�ڵ� ����] switch �� ��� ---
	switch (StatType)
	{
	case EItemStatType::AttackDamage: return AttackDamageLevel;
	case EItemStatType::AttackSpeed: return AttackSpeedLevel;
	case EItemStatType::CritDamage: return CritDamageLevel;
	case EItemStatType::ArmorReduction: return ArmorReductionLevel;
	case EItemStatType::SkillActivationChance: return SkillActivationChanceLevel;
	default: return 0;
	}
	// ----------------------------------
}

/** UI���� ȣ���ϴ� ���� RPC */
void AMyPlayerState::Server_RequestStatUpgrade_Implementation(EItemStatType StatToUpgrade)
{
	// �������� ���� ��ȭ �õ�
	TryUpgradeStat(StatToUpgrade);
}

/** (���� ����) ���� ��ȭ ���� */
/** (���� ����) ���� ��ȭ ���� */
bool AMyPlayerState::TryUpgradeStat(EItemStatType StatToUpgrade)
{
	if (!HasAuthority()) return false;

	// ��ȭ ���� ���� Ȯ�� (���� ����)
	const bool bIsGoldUpgradableBasicStat = (StatToUpgrade == EItemStatType::AttackDamage ||
		StatToUpgrade == EItemStatType::AttackSpeed ||
		StatToUpgrade == EItemStatType::CritDamage);

	const bool bIsGoldUpgradableSpecialStat = (StatToUpgrade == EItemStatType::ArmorReduction ||
		StatToUpgrade == EItemStatType::SkillActivationChance);

	if (!bIsGoldUpgradableBasicStat && !bIsGoldUpgradableSpecialStat)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TryUpgradeStat: %s cannot be upgraded with gold."), *UEnum::GetValueAsString(StatToUpgrade)));
		return false;
	}

	int32 CurrentLevel = GetStatLevel(StatToUpgrade);

	// --- ��ȭ ��Ģ (���� ����) ---
	int32 MaxLevel = bIsGoldUpgradableBasicStat ? MAX_NORMAL_STAT_LEVEL : MAX_SPECIAL_STAT_LEVEL; // Ư�� ���� �ִ� 3����
	int32 BaseCost = BASE_LEVELUP_COST;
	int32 CostIncreaseFactor = INCREASING_COST_PER_LEVEL;
	// ---------------------------

	// ���� ���� Ȯ�� (���� ����)
	if (CurrentLevel >= MaxLevel)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("TryUpgradeStat: Max level reached"));
		return false;
	}

	// ��� ��� (���� ����)
	int32 UpgradeCost = BaseCost + (CurrentLevel * CostIncreaseFactor);

	// ��� Ȯ�� �� �Ҹ� (���� ����)
	if (!SpendGold(UpgradeCost))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("TryUpgradeStat: Not enough gold"));
		return false;
	}

	// --- [�ڵ� ����] ���� Ȯ�� ��� (���ο� ��Ģ ����) ---
	bool bUpgradeSuccess = true; // �⺻ ������ �׻� ����
	float SuccessChance = 1.0f; // ���� Ȯ�� ��Ͽ� ����

	if (bIsGoldUpgradableSpecialStat) // Ư�� ������ ��� Ȯ�� ����
	{
		// CurrentLevel �������� ���� ������ �� Ȯ�� ����
		switch (CurrentLevel)
		{
		case 0: // 0 -> 1 ���� ��ȭ �õ�
			SuccessChance = 0.5f; // 50%
			break;
		case 1: // 1 -> 2 ���� ��ȭ �õ�
			SuccessChance = 0.4f; // 40%
			break;
		case 2: // 2 -> 3 ���� ��ȭ �õ�
			SuccessChance = 0.3f; // 30%
			break;
		default: // �̹� �ִ� �����̰ų� ���� ��Ȳ (�̷л� ���⿡ �����ϸ� �� ��)
			SuccessChance = 0.0f;
			break;
		}
		// 0.0 ~ 1.0 ���� ���� �����Ͽ� ���� ���� ����
		bUpgradeSuccess = (FMath::FRand() < SuccessChance);
	}
	// ----------------------------------------------------

	// ��ȭ ���� �� (���� ����)
	if (bUpgradeSuccess)
	{
		// 1. ���� ���� �� ���� ������Ʈ (switch ���)
		int32 NewLevel = CurrentLevel + 1;
		bool bLevelUpdated = false;
		switch (StatToUpgrade)
		{
		case EItemStatType::AttackDamage: AttackDamageLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::AttackSpeed: AttackSpeedLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::CritDamage: CritDamageLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::ArmorReduction: ArmorReductionLevel = NewLevel; bLevelUpdated = true; break;
		case EItemStatType::SkillActivationChance: SkillActivationChanceLevel = NewLevel; bLevelUpdated = true; break;
		}

		// ���� ������Ʈ ���� �� ���� UI ��� ������Ʈ �� Ŭ���̾�Ʈ ���� Ʈ����
		if (bLevelUpdated)
		{
			// ����� ������ OnRep �Լ� ���� ȣ�� (���� ����)
			switch (StatToUpgrade)
			{
			case EItemStatType::AttackDamage: OnRep_AttackDamageLevel(); break;
			case EItemStatType::AttackSpeed: OnRep_AttackSpeedLevel(); break;
			case EItemStatType::CritDamage: OnRep_CritDamageLevel(); break;
			case EItemStatType::ArmorReduction: OnRep_ArmorReductionLevel(); break;
			case EItemStatType::SkillActivationChance: OnRep_SkillActivationChanceLevel(); break;
			}

			if (GEngine) // ���� �α� (���� ����)
			{
				FString StatName = UEnum::GetValueAsString(StatToUpgrade);
				// Ư�� ���� ���� �� Ȯ���� �Բ� ǥ�� (���� ����)
				FString ChanceString = bIsGoldUpgradableSpecialStat ? FString::Printf(TEXT(" (Chance: %.0f%%)"), SuccessChance * 100) : TEXT("");
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Upgrade Success: %s to Level %d (Cost: %d)%s"), *StatName, NewLevel, UpgradeCost, *ChanceString));
			}
		}

		// 2. ���� ���� ���� ��û (ĳ���� ASC��) (���� ����)
		ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
		if (Character)
		{
			Character->ApplyStatUpgrade(StatToUpgrade, NewLevel);
		}
		return true;
	}
	else // ��ȭ ���� �� (Ư�� ���� - �α� ���� ����)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Upgrade Failed: %s (Level %d -> %d, Cost: %d, Chance: %.0f%%)"),
			*UEnum::GetValueAsString(StatToUpgrade), CurrentLevel, CurrentLevel + 1, UpgradeCost, SuccessChance * 100));
		return false;
	}
}