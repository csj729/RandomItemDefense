// Copyright Epic Games, Inc. All Rights Reserved.

#include "RamdomItemDefenseCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "MyPlayerState.h"
#include "MyAttributeSet.h"
#include "MonsterBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplayTagsModule.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "Engine/Engine.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/World.h"
#include "Navigation/PathFollowingComponent.h"

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

	// Create ASC, AS
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComp"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSet"));

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

	// ���������� Ÿ�̸Ӹ� �����ϵ��� �մϴ�.
	if (HasAuthority())
	{
		// AttributeSet�� ��ȿ���� Ȯ���մϴ�.
		if (AttributeSet)
		{
			// AttributeSet���� ���� ���� �ӵ� ���� �����ɴϴ�.
			const float CurrentAttackSpeed = AttributeSet->GetAttackSpeed();
			const float TimerInterval = CurrentAttackSpeed > 0 ? 1.0f / CurrentAttackSpeed : 1.0f;

			GetWorld()->GetTimerManager().SetTimer(FindTargetTimerHandle, this, &ARamdomItemDefenseCharacter::FindTarget, 0.1f, true);
			GetWorld()->GetTimerManager().SetTimer(PerformAttackTimerHandle, this, &ARamdomItemDefenseCharacter::PerformAttack, TimerInterval, true);
		}
	}
}

// ĳ���Ͱ� AbilitySystemComponent�� ��ȯ
UAbilitySystemComponent* ARamdomItemDefenseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// ĳ������ �ʱ� ������ �����մϴ�.
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

// �������� ��Ʈ�ѷ��� �� ĳ���Ϳ� ����(Possess)���� �� ȣ��˴ϴ�.
void ARamdomItemDefenseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		// ASC�� Owner�� Avatar�� ��� �� ĳ���� �ڽ����� �����մϴ�.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		ApplyDefaultStats();

		if (DefaultAttackAbility != nullptr)
		{
			// �����Ƽ�� �ο��մϴ�.
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DefaultAttackAbility));
		}
	}
}

// Ŭ���̾�Ʈ���� PlayerState�� ���� �Ϸ�Ǿ��� �� ȣ��˴ϴ�.
void ARamdomItemDefenseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		// ASC�� Owner�� Avatar�� ��� �� ĳ���� �ڽ����� �����մϴ�.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Ŭ���̾�Ʈ������ ������ �����Ͽ� UI ���� �ʱ�ȭ�� �� �ֽ��ϴ�.
		ApplyDefaultStats();
	}
}

// ManualTarget ������ ���� ��Ͽ� �߰��մϴ�.
void ARamdomItemDefenseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARamdomItemDefenseCharacter, ManualTarget);
}

void ARamdomItemDefenseCharacter::SetManualTarget(AActor* NewTarget)
{
	// ���� Ÿ���� �����Ǹ� �ٸ� Ÿ�ٵ��� ��� �ʱ�ȭ�մϴ�.
	ManualTarget = NewTarget;
	PendingManualTarget = nullptr;
	AutoTarget = nullptr;
}

void ARamdomItemDefenseCharacter::SetPendingManualTarget(AActor* NewTarget)
{
	// ���� ���� Ÿ���� �����Ǹ� ���� ���� Ÿ���� �ʱ�ȭ�մϴ�.
	PendingManualTarget = NewTarget;
	ManualTarget = nullptr;
	AutoTarget = nullptr;
}

void ARamdomItemDefenseCharacter::ClearAllTargets()
{
	ManualTarget = nullptr;
	PendingManualTarget = nullptr;
	AutoTarget = nullptr;
}

void ARamdomItemDefenseCharacter::OrderAttack(AActor* Target)
{
	if (!Target || !AttributeSet || !HasAuthority())
	{
		return;
	}

	AController* MyController = GetController();
	if (!MyController)
	{
		return;
	}

	// �ܼ��� ���� Ÿ������ �����մϴ�. Ÿ�̸ӿ� ���� ����Ǵ� PerformAttack �Լ���
	// ��ǥ�� ��Ÿ� �ۿ� ���� ��� �ڵ����� �̵� ó���� ���� ���Դϴ�.
	// �� ����� ������ �ܼ�ȭ�ϰ� AIController ���� ������Ʈ �������� �����մϴ�.
	if (GEngine)
	{
		FString Msg = FString::Printf(TEXT("OrderAttack received for Target(%s). Setting as ManualTarget."), *Target->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, Msg);
	}
	SetManualTarget(Target);

	// ���ݰ� ���� ���� ������ �ٸ� �̵��� ��� ����ϴ�.
	MyController->StopMovement();
}

void ARamdomItemDefenseCharacter::FindTarget()
{
	if (ManualTarget || PendingManualTarget)
	{
		AutoTarget = nullptr;
		return;
	}

	if (!AttributeSet) return;

	const float CurrentAttackRange = AttributeSet->GetAttackRange();

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		this,
		GetActorLocation(),
		CurrentAttackRange,
		ObjectTypes,
		AMonsterBaseCharacter::StaticClass(),
		{},
		OverlappedActors
	);

	AActor* ClosestActor = nullptr;
	float MinDistance = TNumericLimits<float>::Max();

	for (AActor* Actor : OverlappedActors)
	{
		float Distance = GetDistanceTo(Actor);
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			ClosestActor = Actor;
		}
	}
	AutoTarget = ClosestActor;
}

void ARamdomItemDefenseCharacter::PerformAttack()
{
	if (!HasAuthority()) return;

	AActor* TargetToAttack = ManualTarget ? ManualTarget : AutoTarget;

	if (TargetToAttack)
	{
		if (!AttributeSet)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ERROR: AttributeSet is NULL!"));
			return;
		}

		const float CurrentAttackRange = AttributeSet->GetAttackRange();
		const float DistanceToTarget = GetDistanceTo(TargetToAttack);

		AController* MyController = GetController();
		if (!MyController) return;

		// A. Ÿ���� ���� ������ ����� ���
		if (DistanceToTarget > CurrentAttackRange)
		{
			// ���� Ÿ���� ��쿡�� �Ѿư��ϴ�. �ڵ� Ÿ���� ������ ����� �����ϴ�.
			if (TargetToAttack == ManualTarget)
			{
				const float AttackRangeBuffer = 50.0f;
				const float TargetDistance = FMath::Max(0.0f, CurrentAttackRange - AttackRangeBuffer);
				const FVector MyLocation = GetActorLocation();
				const FVector TargetLocation = TargetToAttack->GetActorLocation();
				const FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
				const FVector Destination = TargetLocation - (Direction * TargetDistance);
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(MyController, Destination);
			}
			else
			{
				AutoTarget = nullptr;
			}
			return;
		}
		// B. Ÿ���� ���� ���� �ȿ� ���� ��� -> "���� �� ����"
		else
		{
			MyController->StopMovement();

			if (GetVelocity().Size() > 1.0f)
			{
				return;
			}

			// --- ������ �κ� ---
			// ���ڸ����� Ÿ���� ��� �ٶ󺸵��� ĳ������ ȸ���� ���� �����մϴ�.
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetToAttack->GetActorLocation());
			// Yaw ���� �����Ͽ� ĳ���Ͱ� �������� ���� �����մϴ�.
			SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));
			// --- ���� �� ---

			if (AbilitySystemComponent)
			{
				FGameplayEventData Payload;
				Payload.Target = TargetToAttack;
				FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.Perform"));
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, Payload);

				if (GEngine)
				{
					FString DebugMes = FString::Printf(TEXT("Attacking -> %s"), *TargetToAttack->GetName());
					GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, DebugMes);
				}
			}
		}
	}
}