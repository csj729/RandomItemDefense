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
		if (DefaultAttackAbility != nullptr)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DefaultAttackAbility));
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

/** (서버 전용) 스탯 강화 적용 함수 */
void ARamdomItemDefenseCharacter::ApplyStatUpgrade(EItemStatType StatType, int32 NewLevel)
{
	if (!HasAuthority() || !AbilitySystemComponent) return;

	// 해당 스탯 타입에 맞는 GE 클래스를 TMap에서 찾습니다.
	TSubclassOf<UGameplayEffect>* EffectClassPtr = UpgradeEffects.Find(StatType);
	if (EffectClassPtr && *EffectClassPtr)
	{
		// 기존에 적용된 같은 레벨의 강화 효과가 있다면 제거해야 할 수 있음 (Stacking 방식에 따라 다름)
		// TODO: Remove Gameplay Effects by Tag Query (강화 효과 식별 태그 필요)

		// 새 레벨에 맞는 GameplayEffect를 적용합니다.
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		// GE의 레벨을 설정하여 적용 (GE 내부에서 레벨에 따른 수치 계산 필요)
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(*EffectClassPtr, NewLevel, ContextHandle);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			if (GEngine)
			{
				FString StatName = UEnum::GetValueAsString(StatType);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Applied Upgrade Effect: %s (Level %d)"), *StatName, NewLevel));
			}
		}
	}
	else if (GEngine)
	{
		FString StatName = UEnum::GetValueAsString(StatType);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("ApplyStatUpgrade Error: No GE found for %s"), *StatName));
	}
}