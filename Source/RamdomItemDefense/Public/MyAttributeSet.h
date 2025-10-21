// MyAttributeSet.h

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ItemTypes.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.generated.h"

class ARamdomItemDefenseCharacter;

// Getter/Setter 매크로
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
    // 변수 복제를 위해 필요한 함수
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- 기본 스탯 (골드 강화 가능) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackDamage)
    FGameplayAttributeData AttackDamage;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, AttackDamage);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackSpeed)
    FGameplayAttributeData AttackSpeed;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, AttackSpeed);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CritDamage)
    FGameplayAttributeData CritDamage;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, CritDamage);

    // --- 특수 스탯 (골드 확률 강화 가능) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ArmorReduction)
    FGameplayAttributeData ArmorReduction;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, ArmorReduction);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeedReduction)
    FGameplayAttributeData MoveSpeedReduction;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, MoveSpeedReduction);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_SkillActivationChance)
    FGameplayAttributeData SkillActivationChance;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, SkillActivationChance);

    // --- 아이템 전용 스탯 (골드 강화 불가) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CritChance)
    FGameplayAttributeData CritChance;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, CritChance);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_StunChance)
    FGameplayAttributeData StunChance;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, StunChance);

    // --- 고정 스탯 (캐릭터마다 정해진 고정값) ---
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackRange)
    FGameplayAttributeData AttackRange;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, AttackRange);

protected:
    // --- 각 스탯에 대한 RepNotify 함수 선언 ---
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