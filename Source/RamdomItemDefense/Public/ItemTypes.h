#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "ItemTypes.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntChangedDelegate, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloatChangedDelegate, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedDelegate);

// --- [ Enums ] ---
UENUM(BlueprintType)
enum class EItemGrade : uint8
{
    Common       UMETA(DisplayName = "흔함"),
    Uncommon     UMETA(DisplayName = "안흔함"),
    Special      UMETA(DisplayName = "특별함"),
    Rare         UMETA(DisplayName = "희귀함"),
    Legendary    UMETA(DisplayName = "전설적인"),
    Hidden       UMETA(DisplayName = "히든"),
    Transcendent UMETA(DisplayName = "초월적인"),
    Immortal     UMETA(DisplayName = "불멸의"),
    Eternal      UMETA(DisplayName = "영원함")
};

UENUM(BlueprintType)
enum class EItemStatType : uint8
{
    AttackDamage          UMETA(DisplayName = "공격력"),
    AttackSpeed           UMETA(DisplayName = "공격 속도"),
    CritDamage            UMETA(DisplayName = "치명타 피해"),
    CritChance            UMETA(DisplayName = "치명타 확률"),
    ArmorReduction        UMETA(DisplayName = "방어력 감소"),
    MoveSpeedReduction    UMETA(DisplayName = "이동 속도 감소"),
    StunChance            UMETA(DisplayName = "스턴 확률"),
    SkillActivationChance UMETA(DisplayName = "스킬 발동 확률")
};

UENUM(BlueprintType)
enum class EButtonActionKey : uint8
{
    Key_Q   UMETA(DisplayName = "Q Key"),
    Key_W   UMETA(DisplayName = "W Key"),
    Key_E   UMETA(DisplayName = "E Key"),
    Key_R   UMETA(DisplayName = "R Key"),
    Key_A   UMETA(DisplayName = "A Key"),
    Key_S   UMETA(DisplayName = "S Key"),
    Key_D   UMETA(DisplayName = "D Key"),
    Key_F   UMETA(DisplayName = "F Key")
};

UENUM(BlueprintType)
enum class EOnHitEffectType : uint8
{
    None,
    Slow,
    Stun,
    Damage
};

// --- [ Structs ] ---
USTRUCT(BlueprintType)
struct FItemStatData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) EItemStatType StatType;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Value;
};

USTRUCT(BlueprintType)
struct FItemOnHitEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) EOnHitEffectType EffectType = EOnHitEffectType::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Chance = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Magnitude = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Duration = 3.0f;
};

USTRUCT(BlueprintType)
struct RAMDOMITEMDEFENSE_API FItemData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data") FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data") FText ItemName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data") TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data") EItemGrade Grade;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data") FText ItemDescription;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS") TArray<FItemStatData> BaseStats;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS") TSubclassOf<UGameplayAbility> GrantedAbility;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") TArray<FItemOnHitEffect> OnHitEffects;
};

USTRUCT(BlueprintType)
struct RAMDOMITEMDEFENSE_API FRecipeData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe Data") FName ResultItemID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe Data") TArray<FName> Ingredients;
};