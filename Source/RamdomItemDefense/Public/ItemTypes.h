// ItemTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "ItemTypes.generated.h"

#define GAMEOVER_MONSTER_NUM 60
#define MAX_NORMAL_STAT_LEVEL 100
#define MAX_SPECIAL_STAT_LEVEL 3
#define BASE_LEVELUP_COST 100
#define INCREASING_COST_PER_LEVEL 50

#define MAX_ULTIMATE_CHARGE 5 // 궁극기 최대 스택

#define SPECIAL_STAT_UPGRADE_CHANCE_LVL0 0.5f // 0 -> 1
#define SPECIAL_STAT_UPGRADE_CHANCE_LVL1 0.4f // 1 -> 2
#define SPECIAL_STAT_UPGRADE_CHANCE_LVL2 0.3f // 2 -> 3

#define MONSTER_BASE_HP 100

// 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntChangedDelegate, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloatChangedDelegate, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedDelegate);


// 아이템 등급 열거형
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

// 스탯 종류 열거형
UENUM(BlueprintType)
enum class EItemStatType : uint8
{
    AttackDamage          UMETA(DisplayName = "공격력"),
    AttackSpeed           UMETA(DisplayName = "공격 속도"),
    CritDamage            UMETA(DisplayName = "치명타 피해"),
    CritChance            UMETA(DisplayName = "치명타 확률"),
    ArmorReduction        UMETA(DisplayName = "방어력 감소"),
    MoveSpeedReduction    UMETA(DisplayName = "이동 속도 감소"),
    StunChance            UMETA(DisplayName = "스턴 확률"), // MyAttributeSet에도 있어야 함
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

/**
 * @brief SetByCaller GE에 주입할 스탯 데이터 (데이터 테이블 편집용)
 */
USTRUCT(BlueprintType)
struct FItemStatData
{
    GENERATED_BODY()

    /** 이 아이템이 변경할 스탯의 종류입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EItemStatType StatType;

    /** 스탯에 더할 값입니다. (예: 10, 0.05) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Value;
};

// 아이템 데이터 구조체
USTRUCT(BlueprintType)
struct RAMDOMITEMDEFENSE_API FItemData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    EItemGrade Grade;

    /** (SetByCaller) 이 아이템이 제공하는 기본 스탯 목록입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TArray<FItemStatData> BaseStats;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TSubclassOf<UGameplayAbility> GrantedAbility;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    FText ItemDescription;
};

// 조합법 데이터 구조체
USTRUCT(BlueprintType)
struct RAMDOMITEMDEFENSE_API FRecipeData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe Data")
    FName ResultItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe Data")
    TArray<FName> Ingredients;
};