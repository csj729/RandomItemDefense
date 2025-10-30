#include "GA_MagicFighter_MeteorStrike.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MyAttributeSet.h" // 시전자의 스탯을 가져오기 위해 AttributeSet 포함
#include "MonsterBaseCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystem.h"
#include "GameplayEffectTypes.h" // FGameplayEffectSpecHandle 사용
#include "RamdomItemDefense.h" // RID_LOG

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

	// --- [데미지 계산] ---
	// 1. 시전자(Caster)의 AbilitySystemComponent를 가져옵니다.
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	if (!CasterASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 시전자의 현재 AttackDamage 값을 가져옵니다. (MyAttributeSet.h에 GetAttackDamageAttribute()가 정의되어 있어야 함)
	float CasterAttackDamage = CasterASC->GetNumericAttribute(UMyAttributeSet::GetAttackDamageAttribute());

	// 3. 최종 데미지 계산 (기본 100 + 공격력*0.2)
	// Health는 Add로 적용할 때 음수여야 하므로, 결과값에 -1을 곱합니다.
	float TotalDamage = -(100.0f + (CasterAttackDamage * 0.2f));
	// ---------------------

	// 4. '폭발' 이펙트 스폰 (시각 효과)
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, ImpactLocation, FRotator::ZeroRotator, true);
	}

	// 5. ImpactLocation에서 500 반경 내 몬스터 찾기
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		ImpactLocation, // 저장해둔 폭발 지점
		ExplosionRadius,
		ObjectTypes,
		AMonsterBaseCharacter::StaticClass(),
		TArray<AActor*>(),
		OverlappedActors
	);

	RID_LOG(FColor::Cyan, TEXT("Meteor Exploded (Damage: %.1f). Found %d monsters."), FMath::Abs(TotalDamage), OverlappedActors.Num());

	// 6. 찾은 몬스터들에게 GE 적용
	if (OverlappedActors.Num() > 0)
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

		// --- 데미지 GE 스펙(Spec) 생성 ---
		// 데미지 GE는 SetByCaller를 사용하므로, 미리 'Spec'을 만들어 값을 주입해야 합니다.
		FGameplayEffectSpecHandle DamageSpecHandle;
		if (DamageEffectClass && DamageByCallerTag.IsValid())
		{
			DamageSpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
			if (DamageSpecHandle.IsValid())
			{
				// Spec에 계산된 TotalDamage 값을 DamageByCallerTag와 함께 주입합니다.
				DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, TotalDamage);
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
					// [적용 1] 데미지: 미리 만들어둔 Spec을 적용합니다.
					if (DamageSpecHandle.IsValid())
					{
						TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
					}

					// [적용 2] 스턴: 스턴은 정적 값이므로 클래스를 바로 적용합니다.
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