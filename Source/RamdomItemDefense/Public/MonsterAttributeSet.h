// Source/RamdomItemDefense/Public/MonsterAttributeSet.h

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MonsterAttributeSet.generated.h"

class AMyGameState;

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class RAMDOMITEMDEFENSE_API UMonsterAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

    // 체력
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, Health);

    // 최대 체력
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, MaxHealth);

    // 방어력
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Armor)
    FGameplayAttributeData Armor;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, Armor);

    // 이동 속도
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, MoveSpeed);

    // --- [ ★★★ 코드 추가 ★★★ ] ---
    /**
     * @brief 현재 방어력을 기반으로 실제 적용될 데미지를 계산합니다.
     * @param IncomingDamage 방어력 감소 전의 원본 데미지 (양수)
     * @return 방어력 공식이 적용된 최종 데미지 (양수)
     */
    UFUNCTION(BlueprintPure, Category = "Attributes|Calculation")
    float CalculateReducedDamage(float IncomingDamage) const;
    // --- [ ★★★ 코드 추가 끝 ★★★ ] ---

protected:
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_Armor(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
};