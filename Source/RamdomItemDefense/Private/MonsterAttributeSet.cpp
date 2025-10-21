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
        const float OldHealth = GetHealth();
        // NewHealth는 이미 계산된 후의 값이므로 GetHealth() 대신 사용합니다.
        const float NewHealth = Data.EvaluatedData.Magnitude + OldHealth;

        // OldHealth > 0 조건으로 "이미 죽지 않았는지" 반드시 확인합니다.
        if (NewHealth <= 0.f)
        {
            AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor());

            // --- [코드 수정] ---
            // Instigator (유발자)와 EffectCauser (효과 유발자)를 모두 가져옵니다.
            AActor* InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
            AActor* EffectCauserActor = Data.EffectSpec.GetContext().GetEffectCauser();

            // 1. Instigator를 캐스팅해 봅니다.
            ARamdomItemDefenseCharacter* KillerCharacter = Cast<ARamdomItemDefenseCharacter>(InstigatorActor);

            // 2. 만약 Instigator가 플레이어 캐릭터가 아니라면 (예: PlayerController),
            //    방금 블루프린트에서 설정한 EffectCauser를 캐스팅해 봅니다.
            if (KillerCharacter == nullptr)
            {
                KillerCharacter = Cast<ARamdomItemDefenseCharacter>(EffectCauserActor);
            }
            // ------------------

            // (디버깅을 위해 두 액터의 이름을 모두 출력해봅니다)
            if (GEngine)
            {
                FString InstigatorName = (InstigatorActor) ? InstigatorActor->GetName() : TEXT("NULL (Instigator)");
                FString CauserName = (EffectCauserActor) ? EffectCauserActor->GetName() : TEXT("NULL (EffectCauser)");
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Instigator: %s"), *InstigatorName));
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("EffectCauser: %s"), *CauserName));
            }


            // 1. 몬스터의 죽음 처리를 먼저 호출합니다.
            if (Monster)
            {
                Monster->Die(KillerCharacter);
            }

            // 2. 킬러와 몬스터가 모두 유효한 경우에만 골드를 지급합니다.
            if (KillerCharacter && Monster)
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
            else if (GEngine) // 킬러 정보를 여전히 못 가져온 경우 디버그 메시지
            {
                FString MonsterName = (Monster) ? Monster->GetName() : TEXT("NULL");
                FString KillerName = (KillerCharacter) ? KillerCharacter->GetName() : TEXT("NULL");
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                    FString::Printf(TEXT("Gold Award FAILED. Killer: %s, Monster: %s"), *KillerName, *MonsterName));
            }
        }
    }
}
// RepNotify 함수들 구현
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }