// Source/RamdomItemDefense/Private/AttackComponent.cpp

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
#include "MyPlayerState.h"
#include "RamdomItemDefense.h" 

UAttackComponent::UAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	OwnerCharacter = nullptr;

	SetIsReplicatedByDefault(true);
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		UE_LOG(LogRamdomItemDefense, Log, TEXT("AttackComponent [%s]: Initializing for CHARACTER."), *GetNameSafe(GetOwner()));
		Initialize(OwnerCharacter->GetAbilitySystemComponent(), OwnerCharacter->GetAttributeSet());
	}
}

void UAttackComponent::Initialize(UAbilitySystemComponent* InASC, const UMyAttributeSet* InAttributeSet)
{
	AbilitySystemComponent = InASC;
	AttributeSet = InAttributeSet;

	UE_LOG(LogRamdomItemDefense, Log, TEXT("AttackComponent [%s]: Initialize called."), *GetNameSafe(GetOwner()));

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UAttackComponent::OnAttackSpeedChanged);
	}

	if (GetOwner() && GetOwner()->HasAuthority() && AttributeSet)
	{
		const float CurrentAttackSpeed = AttributeSet->GetAttackSpeed();
		const float TimerInterval = CurrentAttackSpeed > 0 ? 1.0f / CurrentAttackSpeed : 1.0f;

		UE_LOG(LogRamdomItemDefense, Log, TEXT("AttackComponent [%s]: Starting timers. AtkSpeed: %.2f (Interval: %.2fs)"), *GetNameSafe(GetOwner()), CurrentAttackSpeed, TimerInterval);

		GetWorld()->GetTimerManager().SetTimer(FindTargetTimerHandle, this, &UAttackComponent::FindTarget, 0.1f, true);
		GetWorld()->GetTimerManager().SetTimer(PerformAttackTimerHandle, this, &UAttackComponent::PerformAttack, TimerInterval, true);
	}
	else
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("AttackComponent [%s]: FAILED to start timers. (IsAuthority: %d, HasAttributeSet: %d)"),
			*GetNameSafe(GetOwner()),
			GetOwner() ? GetOwner()->HasAuthority() : -1,
			AttributeSet != nullptr
		);
	}
}

void UAttackComponent::OnAttackSpeedChanged(const FOnAttributeChangeData& Data)
{
	const float NewAttackSpeed = Data.NewValue;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().ClearTimer(PerformAttackTimerHandle);

		if (NewAttackSpeed > 0.f)
		{
			const float NewInterval = 1.0f / NewAttackSpeed;
			GetWorld()->GetTimerManager().SetTimer(PerformAttackTimerHandle, this, &UAttackComponent::PerformAttack, NewInterval, true);
		}

		UE_LOG(LogRamdomItemDefense, Log, TEXT("AttackComponent [%s]: AttackSpeed changed to: %.2f. Timer rescheduled."), *GetNameSafe(GetOwner()), NewAttackSpeed);
	}
}

void UAttackComponent::OrderAttack_Implementation(AActor* Target)
{
	if (!Target) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* MyController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	UE_LOG(LogRamdomItemDefense, Log, TEXT("AttackComponent [%s]: OrderAttack received for Target(%s)."), *GetNameSafe(GetOwner()), *GetNameSafe(Target));

	ManualTarget = Target;
	AutoTarget = nullptr;
	PendingManualTarget = nullptr;

	if (MyController)
	{
		MyController->StopMovement();
	}
}

void UAttackComponent::ClearAllTargets()
{
	ManualTarget = nullptr;
	AutoTarget = nullptr;
	PendingManualTarget = nullptr;
}

