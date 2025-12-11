// SSource/RamdomItemDefense/Private/GA_MagicFighter_BlackHole.cpp

#include "GA_MagicFighter_BlackHole.h"
#include "RamdomItemDefense.h" // PCH (모든 GAS 헤더 포함)
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"
#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "MyAttributeSet.h"
#include "RID_DamageStatics.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetMathLibrary.h" // FindLookAtRotation을 위해 필요

UGA_MagicFighter_BlackHole::UGA_MagicFighter_BlackHole()
{
	// 블루프린트에서 설정할 기본값들
	DamageRadius = 1500.0f;
	DamageBase = 500.0f;
	DamageCoefficient = 2.0f;
	PullTickInterval = 0.1f; // 초당 10회 틱

	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value"));

	// 상태 변수 초기화
	bMontageFinished = false;
	bPullFinished = false;
}

void UGA_MagicFighter_BlackHole::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 1. (필수!) C++ 부모의 ActivateAbility를 호출합니다.
	// (이 함수가 스택을 리셋하고, 'State.Player.IsUsingUltimate' 태그를 적용합니다)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 2. 시전자(캐릭터)와 PlayerState, 스포너, ASC를 가져옵니다.
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	AMyPlayerState* PS = OwnerCharacter ? OwnerCharacter->GetPlayerState<AMyPlayerState>() : nullptr;
	SourceASC = ActorInfo->AbilitySystemComponent.Get(); // (★★★) 멤버 변수에 ASC 캐시

	if (!OwnerCharacter || !PS || !PS->MySpawner || !SourceASC)
	{
		RID_LOG(FColor::Red, TEXT("GA_MagicFighter_BlackHole: Character, PS, MySpawner or SourceASC is missing."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UltimateStateEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UltimateStateEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 부모의 UltimateStateEffectHandle 변수에 핸들을 저장합니다.
			// (부모의 EndAbility가 이 핸들을 사용해 태그를 제거합니다)
			UltimateStateEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	bMontageFinished = false;
	bPullFinished = false;

	// 3. 블랙홀 위치 = 스포너의 (X, Y) + 시전자의 (Z)
	FVector SpawnerLocation = PS->MySpawner->GetActorLocation();
	CasterLocation = OwnerCharacter->GetActorLocation();
	BlackHoleLocation = FVector(SpawnerLocation.X, SpawnerLocation.Y, CasterLocation.Z);

	// 4. 캐릭터가 블랙홀 위치(0번 방향)를 바라보도록 즉시 회전
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(CasterLocation, BlackHoleLocation);
	OwnerCharacter->SetActorRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));

	// 5. 파티클 스폰 (스케일 수정됨)
	if (BlackHoleEffect)
	{
		OwnerCharacter->Multicast_SpawnParticleAtLocation(
			BlackHoleEffect,
			BlackHoleLocation,
			FRotator::ZeroRotator,
			FVector(2.0f) // 스케일 2.0
		);
	}
	if (CasterEffect && OwnerCharacter->GetMesh()) { UGameplayStatics::SpawnEmitterAttached(CasterEffect, OwnerCharacter->GetMesh(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.5f), EAttachLocation::KeepRelativeOffset, true); }

	// --- 6. (즉시 실행) '총' 데미지 계산 ---
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float TotalBaseDamage = DamageBase + (AttributeSet->GetAttackDamage() * DamageCoefficient);
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(SourceASC, true);
	float TotalFinalDamage = TotalBaseDamage;
	if (bDidCrit)
	{
		TotalFinalDamage = TotalBaseDamage * URID_DamageStatics::GetCritMultiplier(SourceASC);
	}

	// --- 7. (즉시 실행) '틱당' 데미지 Spec 생성 ---
	float Duration = (PullDuration > 0.0f) ? PullDuration : 1.0f;
	float DamagePerTick = (TotalFinalDamage / Duration) * PullTickInterval;

	if (DamageEffectClass && DamageDataTag.IsValid())
	{
		// 틱당 데미지 Spec을 만들어서 멤버 변수(DamageSpecHandle)에 저장
		DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, MakeEffectContext(GetCurrentAbilitySpecHandle(), ActorInfo));
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -DamagePerTick); // 음수로 데미지 적용
	}

	// --- 8. (★★★) (즉시 실행) 광역 스턴 로직 [제거됨] ---
	// 스턴 로직은 이제 PullTick으로 이동했습니다.

	// --- 9. (즉시 실행) 끌어당기기 타이머 시작 ---
	if (PullDuration > 0.0f && PullTickInterval > 0.0f)
	{
		// PullTickInterval 마다 PullTick 함수 호출
		GetWorld()->GetTimerManager().SetTimer(PullTimerHandle, this, &UGA_MagicFighter_BlackHole::PullTick, PullTickInterval, true);
		// PullDuration 후에 StopPullingAndResetAI 함수 호출
		GetWorld()->GetTimerManager().SetTimer(PullDurationTimerHandle, this, &UGA_MagicFighter_BlackHole::StopPullingAndResetAI, PullDuration, false);
	}
	else
	{
		bPullFinished = true; // 지속시간이 0이므로 즉시 종료
	}

	// --- 10. (즉시 실행) 몽타주 재생 태스크 실행 ---
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, UltimateMontage);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_MagicFighter_BlackHole::OnMontageFinished);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_MagicFighter_BlackHole::OnMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_MagicFighter_BlackHole::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_MagicFighter_BlackHole::OnMontageCancelled);

		MontageTask->ReadyForActivation();
	}
	else
	{
		bMontageFinished = true; // 몽타주 없으면 즉시 종료
	}

	CheckEndAbility(); // 둘 다 즉시 종료되었을 경우 대비
}

