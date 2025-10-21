// ItemTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "ItemTypes.generated.h"

#define GAMEOVER_MONSTER_NUM 60


// ��������Ʈ ������ �� ���� ���Ͽ� *�� ����* �մϴ�.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntChangedDelegate, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloatChangedDelegate, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedDelegate);
// ------------------

// ������ ��� ������
UENUM(BlueprintType)
enum class EItemGrade : uint8
{
    Common       UMETA(DisplayName = "����"),
    Uncommon     UMETA(DisplayName = "������"),
    Special      UMETA(DisplayName = "Ư����"),
    Rare         UMETA(DisplayName = "�����"),
    Legendary    UMETA(DisplayName = "��������"),
    Hidden       UMETA(DisplayName = "����"),
    Transcendent UMETA(DisplayName = "�ʿ�����"),
    Immortal     UMETA(DisplayName = "�Ҹ���"),
    Eternal      UMETA(DisplayName = "������")
};

// ���� ���� ������
UENUM(BlueprintType)
enum class EItemStatType : uint8
{
    AttackDamage          UMETA(DisplayName = "���ݷ�"),
    AttackSpeed           UMETA(DisplayName = "���� �ӵ�"),
    CritDamage            UMETA(DisplayName = "ġ��Ÿ ����"),
    CritChance            UMETA(DisplayName = "ġ��Ÿ Ȯ��"),
    ArmorReduction        UMETA(DisplayName = "���� ����"),
    MoveSpeedReduction    UMETA(DisplayName = "�̵� �ӵ� ����"),
    StunChance            UMETA(DisplayName = "���� Ȯ��"),
    SkillActivationChance UMETA(DisplayName = "��ų �ߵ� Ȯ��")
};

/**
 * @brief SetByCaller GE�� ������ ���� ������ (������ ���̺� ������)
 */
USTRUCT(BlueprintType)
struct FItemStatData
{
    GENERATED_BODY()

    /** �� �������� ������ ������ �����Դϴ�. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EItemStatType StatType;

    /** ���ȿ� ���� ���Դϴ�. (��: 10, 0.05) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Value;
};

// ������ ������ ����ü
USTRUCT(BlueprintType)
struct RAMDOMITEMDEFENSE_API FItemData : public FTableRowBase // ���⿡ API ��ũ�� �߰�
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

    /** (SetByCaller) �� �������� �����ϴ� �⺻ ���� ����Դϴ�. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TArray<FItemStatData> BaseStats;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TSubclassOf<UGameplayAbility> GrantedAbility;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
    FText AbilityDescription;
};

// ���չ� ������ ����ü
USTRUCT(BlueprintType)
struct RAMDOMITEMDEFENSE_API FRecipeData : public FTableRowBase // ���⿡ API ��ũ�� �߰�
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe Data")
    FName ResultItemID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe Data")
    TArray<FName> Ingredients;
};