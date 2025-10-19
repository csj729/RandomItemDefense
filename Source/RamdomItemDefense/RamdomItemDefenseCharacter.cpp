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

	// 서버에서만 타이머를 설정하도록 합니다.
	if (HasAuthority())
	{
		// AttributeSet이 유효한지 확인합니다.
		if (AttributeSet)
		{
			// AttributeSet에서 직접 공격 속도 값을 가져옵니다.
			const float CurrentAttackSpeed = AttributeSet->GetAttackSpeed();
			const float TimerInterval = CurrentAttackSpeed > 0 ? 1.0f / CurrentAttackSpeed : 1.0f;

			GetWorld()->GetTimerManager().SetTimer(FindTargetTimerHandle, this, &ARamdomItemDefenseCharacter::FindTarget, 0.1f, true);
			GetWorld()->GetTimerManager().SetTimer(PerformAttackTimerHandle, this, &ARamdomItemDefenseCharacter::PerformAttack, TimerInterval, true);
		}
	}
}

// 캐릭터가 AbilitySystemComponent를 반환
UAbilitySystemComponent* ARamdomItemDefenseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// 캐릭터의 초기 스탯을 적용합니다.
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

// 서버에서 컨트롤러가 이 캐릭터에 빙의(Possess)했을 때 호출됩니다.
void ARamdomItemDefenseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		// ASC의 Owner와 Avatar를 모두 이 캐릭터 자신으로 설정합니다.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		ApplyDefaultStats();

		if (DefaultAttackAbility != nullptr)
		{
			// 어빌리티를 부여합니다.
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DefaultAttackAbility));
		}
	}
}

// 클라이언트에서 PlayerState가 복제 완료되었을 때 호출됩니다.
void ARamdomItemDefenseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		// ASC의 Owner와 Avatar를 모두 이 캐릭터 자신으로 설정합니다.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 클라이언트에서도 스탯을 적용하여 UI 등을 초기화할 수 있습니다.
		ApplyDefaultStats();
	}
}

// ManualTarget 변수를 복제 목록에 추가합니다.
void ARamdomItemDefenseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARamdomItemDefenseCharacter, ManualTarget);
}

void ARamdomItemDefenseCharacter::SetManualTarget(AActor* NewTarget)
{
	// 수동 타겟이 설정되면 다른 타겟들은 모두 초기화합니다.
	ManualTarget = NewTarget;
	PendingManualTarget = nullptr;
	AutoTarget = nullptr;
}

void ARamdomItemDefenseCharacter::SetPendingManualTarget(AActor* NewTarget)
{
	// 공격 예정 타겟이 설정되면 현재 공격 타겟은 초기화합니다.
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

	// 단순히 수동 타겟으로 지정합니다. 타이머에 의해 실행되는 PerformAttack 함수가
	// 목표가 사거리 밖에 있을 경우 자동으로 이동 처리를 해줄 것입니다.
	// 이 방식은 로직을 단순화하고 AIController 관련 컴포넌트 의존성을 제거합니다.
	if (GEngine)
	{
		FString Msg = FString::Printf(TEXT("OrderAttack received for Target(%s). Setting as ManualTarget."), *Target->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, Msg);
	}
	SetManualTarget(Target);

	// 공격과 관련 없는 현재의 다른 이동은 즉시 멈춥니다.
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

		// A. 타겟이 공격 범위를 벗어났을 경우
		if (DistanceToTarget > CurrentAttackRange)
		{
			// 수동 타겟일 경우에만 쫓아갑니다. 자동 타겟은 범위를 벗어나면 버립니다.
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
		// B. 타겟이 공격 범위 안에 있을 경우 -> "정지 후 공격"
		else
		{
			MyController->StopMovement();

			if (GetVelocity().Size() > 1.0f)
			{
				return;
			}

			// --- 수정된 부분 ---
			// 제자리에서 타겟을 즉시 바라보도록 캐릭터의 회전을 직접 설정합니다.
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetToAttack->GetActorLocation());
			// Yaw 값만 변경하여 캐릭터가 기울어지는 것을 방지합니다.
			SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));
			// --- 수정 끝 ---

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