/** 몽타주가 '중단/취소'되었을 때 (파라미터 없음) */
void UGA_MagicFighter_BlackHole::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}


/** 몽타주 재생이 '성공적으로' 끝났을 때 호출됩니다. */
void UGA_MagicFighter_BlackHole::OnMontageFinished()
{
	bMontageFinished = true;
	CheckEndAbility();
}

/** (★★★) 끌어당기기, 데미지, 스턴 (매 틱 실행) */
void UGA_MagicFighter_BlackHole::PullTick()
{
	if (!SourceASC) return; // 시전자 ASC가 없으면 중지

	float DeltaTime = PullTickInterval; // 타이머 간격만큼의 시간을 사용

	// (★★★) 매 틱마다 새로 몬스터를 검색합니다
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), CasterLocation, DamageRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	for (AActor* TargetActor : OverlappedActors)
	{
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
		if (!Monster || Monster->IsDying()) continue; // (IsActorBeingDestroyed() 대신 IsDying() 사용)

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
		if (TargetASC)
		{
			// (★★★) 1. 틱 데미지 적용
			if (DamageSpecHandle.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
			}

			// (★★★) 2. 틱 스턴 적용
			// 몬스터가 범위 안에 있는 동안 스턴 GE를 계속 적용(갱신)합니다.
			if (StunEffectClass)
			{
				TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, MakeEffectContext(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo()));
			}
		}

		// (★★★) 3. 끌어당기기
		FVector CurrentLocation = Monster->GetActorLocation();
		FVector FlattenedTarget = FVector(BlackHoleLocation.X, BlackHoleLocation.Y, CurrentLocation.Z); // Z축 고정
		FVector TargetLocation = UKismetMathLibrary::VInterpTo(CurrentLocation, FlattenedTarget, DeltaTime, PullInterpSpeed);
		Monster->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::None);
	}
}

/** 끌어당기기 종료 및 AI 리셋 */
void UGA_MagicFighter_BlackHole::StopPullingAndResetAI()
{
	// 1. PullTick 타이머 중지
	GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);

	// 2. (몬스터 위치 보정 및 AI 리셋)
	// (★★★) 틱이 멈추기 직전에 마지막으로 범위 내 몬스터를 다시 검색합니다.
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), CasterLocation, DamageRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	for (AActor* TargetActor : OverlappedActors)
	{
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
		// (IsActorBeingDestroyed() 대신 IsDying() 사용)
		if (Monster && !Monster->IsDying())
		{
			// (★★★) 2a. 위치 보정
			FVector FinalTeleportLocation = FVector(BlackHoleLocation.X, BlackHoleLocation.Y, Monster->GetActorLocation().Z);
			Monster->SetActorLocation(FinalTeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);

			// (★★★) 2b. AI 리셋
			AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(Monster->GetController());
			if (MonsterAI)
			{
				UBlackboardComponent* Blackboard = MonsterAI->GetBlackboardComponent();
				if (Blackboard)
				{
					Blackboard->SetValueAsInt(FName("CurrentSplinePointIndex"), 0);
				}
			}
		}
	}

	// 3. (★★★) "끌어당기기"가 끝났다고 표시
	bPullFinished = true;
	CheckEndAbility();
}

/** 몽타주와 끌어당기기가 모두 끝났는지 확인하고 어빌리티를 종료합니다. */
void UGA_MagicFighter_BlackHole::CheckEndAbility()
{
	// 몽타주가 끝났고 (bMontageFinished == true)
	// 그리고 끌어당기기도 끝났다면 (bPullFinished == true)
	if (bMontageFinished && bPullFinished)
	{
		// (필수!) 어빌리티를 "성공"으로 종료합니다.
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}


void UGA_MagicFighter_BlackHole::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 1. (정리) 혹시라도 타이머가 돌고 있었다면 강제 중지
	GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(PullDurationTimerHandle);

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	// 2. (정리) 이 어빌리티가 적용했던 '궁극기 사용 중' 상태 GE를 제거합니다.
	// (이것을 하지 않으면 AttackComponent가 계속 공격을 차단합니다)
	if (UltimateStateEffectHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(UltimateStateEffectHandle);
	}
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

	// 3. (필수!) C++ 부모의 EndAbility를 호출 (태그 제거)
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}