void UAttackComponent::FindTarget()
{
	if (ManualTarget || PendingManualTarget)
	{
		AutoTarget = nullptr;
		return;
	}

	AActor* MyOwner = GetOwner();
	if (!MyOwner || !AttributeSet)
	{
		return;
	}

	const float CurrentAttackRange = AttributeSet->GetAttackRange();

	if (CurrentAttackRange <= 0.0f)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("AttackComponent [%s]: FindTarget FAILED. CurrentAttackRange is ZERO."), *GetNameSafe(MyOwner));
		AutoTarget = nullptr;
		return;
	}

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetOwner(),
		MyOwner->GetActorLocation(),
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
			continue;
		}

		float Distance = MyOwner->GetDistanceTo(Actor);
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
	AActor* MyOwner = GetOwner();
	if (!MyOwner || !MyOwner->HasAuthority()) return;

	// 궁극기 사용 중이면 공격 중단
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Player.IsUsingUltimate"))))
	{
		return;
	}

	AActor* TargetToAttack = ManualTarget ? ManualTarget : AutoTarget;

	if (TargetToAttack)
	{
		AMonsterBaseCharacter* TargetMonster = Cast<AMonsterBaseCharacter>(TargetToAttack);
		if (TargetMonster && TargetMonster->IsDying())
		{
			if (TargetToAttack == ManualTarget) ManualTarget = nullptr;
			AutoTarget = nullptr;
			return;
		}

		if (!AttributeSet)
		{
			UE_LOG(LogRamdomItemDefense, Error, TEXT("AttackComponent [%s]: PerformAttack FAILED. AttributeSet is NULL!"), *GetNameSafe(MyOwner));
			return;
		}

		const float CurrentAttackRange = AttributeSet->GetAttackRange();
		const float DistanceToTarget = MyOwner->GetDistanceTo(TargetToAttack);

		APawn* OwnerPawn = Cast<APawn>(MyOwner);
		AController* MyController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

		// 1. 사거리 밖이면 이동
		if (DistanceToTarget > CurrentAttackRange)
		{
			if (TargetToAttack == ManualTarget && MyController && OwnerCharacter)
			{
				const float AttackRangeBuffer = 50.0f;
				const float TargetDistance = FMath::Max(0.0f, CurrentAttackRange - AttackRangeBuffer);
				const FVector MyLocation = MyOwner->GetActorLocation();
				const FVector TargetLocation = TargetToAttack->GetActorLocation();
				const FVector Direction = (TargetLocation - MyLocation).GetSafeNormal();
				const FVector Destination = TargetLocation - (Direction * TargetDistance);
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(MyController, Destination);
			}
			else
			{
				// 자동 타겟인데 사거리 밖이면 타겟 해제 (다음 FindTarget에서 다시 찾음)
				AutoTarget = nullptr;
			}
			return;
		}
		// 2. 사거리 안이면 공격 실행
		else
		{
			if (MyController)
			{
				MyController->StopMovement();
			}

			// [리팩토링] 공격 실행 로직 분리 호출
			ExecuteAttackLogic(TargetToAttack);
		}
	}
}

/** [리팩토링] 분리된 공격 실행 함수 */
void UAttackComponent::ExecuteAttackLogic(AActor* TargetActor)
{
	if (!TargetActor || !GetOwner()) return;

	// 1. 캐릭터 회전 및 애니메이션 (시각 효과)
	if (OwnerCharacter)
	{
		if (OwnerCharacter->GetVelocity().Size() > 1.0f)
		{
			return; // 움직이는 중이면 스킵
		}

		// Z축 회전(Yaw)만 적용하여 타겟 바라보기
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(OwnerCharacter->GetActorLocation(), TargetActor->GetActorLocation());
		LookAtRotation.Pitch = 0.0f;
		LookAtRotation.Roll = 0.0f;

		// (서버에서도 회전 적용 - 싱크를 위해)
		OwnerCharacter->SetActorRotation(LookAtRotation);

		UAnimMontage* MontageToPlay = OwnerCharacter->GetRandomAttackMontage();
		if (MontageToPlay)
		{
			OwnerCharacter->Multicast_PlayAttack(MontageToPlay, LookAtRotation);
		}
	}

	// 2. GAS 이벤트 전송 (실제 데미지 로직 트리거)
	if (AbilitySystemComponent)
	{
		UE_LOG(LogRamdomItemDefense, Log, TEXT("AttackComponent [%s]: Sending 'Event.Attack.Perform' to SELF targeting %s"), *GetNameSafe(GetOwner()), *GetNameSafe(TargetActor));

		FGameplayEventData Payload;
		Payload.Target = TargetActor;
		FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.Perform"));

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), EventTag, Payload);

		// 궁극기 게이지 충전
		if (OwnerCharacter)
		{
			AMyPlayerState* PS = OwnerCharacter->GetPlayerState<AMyPlayerState>();
			if (PS)
			{
				PS->AddUltimateCharge(1);
			}
		}
	}
}