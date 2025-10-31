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
#include "RamdomItemDefense.h" // RID_LOG 매크로용

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

		// AbilitySystemComponent가 유효하다면, 공격 속성 변경에 대한 델리게이트를 바인딩합니다.
		if (AbilitySystemComponent)
		{
			// UMyAttributeSet 클래스에 GetAttackSpeedAttribute() static 함수가 구현되어 있어야 합니다. (ATTRIBUTE_ACCESSORS 매크로가 처리)
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UAttackComponent::OnAttackSpeedChanged);
		}
	}

	// 서버에서만 타이머를 설정하도록 합니다.
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

	// 타이머 재설정을 서버에서만 수행합니다.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 기존 타이머를 제거합니다.
		GetWorld()->GetTimerManager().ClearTimer(PerformAttackTimerHandle);

		// 새로운 공격 속도로 타이머를 다시 설정합니다.
		if (NewAttackSpeed > 0.f)
		{
			const float NewInterval = 1.0f / NewAttackSpeed;
			GetWorld()->GetTimerManager().SetTimer(PerformAttackTimerHandle, this, &UAttackComponent::PerformAttack, NewInterval, true);
		}

		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		RID_LOG(FColor::Green, TEXT("AttackSpeed changed to: %.2f. Timer rescheduled."), NewAttackSpeed);
		// -----------------------------------------
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

	// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
	RID_LOG(FColor::Cyan, TEXT("OrderAttack received for Target(%s). Setting as ManualTarget."), *Target->GetName());
	// -----------------------------------------

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
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(Actor);
		if (Monster && Monster->IsDying())
		{
			continue; // 죽어가는 몬스터는 타겟 목록에서 제외합니다.
		}

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
		// 공격 직전, 타겟이 몬스터이고 죽어가는 상태인지 마지막으로 확인합니다.
		AMonsterBaseCharacter* TargetMonster = Cast<AMonsterBaseCharacter>(TargetToAttack);
		if (TargetMonster && TargetMonster->IsDying())
		{
			// 타겟이 죽었다면, 더 이상 공격하지 않도록 타겟을 해제합니다.
			if (TargetToAttack == ManualTarget)
			{
				ManualTarget = nullptr;
			}
			AutoTarget = nullptr;
			return; // 공격 로직을 즉시 중단합니다.
		}

		if (!AttributeSet)
		{
			// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
			RID_LOG(FColor::Red, TEXT("ERROR: AttributeSet is NULL!"));
			// -----------------------------------------
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

			// 1. 캐릭터에서 '단일' 몽타주가 아닌 '랜덤' 몽타주를 가져옵니다.
			UAnimMontage* MontageToPlay = OwnerCharacter->GetRandomAttackMontage();

			if (MontageToPlay)
			{
				// 2. 서버에서 몽타주를 재생합니다.
				OwnerCharacter->PlayAnimMontage(MontageToPlay);
			}

			if (AbilitySystemComponent)
			{
				FGameplayEventData Payload;
				Payload.Target = TargetToAttack;
				FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.Perform"));
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerCharacter, EventTag, Payload);

				// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
				//RID_LOG(FColor::Yellow, TEXT("Attacking -> %s"), *TargetToAttack->GetName());
				// -----------------------------------------
			}
		}
	}
}