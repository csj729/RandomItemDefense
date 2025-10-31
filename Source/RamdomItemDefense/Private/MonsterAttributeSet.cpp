// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"
#include "RamdomItemDefense.h"
#include "MyGameState.h"       // GetCurrentWave() 사용
#include "Engine/World.h"      // GetWorld() 사용

void UMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMonsterAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float NewHealth = GetHealth();
		const float Magnitude = Data.EvaluatedData.Magnitude; // 데미지/힐량

		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		if (Monster && !Monster->IsDying())
		{
			if (NewHealth <= 0.f)
			{
				// (기존 사망 로직)
				AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
				AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();

				ARamdomItemDefenseCharacter* KillerCharacter = Cast<ARamdomItemDefenseCharacter>(InstigatorActor);
				if (KillerCharacter == nullptr)
				{
					KillerCharacter = Cast<ARamdomItemDefenseCharacter>(EffectCauserActor);
				}

				Monster->Die(KillerCharacter);

				if (KillerCharacter)
				{
					AMyPlayerState* PS = KillerCharacter->GetPlayerState<AMyPlayerState>();
					if (PS)
					{
						// --- [ ★★★ 몬스터 처치 골드 계산 로직 수정 ★★★ ] ---

						// 1. GameState에서 현재 웨이브 가져오기
						AMyGameState* MyGameState = GetWorld() ? GetWorld()->GetGameState<AMyGameState>() : nullptr;
						int32 CurrentWave = 1; // GameState가 없거나 웨이브가 0이면 1로 간주
						if (MyGameState && MyGameState->GetCurrentWave() > 0)
						{
							CurrentWave = MyGameState->GetCurrentWave();
						}

						// 2. 웨이브 기반으로 골드 계산 (10 + (Wave - 1) * 5)
						// const int32 GoldAmount = Monster->GetGoldOnDeath(); // (기존 코드)
						const int32 BaseGold = 10;
						const int32 BonusGold = (CurrentWave - 1) * 5;
						const int32 GoldAmount = FMath::Max(BaseGold, BaseGold + BonusGold); // Wave 1일 때 10골드 보장

						// --- [ ★★★ 로직 수정 끝 ★★★ ] ---

						PS->AddGold(GoldAmount);
						RID_LOG(FColor::Yellow, TEXT("Awarded %d gold to %s for killing %s (Wave: %d)"),
							GoldAmount,
							*PS->GetPlayerName(),
							*Monster->GetName(),
							CurrentWave
						);
					}
				}

				// --- [ 피격 이펙트 재생 (사망 시) ] ---
				RID_LOG(FColor::Yellow, TEXT("MonsterAttributeSet: Playing HIT effect on DEATH. Checking Asset Tags..."));

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);

				Monster->PlayHitEffect(EffectTags);
				// --- [ 코드 끝 ] ---
			}
			else if (Magnitude < 0.f)
			{
				// [로그 5: 피격 처리 진입]
				//RID_LOG(FColor::Yellow, TEXT("MonsterAttributeSet: Monster was HIT (Magnitude < 0 and NewHealth > 0). Checking Asset Tags..."));

				FGameplayTagContainer EffectTags;
				Data.EffectSpec.GetAllAssetTags(EffectTags);


				Monster->PlayHitEffect(EffectTags);
			}
			else
			{
				// [로그 6: 아무 조건도 맞지 않음]
				RID_LOG(FColor::Orange, TEXT("MonsterAttributeSet: Health changed, but NOT hit (Magnitude: %.1f) and NOT dead (NewHealth: %.1f)."), Magnitude, NewHealth);
			}
		}
	}
	// --- [ Health/MaxHealth 동기화 로직 ] ---
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// [로그 7: MaxHealth 변경 감지]
		//RID_LOG(FColor::Green, TEXT("MonsterAttributeSet: MaxHealth Attribute CHANGED."));

		// 새 MaxHealth 값을 가져옵니다.
		const float NewMaxHealth = GetMaxHealth();

		// 현재 Health 값을 새 MaxHealth 값으로 즉시 설정합니다.
		SetHealth(NewMaxHealth);

		// [로그 8: Health 동기화 완료]
		//RID_LOG(FColor::Green, TEXT("MonsterAttributeSet: Health SYNCED to NewMaxHealth: %.1f"), NewMaxHealth);
	}
	// --- [ 로직 끝 ] ---
}

// RepNotify 함수들 구현
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }