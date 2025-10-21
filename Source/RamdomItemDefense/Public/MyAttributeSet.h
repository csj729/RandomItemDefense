// MyAttributeSet.h

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ItemTypes.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.generated.h"

class ARamdomItemDefenseCharacter;

// Getter/Setter ��ũ��
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class RAMDOMITEMDEFENSE_API UMyAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    // ���� ������ ���� �ʿ��� �Լ�
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- �⺻ ���� (��� ��ȭ ����) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackDamage)
    FGameplayAttributeData AttackDamage;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, AttackDamage);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackSpeed)
    FGameplayAttributeData AttackSpeed;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, AttackSpeed);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CritDamage)
    FGameplayAttributeData CritDamage;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, CritDamage);

    // --- Ư�� ���� (��� Ȯ�� ��ȭ ����) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ArmorReduction)
    FGameplayAttributeData ArmorReduction;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, ArmorReduction);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeedReduction)
    FGameplayAttributeData MoveSpeedReduction;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, MoveSpeedReduction);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_SkillActivationChance)
    FGameplayAttributeData SkillActivationChance;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, SkillActivationChance);

    // --- ������ ���� ���� (��� ��ȭ �Ұ�) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CritChance)
    FGameplayAttributeData CritChance;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, CritChance);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_StunChance)
    FGameplayAttributeData StunChance;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, StunChance);

    // --- ���� ���� (ĳ���͸��� ������ ������) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackRange)
    FGameplayAttributeData AttackRange;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, AttackRange);

protected:
    // --- �� ���ȿ� ���� RepNotify �Լ� ���� ---
    UFUNCTION()
    virtual void OnRep_AttackDamage(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_CritDamage(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_ArmorReduction(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MoveSpeedReduction(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_SkillActivationChance(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_CritChance(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_StunChance(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_AttackRange(const FGameplayAttributeData& OldValue);
};