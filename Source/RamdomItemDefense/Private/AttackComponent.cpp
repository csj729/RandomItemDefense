// Copyright Epic Games, Inc. All Rights Reserved.

#include "AttackComponent.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "MonsterBaseCharacter.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/Engine.h"

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AbilitySystemComponent = OwnerCharacter->GetAbilitySystemComponent();
		AttributeSet = OwnerCharacter->GetAttributeSet();

		// AbilitySystemComponent�� ��ȿ�ϴٸ�, ���� �Ӽ� ���濡 ���� ��������Ʈ�� ���ε��մϴ�.
		if (AbilitySystemComponent)
		{
			// UMyAttributeSet Ŭ������ GetAttackSpeedAttribute() static �Լ��� �����Ǿ� �־�� �մϴ�. (ATTRIBUTE_ACCESSORS ��ũ�ΰ� ó��)
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UAttackComponent::OnAttackSpeedChanged);
		}
	}

	// ���������� Ÿ�̸Ӹ� �����ϵ��� �մϴ�.
	if (GetOwner()->HasAuthority() && AttributeSet)
	{
		const float CurrentAttackSpeed = AttributeSet->GetAttackSpeed();
		const float TimerInterval = CurrentAttackSpeed > 0 ? 1.0f / CurrentAttackSpeed : 1.0f;

		GetWorld()->GetTimerManager().SetTimer(FindTargetTimerHandle, this, &UAttackComponent::FindTarget, 0.1f, true);
		GetWorld()->GetTimerManager().SetTimer(PerformAttackTimerHandle, this, &UAttackComponent::PerformAttack, TimerInterval, true);
	}
}

void UAttackComponent::OnAttackSpeedChanged(const FOnAttributeChangeData& Data)
{
	const float NewAttackSpeed = Data.NewValue;

	// Ÿ�̸� �缳���� ���������� �����մϴ�.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// ���� Ÿ�̸Ӹ� �����մϴ�.
		GetWorld()->GetTimerManager().ClearTimer(PerformAttackTimerHandle);

		// ���ο� ���� �ӵ��� Ÿ�̸Ӹ� �ٽ� �����մϴ�.
		if (NewAttackSpeed > 0.f)
		{
			const float NewInterval = 1.0f / NewAttackSpeed;
			GetWorld()->GetTimerManager().SetTimer(PerformAttackTimerHandle, this, &UAttackComponent::PerformAttack, NewInterval, true);
		}

		if (GEngine)
		{
			FString Msg = FString::Printf(TEXT("AttackSpeed changed to: %.2f. Timer rescheduled."), NewAttackSpeed);
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, Msg);
		}
	}
}

void UAttackComponent::OrderAttack(AActor* Target)
{
	if (!Target || !OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	AController* MyController = OwnerCharacter->GetController();
	if (!MyController)
	{
		return;
	}

	if (GEngine)
	{
		FString Msg = FString::Printf(TEXT("OrderAttack received for Target(%s). Setting as ManualTarget."), *Target->GetName());
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, Msg);
	}

	ManualTarget = Target;
	AutoTarget = nullptr;
	PendingManualTarget = nullptr;

	MyController->StopMovement();
}

void UAttackComponent::ClearAllTargets()
{
	ManualTarget = nullptr;
	AutoTarget = nullptr;
	PendingManualTarget = nullptr;
}

void UAttackComponent::FindTarget()
{
	if (ManualTarget || PendingManualTarget || !OwnerCharacter)
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
		GetOwner(),
		OwnerCharacter->GetActorLocation(),
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
		float Distance = OwnerCharacter->GetDistanceTo(Actor);
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			ClosestActor = Actor;
		}
	}
	AutoTarget = ClosestActor;
}

void UAttackComponent::PerformAttack()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority()) return;

	AActor* TargetToAttack = ManualTarget ? ManualTarget : AutoTarget;

	if (TargetToAttack)
	{
		if (!AttributeSet)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("ERROR: AttributeSet is NULL!"));
			return;
		}

		const float CurrentAttackRange = AttributeSet->GetAttackRange();
		const float DistanceToTarget = OwnerCharacter->GetDistanceTo(TargetToAttack);

		AController* MyController = OwnerCharacter->GetController();
		if (!MyController) return;

		if (DistanceToTarget > CurrentAttackRange)
		{
			if (TargetToAttack == ManualTarget)
			{
				const float AttackRangeBuffer = 50.0f;
				const float TargetDistance = FMath::Max(0.0f, CurrentAttackRange - AttackRangeBuffer);
				const FVector MyLocation = OwnerCharacter->GetActorLocation();
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
		else
		{
			MyController->StopMovement();

			if (OwnerCharacter->GetVelocity().Size() > 1.0f)
			{
				return;
			}

			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(OwnerCharacter->GetActorLocation(), TargetToAttack->GetActorLocation());
			OwnerCharacter->SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));

			if (AbilitySystemComponent)
			{
				FGameplayEventData Payload;
				Payload.Target = TargetToAttack;
				FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.Perform"));
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, EventTag, Payload);

				if (GEngine)
				{
					FString DebugMes = FString::Printf(TEXT("Attacking -> %s"), *TargetToAttack->GetName());
					GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, DebugMes);
				}
			}
		}
	}
}

