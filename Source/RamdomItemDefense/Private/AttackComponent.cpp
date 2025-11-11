// Private/AttackComponent.cpp (수정)

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
	OwnerCharacter = nullptr; // nullptr로 명시적 초기화
}

void UAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	// (수정) 오너가 캐릭터인 경우에만 기존 방식대로 자동 초기화
	OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		// 캐릭터의 BeginPlay에서 호출되므로, 캐릭터의 ASC와 AttributeSet을 사용해 즉시 초기화
		Initialize(OwnerCharacter->GetAbilitySystemComponent(), OwnerCharacter->GetAttributeSet());
	}
	// (오너가 드론인 경우, 드론의 BeginPlay에서 수동으로 Initialize()를 호출할 때까지 대기)
}

/**
 * @brief (새 함수) ASC와 AttributeSet을 받아 컴포넌트를 초기화하고 타이머를 시작합니다.
 */
void UAttackComponent::Initialize(UAbilitySystemComponent* InASC, const UMyAttributeSet* InAttributeSet)
{
	// 1. ASC와 AttributeSet 캐시
	AbilitySystemComponent = InASC;
	AttributeSet = InAttributeSet;

	// 2. 델리게이트 바인딩 (기존 BeginPlay 로직)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UAttackComponent::OnAttackSpeedChanged);
	}

	// 3. 타이머 설정 (기존 BeginPlay 로직)
	// (수정) GetOwner() (즉, 드론 또는 캐릭터)의 권한 확인
	if (GetOwner() && GetOwner()->HasAuthority() && AttributeSet)
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

	// (수정) GetOwner()의 권한 확인
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

		RID_LOG(FColor::Green, TEXT("AttackSpeed changed to: %.2f. Timer rescheduled."), NewAttackSpeed);
	}
}

void UAttackComponent::OrderAttack(AActor* Target)
{
	if (!Target || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// (수정) 오너가 Pawn일 때만 Controller 로직 수행
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* MyController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	RID_LOG(FColor::Cyan, TEXT("OrderAttack received for Target(%s). Setting as ManualTarget."), *Target->GetName());

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
	if (!MyOwner || !AttributeSet) return;

	const float CurrentAttackRange = AttributeSet->GetAttackRange();

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetOwner(),
		MyOwner->GetActorLocation(), // OwnerCharacter -> MyOwner
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

		float Distance = MyOwner->GetDistanceTo(Actor); // OwnerCharacter -> MyOwner
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

	// (수정) 드론은 궁극기가 없지만, 코드는 호환성을 위해 유지 (ASC가 태그를 가질 수 있으므로)
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Player.IsUsingUltimate"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Using Ultimate"));
		return;
	}

	AActor* TargetToAttack = ManualTarget ? ManualTarget : AutoTarget;

	if (TargetToAttack)
	{
		// 공격 직전, 타겟이 몬스터이고 죽어가는 상태인지 마지막으로 확인
		AMonsterBaseCharacter* TargetMonster = Cast<AMonsterBaseCharacter>(TargetToAttack);
		if (TargetMonster && TargetMonster->IsDying())
		{
			if (TargetToAttack == ManualTarget) ManualTarget = nullptr;
			AutoTarget = nullptr;
			return;
		}

		if (!AttributeSet)
		{
			RID_LOG(FColor::Red, TEXT("ERROR: AttributeSet is NULL!"));
			return;
		}

		const float CurrentAttackRange = AttributeSet->GetAttackRange();
		const float DistanceToTarget = MyOwner->GetDistanceTo(TargetToAttack); // OwnerCharacter -> MyOwner

		// (수정) 오너가 Pawn일 때만 Controller/Movement 로직 수행
		APawn* OwnerPawn = Cast<APawn>(MyOwner);
		AController* MyController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

		if (DistanceToTarget > CurrentAttackRange)
		{
			// (수정) 수동 타겟이고 + 컨트롤러가 있고 + 오너가 캐릭터일 때만 이동
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
				AutoTarget = nullptr; // 드론은 사거리 밖이면 타겟 해제
			}
			return;
		}
		else
		{
			if (MyController) // 컨트롤러가 있다면 (캐릭터의 경우)
			{
				MyController->StopMovement();
			}

			// (수정) 오너가 캐릭터(OwnerCharacter)일 때만 Velocity/Montage/회전 로직 수행
			if (OwnerCharacter)
			{
				if (OwnerCharacter->GetVelocity().Size() > 1.0f)
				{
					return; // 아직 이동 중이면 공격 안 함
				}

				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(OwnerCharacter->GetActorLocation(), TargetToAttack->GetActorLocation());
				OwnerCharacter->SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));

				UAnimMontage* MontageToPlay = OwnerCharacter->GetRandomAttackMontage();
				if (MontageToPlay)
				{
					OwnerCharacter->PlayAnimMontage(MontageToPlay);
				}
			}
			// (드론은 위 로직을 건너뛰고 바로 이벤트 전송)
			// (드론의 회전은 드론의 Tick에서 이미 처리 중)

			if (AbilitySystemComponent)
			{
				FGameplayEventData Payload;
				Payload.Target = TargetToAttack;
				FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.Perform"));
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MyOwner, EventTag, Payload); // OwnerCharacter -> MyOwner

				// (수정) 오너가 캐릭터(OwnerCharacter)일 때만 궁극기 스택 추가
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
	}
}