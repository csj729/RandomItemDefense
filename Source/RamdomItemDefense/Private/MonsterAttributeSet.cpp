// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"
#include "MonsterBaseCharacter.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"

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
		// --- [코드 수정] ---
		// 데미지 적용 *후*의 실제 체력 값을 가져옵니다.
		const float NewHealth = GetHealth();

		// 1. 몬스터 액터를 먼저 가져옵니다.
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

		// 2. 몬스터가 유효하고, [핵심] 아직 죽음 상태(bIsDying)가 아닌지 확인합니다.
		if (Monster && !Monster->IsDying())
		{
			// 3. 이 공격으로 인해 체력이 0 이하가 되었는지 확인합니다.
			if (NewHealth <= 0.f)
			{
				// 킬러(Instigator/EffectCauser)를 찾습니다.
				AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
				AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();

				ARamdomItemDefenseCharacter* KillerCharacter = Cast<ARamdomItemDefenseCharacter>(InstigatorActor);
				if (KillerCharacter == nullptr)
				{
					KillerCharacter = Cast<ARamdomItemDefenseCharacter>(EffectCauserActor);
				}

				// (디버깅 로그는 유지하셔도 좋습니다)

				// 4. Die() 함수를 호출합니다. (이 함수 내부에서 bIsDying이 true로 설정됩니다)
				Monster->Die(KillerCharacter);

				// 5. 킬러가 유효하면 골드를 지급합니다.
				if (KillerCharacter)
				{
					AMyPlayerState* PS = KillerCharacter->GetPlayerState<AMyPlayerState>();
					if (PS)
					{
						const int32 GoldAmount = Monster->GetGoldOnDeath();
						PS->AddGold(GoldAmount);

						if (GEngine)
						{
							FString GoldMessage = FString::Printf(TEXT("Awarded %d gold to %s for killing %s"),
								GoldAmount,
								*PS->GetPlayerName(),
								*Monster->GetName()
							);
							GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, GoldMessage);
						}
					}
				}
				// (킬러 정보가 없는 디버그 로그는 유지하셔도 좋습니다)
			}
		}
		// ------------------
	}
}

// RepNotify 함수들 구현
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }