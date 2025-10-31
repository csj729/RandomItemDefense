// MonsterAttributeSet.h

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

    // ü��
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, Health);

    // �ִ� ü��
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, MaxHealth);

    // ����
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Armor)
    FGameplayAttributeData Armor;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, Armor);

    // �̵� �ӵ�
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UMonsterAttributeSet, MoveSpeed);

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