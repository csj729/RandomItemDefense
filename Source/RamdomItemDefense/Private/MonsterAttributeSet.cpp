// Source/RamdomItemDefense/Private/MonsterAttributeSet.cpp (수정)

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h" 
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"
#include "RamdomItemDefense.h"
#include "MyGameState.h"       
#include "Engine/World.h"      
// --- [ ★★★ 코드 추가 ★★★ ] ---
#include "Kismet/KismetMathLibrary.h" // FMath::Pow() 함수 사용
// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

void UMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}


// --- [ ★★★ 새 함수 구현 ★★★ ] ---
float UMonsterAttributeSet::CalculateReducedDamage(float IncomingDamage) const
{
	// 1. 현재 방어력 가져오기 (최소 0)
	const float CurrentArmor = FMath::Max(0.f, GetArmor());

	// 2. 방어력이 0 이하면, 원본 데미지(양수)를 그대로 반환
	if (CurrentArmor <= 0.f)
	{
		return IncomingDamage;
	}

	// 3. 방어력 공식을 통한 데미지 감소율(%) 계산
	// 데미지 감소율 = 0.05 * (방어력 ^ 0.4515)
	float DamageReductionPercent = 0.05f * FMath::Pow(CurrentArmor, 0.4515f);

	// (안전장치) 데미지 감소율이 90%를 넘지 않도록 제한
	DamageReductionPercent = FMath::Min(DamageReductionPercent, 0.9f);

	// 4. 최종 데미지(양수) 계산 및 반환
	const float ReducedDamage = IncomingDamage * (1.0f - DamageReductionPercent);

	return ReducedDamage;
}
// --- [ ★★★ 구현 끝 ★★★ ] ---


void UMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float Magnitude = Data.EvaluatedData.Magnitude;
		const float NewHealth = GetHealth(); // (참고: 이 값은 이미 Magnitude가 적용된 값)

		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		if (Monster && !Monster->IsDying())
		{
			if (NewHealth <= 0.f)
			{
				// [1] --- 몬스터 사망 로직 ---
				// (기존 사망 로직과 동일)
				AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
				AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();
				ARamdomItemDefenseCharacter* KillerCharacter = Cast<ARamdomItemDefenseCharacter>(InstigatorActor ? InstigatorActor : EffectCauserActor);

				Monster->Die(KillerCharacter);

				if (KillerCharacter)
				{
					AMyPlayerState* PS = KillerCharacter->GetPlayerState<AMyPlayerState>();
					if (PS)
					{
						AMyGameState* MyGameState = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
						int32 CurrentWave = (MyGameState && MyGameState->GetCurrentWave() > 0) ? MyGameState->GetCurrentWave() : 1;

						if (Monster->IsBoss())
						{
							const int32 BossStage = CurrentWave / 10;
							const int32 ItemChoiceReward = BossStage * 3;
							PS->AddCommonItemChoice(ItemChoiceReward);
							const int32 BaseGold = 3000;
							const int32 BonusGold = (BossStage - 1) * 4000;
							PS->AddGold(BaseGold + BonusGold);
						}
						else
						{
							const int32 BaseGold = 10;
							const int32 BonusGold = (CurrentWave - 1) * 5;
							PS->AddGold(FMath::Max(BaseGold, BaseGold + BonusGold));
						}
					}
				}
				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);
				Monster->PlayHitEffect(EffectTags);
			}
			else if (Magnitude < 0.f)
			{
				// [2] --- ★★★ 몬스터 피격 (데미지) 로직 (수정됨) ★★★ ---

				// 2-1. 원본 데미지(음수)와 피해 입기 전 체력 계산
				const float OriginalDamage = Magnitude; // 예: -100
				const float HealthBeforeDamage = NewHealth - OriginalDamage; // 예: 400 - (-100) = 500

				// 2-2. 새 함수를 호출하여 실제 적용될 데미지(양수) 계산
				// 원본 데미지를 양수로 변환하여 전달 ( -OriginalDamage )
				const float ActualDamageToApply = CalculateReducedDamage(-OriginalDamage); // 예: CalculateReducedDamage(100) -> 60 반환

				// 2-3. 새 체력 값 설정
				// (피해 입기 전 체력) - (방어력이 적용된 실제 데미지)
				SetHealth(HealthBeforeDamage - ActualDamageToApply); // 예: 500 - 60 = 440
				UE_LOG(LogTemp, Warning, TEXT("Damage = %f"), ActualDamageToApply);
				// 2-4. 피격 이펙트 재생
				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);
				Monster->PlayHitEffect(EffectTags);

				// --- [ ★★★ 수정 끝 ★★★ ] ---
			}
			else
			{
				// [3] 힐 또는 기타 효과 (기존과 동일)
				RID_LOG(FColor::Orange, TEXT("MonsterAttributeSet: Health changed, but NOT hit (Magnitude: %.1f) and NOT dead (NewHealth: %.1f)."), Magnitude, NewHealth);
			}
		}
	}
	// --- [ Health/MaxHealth 동기화 로직 ] ---
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// (기존과 동일)
		const float NewMaxHealth = GetMaxHealth();
		SetHealth(NewMaxHealth);
	}
	// --- [ 로직 끝 ] ---
}

// RepNotify 함수들 구현
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }