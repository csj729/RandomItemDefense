// Source/RamdomItemDefense/Private/GA_MagicFighter_MeteorStrike.cpp (수정)

#include "GA_MagicFighter_MeteorStrike.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MyAttributeSet.h" 
#include "MonsterBaseCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystem.h"
#include "GameplayEffectTypes.h" 
#include "RamdomItemDefense.h" 
// --- [코드 추가] ---
#include "RID_DamageStatics.h" // 데미지 계산 헬퍼 포함
// --- [코드 추가 끝] ---

UGA_MagicFighter_MeteorStrike::UGA_MagicFighter_MeteorStrike()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.1f;
}

void UGA_MagicFighter_MeteorStrike::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	// 서버에서만 로직을 실행합니다.
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. GA_AttackSelecter가 넘겨준 타겟 정보를 가져옵니다.
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);

	if (!TargetActor)
	{
		RID_LOG(FColor::Red, TEXT("GA_MagicFighter_MeteorStrike Error: No Target found!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 폭발 위치(ImpactLocation)와 이펙트 스폰 위치(SpawnLocation) 계산
	ImpactLocation = TargetActor->GetActorLocation(); // 타겟의 현재 위치를 멤버 변수에 저장
	FVector SpawnLocation = ImpactLocation + FVector(0.0f, 0.0f, SpawnHeight);

	// 3. '떨어지는' 이펙트 스폰 (시각 효과)
	if (FallingEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FallingEffect, SpawnLocation, FRotator::ZeroRotator, true);
	}

	// 4. FallDuration(0.5초) 후에 Explode 함수를 호출하도록 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_MagicFighter_MeteorStrike::Explode,
		FallDuration,
		false
	);

	// 어빌리티는 Explode 함수에서 종료
}

/** 타이머가 만료되었을 때 실제 폭발을 실행하는 함수 */
void UGA_MagicFighter_MeteorStrike::Explode()
{
	// 현재 어빌리티 핸들 및 정보 가져오기
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	// 1. 시전자(Caster)의 AbilitySystemComponent를 가져옵니다.
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	if (!CasterASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 시전자의 현재 AttackDamage 값을 가져옵니다.
	float CasterAttackDamage = CasterASC->GetNumericAttribute(UMyAttributeSet::GetAttackDamageAttribute());

	// 3. 기본 데미지 계산 (양수)
	float BaseDamage = 100.0f + (CasterAttackDamage * 0.2f);

	// --- [ ★★★ 범위 스킬 치명타 로직 수정 ★★★ ] ---

	// 3-1. 치명타 굴림을 '단 한 번' 수행합니다. (bIsSkillAttack: true)
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(CasterASC, true);

	// 3-2. 치명타 여부에 따라 '최종 데미지'를 '단 한 번' 계산합니다.
	float FinalDamage = BaseDamage;
	if (bDidCrit)
	{
		FinalDamage = BaseDamage * URID_DamageStatics::GetCritMultiplier(CasterASC);
	}
	// --- [ ★★★ 로직 수정 끝 ★★★ ] ---


	// 4. '폭발' 이펙트 스폰 (시각 효과)
	if (ExplosionEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, ImpactLocation, FRotator::ZeroRotator, true);

	// 5. ImpactLocation에서 500 반경 내 몬스터 찾기
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("Meteor Exploded! Damage: %.1f (Crit: %s), Found %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 6. 찾은 몬스터들에게 GE 적용
	if (OverlappedActors.Num() > 0)
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

		// 6-1. 데미지 GE Spec을 '단 한 번' 생성합니다.
		FGameplayEffectSpecHandle DamageSpecHandle;
		if (DamageEffectClass && DamageByCallerTag.IsValid())
		{
			DamageSpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
			if (DamageSpecHandle.IsValid())
			{
				// 6-2. Spec에 '최종 데미지'를 '단 한 번' 설정합니다.
				DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
			}
		}

		for (AActor* TargetActor : OverlappedActors)
		{
			AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
			if (Monster && !Monster->IsDying())
			{
				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
				if (TargetASC)
				{
					// [적용 1] 데미지: 미리 만들어둔 '동일한 Spec'을 적용합니다.
					if (DamageSpecHandle.IsValid())
					{
						TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
					}

					// [적용 2] 치명타 텍스트: 치명타가 발생했다면, 이 몬스터에게 델리게이트를 방송합니다.
					if (bDidCrit)
					{
						URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
					}

					// [적용 3] 스턴: 스턴은 클래스를 바로 적용합니다.
					if (StunEffectClass)
					{
						TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
					}
				}
			}
		}
	}

	// 7. 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}