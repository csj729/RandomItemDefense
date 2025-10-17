// MonsterAttributeSet.cpp

#include "MonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Engine/Engine.h"

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
        // AttributeSet의 현재 체력 값을 가져옵니다. 이것이 '이전 값'입니다.
        const float OldHealth = GetHealth();

        // 적용될 변화량 (데미지는 보통 음수이므로 그대로 사용)
        const float Magnitude = Data.EvaluatedData.Magnitude;

        // 예상되는 '새로운 값'을 계산합니다.
        const float NewHealth = OldHealth + Magnitude;

        if (GEngine)
        {
            FString DebugMessage = FString::Printf(TEXT("Monster Health (Server): %.1f -> %.1f (Change: %.1f)"),
                OldHealth,    // 이전 체력
                NewHealth,    // 계산된 새로운 체력
                Magnitude     // 적용된 변화량
            );
            // 화면에 2초간 청록색으로 메시지를 출력합니다. (OnRep과 색 구분)
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, DebugMessage);
        }
    }
}

// RepNotify 함수들 구현
void UMonsterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Health, OldValue); }
void UMonsterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MaxHealth, OldValue); }
void UMonsterAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, Armor, OldValue); }
void UMonsterAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(UMonsterAttributeSet, MoveSpeed, OldValue); }