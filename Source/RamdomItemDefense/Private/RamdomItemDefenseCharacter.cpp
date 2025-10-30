// Copyright Epic Games, Inc. All Rights Reserved.

#include "RamdomItemDefenseCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "MyAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "AttackComponent.h"
#include "InventoryComponent.h"
#include "GameplayTagsManager.h"
#include "ItemTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "RamdomItemDefense.h" // RID_LOG ��ũ�ο�

ARamdomItemDefenseCharacter::ARamdomItemDefenseCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create Components
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSet"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ARamdomItemDefenseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ARamdomItemDefenseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

UAbilitySystemComponent* ARamdomItemDefenseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ARamdomItemDefenseCharacter::ApplyDefaultStats()
{
	if (AbilitySystemComponent && DefaultStatsEffect)
	{
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultStatsEffect, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void ARamdomItemDefenseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		ApplyDefaultStats();

		for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultAbilities)
		{
			if (AbilityClass)
			{
				// FGameplayAbilitySpec�� �����Ͽ� �����Ƽ�� �ο��մϴ�.
				// (���� 1, �Է� ID ����, �ҽ� ������Ʈ�� this)
				FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
				AbilitySystemComponent->GiveAbility(Spec);
			}
		}
	}

	if (InventoryComponent)
	{
		InventoryComponent->Initialize(AbilitySystemComponent);
	}
}

void ARamdomItemDefenseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		ApplyDefaultStats();
	}

	if (InventoryComponent)
	{
		InventoryComponent->Initialize(AbilitySystemComponent);
	}
}

void ARamdomItemDefenseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARamdomItemDefenseCharacter, ManualTarget);
}

/** (���� ����) ���� ��ȭ ���� �Լ� (BaseValue ���� ���) */
void ARamdomItemDefenseCharacter::ApplyStatUpgrade(EItemStatType StatType, int32 NewLevel)
{
	// ���������� �����ϰ� AttributeSet�� ��ȿ���� Ȯ��
	if (!HasAuthority() || !AttributeSet)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		if (HasAuthority()) RID_LOG(FColor::Red, TEXT("ApplyStatUpgrade Error: Not Server or AttributeSet is NULL"));
		// -----------------------------------------
		return;
	}

	// --- [�ڵ� ����] BaseValue ���� ���� ---

	// 1. �̹� ��ȭ �ܰ�� ���� �߰��Ǿ�� �� *������(Delta)* ���
	//    (NewLevel�� 1���� ����)
	float DeltaValue = 0.0f;
	switch (StatType)
	{
		// ����: ������ ������ ���� (�� ������ ���ϴ� ��� �����ϼ���)
	case EItemStatType::AttackDamage: 			DeltaValue = 10.0f; break; // ������ +10
	case EItemStatType::AttackSpeed: 			DeltaValue = 0.05f; break; // ������ +5%
	case EItemStatType::CritDamage: 			DeltaValue = 0.1f; break;  // ������ +10%
	case EItemStatType::ArmorReduction: 		DeltaValue = 10.0f; break;  // ������ +10
	case EItemStatType::SkillActivationChance: 	DeltaValue = 0.05f; break; // ������ +5%
	default:
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("ApplyStatUpgrade Error: Invalid StatType for BaseValue modification: %s"), *UEnum::GetValueAsString(StatType));
		// -----------------------------------------
		return; // ��� ��ȭ �Ұ��� �����̸� ����
	}

	// 2. AttributeSet�� �ش� ���� BaseValue ���� �Լ� ȣ��
	switch (StatType)
	{
	case EItemStatType::AttackDamage: 			AttributeSet->AdjustBaseAttackDamage(DeltaValue); break;
	case EItemStatType::AttackSpeed: 			AttributeSet->AdjustBaseAttackSpeed(DeltaValue); break;
	case EItemStatType::CritDamage: 			AttributeSet->AdjustBaseCritDamage(DeltaValue); break;
	case EItemStatType::ArmorReduction: 		AttributeSet->AdjustBaseArmorReduction(DeltaValue); break;
	case EItemStatType::SkillActivationChance: 	AttributeSet->AdjustBaseSkillActivationChance(DeltaValue); break;
		// default�� ������ ó�������Ƿ� �ʿ� ����
	}

	// 3. �α� ��� (���� Ȯ��)
	// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
	FString StatName = UEnum::GetValueAsString(StatType);
	RID_LOG(FColor::Cyan, TEXT("Applied BaseValue Upgrade: %s (Level %d, Delta: %.2f)"), *StatName, NewLevel, DeltaValue);
	// -----------------------------------------

	// GameplayEffect ���� ������ ��� ���ŵ�
	// --------------------------------------
}

UAnimMontage* ARamdomItemDefenseCharacter::GetRandomAttackMontage() const
{
	// 1. �迭�� ��Ÿ�ְ� �ϳ��� �ִ��� Ȯ���մϴ�.
	if (DefaultAttackMontages.Num() == 0)
	{
		// ��Ÿ�ְ� ������ �α׸� ����� null�� ��ȯ�մϴ�.
		RID_LOG(FColor::Red, TEXT("Character: DefaultAttackMontages �迭�� ����ֽ��ϴ�!"));
		return nullptr;
	}

	// 2. 0���� (�迭 ũ�� - 1) ������ ���� �ε����� �����մϴ�.
	// (��: 3���� ��� 0, 1, 2 �� �ϳ� ����)
	const int32 RandomIndex = FMath::RandRange(0, DefaultAttackMontages.Num() - 1);

	// 3. �ش� �ε����� ��Ÿ�ָ� ��ȯ�մϴ�.
	return DefaultAttackMontages[RandomIndex];
}