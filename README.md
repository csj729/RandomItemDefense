<div align="center">

# 🎲 RandomItemDefense(RID) (UE5 Project)


<table>
  <tr>
    <td align="center" width="33%">
      <img src="https://github.com/user-attachments/assets/49e3d5ef-b1c4-4833-b1f1-3f2889ee7e46" alt="원작 게임 플레이" width="100%"/>
      <br/>
      <b>게임 로고</b>
    </td>
    <td align="center" width="33%">
      <img src="https://github.com/user-attachments/assets/2b93912e-64dc-4ba1-82c3-468d4a5de7ef" alt="게임 플레이" width="100%"/>
      <br/>
      <b>게임 플레이</b>
    </td>
  </tr>
</table>

</div>

---

## 🎮 게임 소개

**Unreal Engine 5와 C++**를 사용하여 제작한 멀티플레이어 랜덤 아이템 디펜스 게임입니다.

언리얼의 강력한 프레임워크인 **GAS(Gameplay Ability System)**를 기반으로 확장성 높은 전투 시스템을 구축하였으며, 로그라이크 요소를 결합하여 매 판 달라지는 아이템 조합과 전략적인 **1:1 PVP 대전**을 구현하였습니다.

---

## 📋 목차

- [게임 소개](#-게임-소개)
- [주요 스크립트](#-주요-스크립트)
  - [GAS 기반 전투 시스템](#-GAS-기반-전투-시스템)
  - [아이템, 인벤토리 시스템](#-아이템,-인벤토리-시스템)
  - [서버 권한 네트워크](#️-서버-권한-네트워크)
- [기술 스택](#️-기술-스택)
- [개발자](#-개발자)

---

## 📜 주요 스크립트

게임의 핵심 로직을 담당하는 주요 클래스와 대표적인 구현 코드를 소개합니다.

### ⚔️ GAS 기반 전투 시스템
* **역할:** 캐릭터의 스탯(Attributes), 스킬(Abilities), 상태 이상(Effects)을 체계적으로 관리합니다.
* **핵심 기능:** AttributeSet의 BaseValue를 직접 수정하여 게임 내 영구적인 스탯 강화를 처리하고, 이를 서버/클라이언트 간에 동기화합니다.

<details>
<summary><b>[코드 보기] ARamdomItemDefenseCharacter::ApplyStatUpgrade (스탯 영구 강화)</b></summary>

```cpp
void ARamdomItemDefenseCharacter::ApplyStatUpgrade(EItemStatType StatType, int32 NewLevel)
{
	if (!HasAuthority() || !AttributeSet)
	{
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		if (HasAuthority()) RID_LOG(FColor::Red, TEXT("ApplyStatUpgrade Error: Not Server or AttributeSet is NULL"));
		// -----------------------------------------
		return;
	}

	// 1. 이번 강화 단계로 인해 추가되어야 할 *증가량(Delta)* 계산
	float DeltaValue = 0.0f;
	switch (StatType)
	{
		// 예시: 레벨당 증가량 정의 (이 값들을 원하는 대로 조절하세요)
	case EItemStatType::AttackDamage: 			DeltaValue = 10.0f; break; // 레벨당 +10
	case EItemStatType::AttackSpeed: 			DeltaValue = 0.05f; break; // 레벨당 +5%
	case EItemStatType::CritDamage: 			DeltaValue = 0.1f; break;  // 레벨당 +10%
	case EItemStatType::ArmorReduction: 		DeltaValue = 10.0f; break;  // 레벨당 +10
	case EItemStatType::SkillActivationChance: 	DeltaValue = 0.03f; break; // 레벨당 +3%
	default:
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		RID_LOG(FColor::Red, TEXT("ApplyStatUpgrade Error: Invalid StatType for BaseValue modification: %s"), *UEnum::GetValueAsString(StatType));
		// -----------------------------------------
		return; // 골드 강화 불가능 스탯이면 종료
	}

	// 2. AttributeSet의 해당 스탯 BaseValue 조정 함수 호출
	switch (StatType)
	{
	case EItemStatType::AttackDamage: 			AttributeSet->AdjustBaseAttackDamage(DeltaValue); break;
	case EItemStatType::AttackSpeed: 			AttributeSet->AdjustBaseAttackSpeed(DeltaValue); break;
	case EItemStatType::CritDamage: 			AttributeSet->AdjustBaseCritDamage(DeltaValue); break;
	case EItemStatType::ArmorReduction: 		AttributeSet->AdjustBaseArmorReduction(DeltaValue); break;
	case EItemStatType::SkillActivationChance: 	AttributeSet->AdjustBaseSkillActivationChance(DeltaValue); break;
	}

	// 3. 로그 출력 (성공 확인)
	FString StatName = UEnum::GetValueAsString(StatType);
	RID_LOG(FColor::Cyan, TEXT("Applied BaseValue Upgrade: %s (Level %d, Delta: %.2f)"), *StatName, NewLevel, DeltaValue);
	// -----------------------------------------
}
```
</details>

---

### 🎲 아이템, 인벤토리 시스템
* **역할:** 라운드마다 주어지는 '아이템 뽑기'와 '골드 도박' 선택지를 관리하여 전략적 다양성을 부여하고 GAS 시스템을 활용해 인벤토리에 저장된 아이템의 스탯을 적용합니다.
* **핵심 기능:** 플레이어의 선택을 서버에서 처리하고, 확률(RNG)에 기반하여 아이템을 인벤토리에 지급하거나 골드 보상을 계산합니다.

<details> <summary><b>[코드 보기] AMyPlayerState::Server_UseRoundChoice (선택지 처리)</b></summary>

```cpp
void AMyPlayerState::Server_UseRoundChoice_Implementation(bool bChoseItemGacha)
{
	if (ChoiceCount <= 0)
	{
		return;
	}

	ChoiceCount--;
	OnRep_ChoiceCount(); // 서버에서 즉시 델리게이트 호출

	if (bChoseItemGacha)
	{
		ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
		if (Character && Character->GetInventoryComponent())
		{
			Character->GetInventoryComponent()->AddRandomItem();
		}

	}
	else
	{
		// 1. GameState에서 현재 웨이브 가져오기
		AMyGameState* MyGameState = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
		int32 CurrentWave = 1; // GameState가 없거나 웨이브가 0이면 1로 간주
		if (MyGameState && MyGameState->GetCurrentWave() > 0)
		{
			CurrentWave = MyGameState->GetCurrentWave();
		}

		// 2. 웨이브 기반으로 골드 계산 (기본값: 50 * Wave, 범위: ± 30)
		const int32 BaseAmount = 50 * CurrentWave;
		const int32 RandomBonus = FMath::RandRange(-30, 30); // TodoList "± 일정값"
		const int32 GambleAmount = FMath::Max(1, BaseAmount + RandomBonus); // 최소 1골드

		AddGold(GambleAmount);
	}
}
```
</details>

---

### 🌐 서버 권한 네트워크 (Server-Authoritative)
* **역할:** 멀티플레이 환경에서 데이터 무결성을 보장하고 치팅을 방지합니다.
* **핵심 기능:** 클라이언트의 중요 요청(강화, 재화 사용)을 Server RPC로 받아 서버가 검증(Validation) 및 수행하고, 결과는 RepNotify를 통해 UI에 반영합니다.

<details> <summary><b>[코드 보기] AMyPlayerState::Server_RequestStatUpgrade (강화 요청 검증)</b></summary>

```cpp
void AMyPlayerState::Server_RequestStatUpgrade_Implementation(EItemStatType StatToUpgrade)
{
    // 실제 로직은 서버(TryUpgradeStat)에서만 수행
    TryUpgradeStat(StatToUpgrade);
}

bool AMyPlayerState::TryUpgradeStat(EItemStatType StatToUpgrade)
{
    if (!HasAuthority()) return false; // [보안] 서버 권한 필수 확인

    AMyGameState* GameState = GetWorld()->GetGameState<AMyGameState>();
    int32 CurrentLevel = GetStatLevel(StatToUpgrade);
    int32 UpgradeCost = GameState->BaseLevelUpCost + (CurrentLevel * GameState->IncreasingCostPerLevel);

    // [검증] 골드가 충분한지 서버에서 직접 확인 및 차감
    if (!SpendGold(UpgradeCost)) 
    {
        return false; 
    }

    // [로직] 특수 스탯(치명타 등)은 확률적으로 성공 여부 결정
    bool bUpgradeSuccess = true;
    if (IsSpecialStat(StatToUpgrade))
    {
        float Chance = GameState->SpecialStatUpgradeChances[CurrentLevel];
        bUpgradeSuccess = (FMath::FRand() < Chance); // 서버에서 확률 계산
    }

    if (bUpgradeSuccess)
    {
        // 성공 시 레벨 증가 -> RepNotify에 의해 UI 자동 갱신
        UpdateStatLevel(StatToUpgrade, CurrentLevel + 1);
        
        // 캐릭터 GAS 스탯에 실제 적용
        if (ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>())
        {
            Character->ApplyStatUpgrade(StatToUpgrade, CurrentLevel + 1);
        }
    }
    return bUpgradeSuccess;
}
```
</details>

---

### 🛠️ 기술 스택
* **Engine** : Unreal Engine 5 (C++ & Blueprint)
* **Framework** : Gameplay Ability System (GAS)
* **Network** : Dedicated Server Architecture (Replication, RPCs)
* **AI** : Behavior Tree & Blackboard
* **UI** : UMG (MVVM Pattern)
---

<div align="center">

### 👨‍💻 개발자

<br>

**천성준**

<br>
<br>

[![GitHub](https://img.shields.io/badge/GitHub-csj729-181717?style=for-the-badge&logo=github)](https://github.com/csj729)

<br>

</div>

---
