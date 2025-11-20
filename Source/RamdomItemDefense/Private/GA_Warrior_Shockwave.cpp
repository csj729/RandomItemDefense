// Private/GA_Warrior_Shockwave.cpp

#include "GA_Warrior_Shockwave.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MonsterBaseCharacter.h"
#include "RID_DamageStatics.h"
#include "Kismet/GameplayStatics.h"

UGA_Warrior_Shockwave::UGA_Warrior_Shockwave()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f; // (임의) 기본 발동 확률 5%
	ExplosionRadius = 400.0f;

	// 부모 변수 기본값 설정
	DamageBase = 150.0f;
	DamageCoefficient = 1.0f; // 공격력의 100%
}

void UGA_Warrior_Shockwave::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 서버 확인, 타겟 유효성 검사
	if (!HasAuthority(&ActivationInfo) || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. GE 클래스 유효성 검사
	if (!DamageEffectClass || !StunEffectClass)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Warrior_Shockwave: DamageEffectClass or StunEffectClass is not set in BP!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 시전자 ASC 및 속성셋 가져오기
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(CasterASC->GetAttributeSet(UMyAttributeSet::StaticClass()));

	if (!CasterASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 4. 폭발 중심 위치 (타겟의 현재 위치)
	FVector ImpactLocation = TriggerEventData->Target.Get()->GetActorLocation();
	if (ImpactEffect)
	{
		// [수정]
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
		{
			OwnerCharacter->Multicast_SpawnParticleAtLocation(
				ImpactEffect,
				ImpactLocation,
				FRotator::ZeroRotator,
				FVector(1.0f)
			);
		}
	}

	// 5. 데미지 계산 (1회)
	const float CasterAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (CasterAttackDamage * DamageCoefficient);
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(CasterASC, true);
	float FinalDamage = BaseDamage;
	if (bDidCrit)
	{
		FinalDamage = BaseDamage * URID_DamageStatics::GetCritMultiplier(CasterASC);
	}

	// 6. 데미지 Spec 생성 (1회)
	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
	FGameplayEffectSpecHandle DamageSpecHandle;
	if (DamageEffectClass && DamageByCallerTag.IsValid())
	{
		DamageSpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (DamageSpecHandle.IsValid())
		{
			DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		}
	}

	// 7. 범위 내 몬스터 탐색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("GA_Warrior_Shockwave Exploded! Damage: %.1f (Crit: %s), Hit %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 8. 찾은 몬스터들에게 GE 적용
	UGameplayEffect* StunEffect = StunEffectClass->GetDefaultObject<UGameplayEffect>();

	for (AActor* TargetActor : OverlappedActors)
	{
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
		if (Monster && !Monster->IsDying())
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
			if (TargetASC)
			{
				// [적용 1] 데미지
				if (DamageSpecHandle.IsValid())
				{
					TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
				}

				// [적용 2] 치명타 텍스트
				if (bDidCrit)
				{
					URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
				}

				// [적용 3] 스턴
				if (StunEffect)
				{
					TargetASC->ApplyGameplayEffectToSelf(StunEffect, 1.0f, ContextHandle);
				}
			}
		}
	}

	// 9. 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}