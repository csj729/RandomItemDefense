#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MonsterAttributeSet.generated.h"

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

    // --- [ Attributes ] ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, MaxHealth);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Armor)
    FGameplayAttributeData Armor;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, Armor);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, MoveSpeed);

    // --- [ Public Helpers ] ---
    /** 방어력 수치를 기반으로 감소된 데미지 반환 */
    UFUNCTION(BlueprintPure, Category = "Attributes|Calculation")
    float CalculateReducedDamage(float IncomingDamage) const;

protected:
    // --- [ Replication Callbacks ] ---
    UFUNCTION() virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
    UFUNCTION() virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
    UFUNCTION() virtual void OnRep_Armor(const FGameplayAttributeData& OldValue);
    UFUNCTION() virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);
};