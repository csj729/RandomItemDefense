#include "GA_Archer_ArrowShower.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "RamdomItemDefense.h"	
#include "MyAttributeSet.h"
#include "RID_DamageStatics.h"


UGA_Archer_ArrowShower::UGA_Archer_ArrowShower()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.1f;
	Radius = 400.0f;

	Duration = 3.0f;
	TickInterval = 0.2f; 

	DamageBase = 20.0f; 
	DamageCoefficient = 0.1f;
}

void UGA_Archer_ArrowShower::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo) || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageEffectClass)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_ArrowShower: [ACTIVATE FAILED] DamageEffectClass is not set in BP!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageByCallerTag.IsValid())
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_ArrowShower: [ACTIVATE FAILED] DamageByCallerTag IS NOT SET in BP! (Should be 'Skill.Damage.Value')"));
	}
	else if (DamageByCallerTag != FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value")))
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_Archer_ArrowShower: [WARNING] DamageByCallerTag is '%s'. (Error log expects 'Skill.Damage.Value')"), *DamageByCallerTag.ToString());
	}


	// 1. 타겟 위치 저장 및 파티클 스폰
	TargetLocation = TriggerEventData->Target.Get()->GetActorLocation();
	if (ArrowShowerEffect)
	{
		// 1. 스케일 값 계산
		float EmitterScaleFactor = Radius / 400.f;

		// X, Y, Z 모두 동일한 비율로 스케일 적용
		const FVector EmitterScale = FVector(EmitterScaleFactor);

		// 2. 스케일 값을 적용하여 파티클 스폰
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ArrowShowerEffect,
			TargetLocation,
			FRotator::ZeroRotator,
			EmitterScale, // <-- 계산된 스케일 적용
			true
		);
	}

	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_ArrowShower: [ACTIVATED] Starting 3-sec tick. (GE: %s)"), *GetNameSafe(DamageEffectClass));

	// 2. 틱 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		EffectTickTimerHandle,
		this,
		&UGA_Archer_ArrowShower::OnEffectTick,
		TickInterval,
		true // true: 반복
	);

	// 3. 종료 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		EffectDurationTimerHandle,
		this,
		&UGA_Archer_ArrowShower::OnEffectEnd,
		Duration,
		false // false: 반복 안 함
	);
}

/** 0.2초마다 호출되는 틱 함수 (SetByCaller 로직으로 수정) */
void UGA_Archer_ArrowShower::OnEffectTick()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return; // 서버가 아니면 중지
	}

	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();

	const UMyAttributeSet* AttributeSet = CasterASC ? Cast<const UMyAttributeSet>(CasterASC->GetAttributeSet(UMyAttributeSet::StaticClass())) : nullptr;

	// SlowEffectClass도 확인
	if (!CasterASC || !DamageEffectClass || !SlowEffectClass || !AttributeSet)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_ArrowShower: [TICK FAILED] CasterASC, DamageEffectClass, SlowEffectClass, or AttributeSet is Null."));
		return;
	}

	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(GetCurrentAbilitySpecHandle(), ActorInfo);

	DrawDebugSphere(GetWorld(), TargetLocation, Radius, 12, FColor::Green, false, TickInterval);

	// --- [ ★★★ 데미지 계산 로직 (부모 변수 사용) ★★★ ] ---

	// 1. 틱당 데미지를 *한 번만* 계산합니다. (부모의 DamageBase, DamageCoefficient 사용)
	const float CasterAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (CasterAttackDamage * DamageCoefficient);

	// 2. 틱마다 치명타를 굴릴지, 아니면 한 번만 굴릴지 결정 (여기선 틱마다 굴림)
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(CasterASC, true); // true = 스킬 공격
	float FinalDamage = BaseDamage;
	if (bDidCrit)
	{
		FinalDamage = BaseDamage * URID_DamageStatics::GetCritMultiplier(CasterASC);
	}

	if (bDidCrit)
	{
		UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_ArrowShower: [TICK] Crit! Damage: %.1f"), FinalDamage);
	}

	// 3. 데미지 GE Spec을 *한 번만* 생성합니다.
	FGameplayEffectSpecHandle DamageSpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
	if (!DamageSpecHandle.IsValid())
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_ArrowShower: [TICK FAILED] MakeOutgoingSpec failed."));
		return;
	}

	// 4. Spec에 틱당 데미지 값을 설정합니다.
	DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);

	// --- [ ★★★ 데미지 계산 로직 끝 ★★★ ] ---


	// 5. 범위 내 몬스터를 찾습니다.
	TArray<AActor*> CurrentOverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), TargetLocation, Radius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, CurrentOverlappedActors);

	if (CurrentOverlappedActors.Num() == 0)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_Archer_ArrowShower: [TICK] 0 monsters found in range."));
	}

	// 6. [효과 적용] 범위 내 모든 몬스터에게 미리 계산된 *Spec*을 적용합니다.
	UGameplayEffect* SlowEffectToApply = SlowEffectClass->GetDefaultObject<UGameplayEffect>();
	if (!SlowEffectToApply)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_ArrowShower: [TICK FAILED] SlowEffectClass's DefaultObject is Null."));
		return;
	}

	// 6. [효과 적용] 범위 내 모든 몬스터에게 (1)데미지 Spec과 (2)슬로우 GE를 적용
	for (AActor* TargetActor : CurrentOverlappedActors)
	{
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
		if (Monster && !Monster->IsDying())
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
			if (TargetASC)
			{
				UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_ArrowShower: [TICK] >> Applying Effects to %s..."), *Monster->GetName());

				// 1. 데미지 Spec 적용
				CasterASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);

				// 2. 슬로우 GE 적용 (짧은 지속시간으로 갱신)
				TargetASC->ApplyGameplayEffectToSelf(SlowEffectToApply, 1.0f, ContextHandle);

				// 3. 치명타 텍스트 수동 띄우기
				if (bDidCrit)
				{
					URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
				}
			}
		}
	}
}

/** 3초가 지나 장판이 종료될 때 호출되는 함수 */
void UGA_Archer_ArrowShower::OnEffectEnd()
{
	// 1. 틱 타이머를 중지시킵니다.
	GetWorld()->GetTimerManager().ClearTimer(EffectTickTimerHandle);

	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_ArrowShower: [END] 3-sec duration finished."));

	// 2. 어빌리티를 "성공"으로 정상 종료합니다.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

/** 어빌리티가 (취소 등으로) 비정상 종료될 경우를 대비한 정리 함수 */
void UGA_Archer_ArrowShower::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 1. 타이머가 돌고 있다면 모두 중지
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EffectTickTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(EffectDurationTimerHandle);
	}

	// 2. (필수) 부모의 EndAbility를 마지막에 호출
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}