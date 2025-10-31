// Source/RamdomItemDefense/Private/GA_MagicFighter_MeteorStrike.cpp (����)

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
// --- [�ڵ� �߰�] ---
#include "RID_DamageStatics.h" // ������ ��� ���� ����
// --- [�ڵ� �߰� ��] ---

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
	// ���������� ������ �����մϴ�.
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. GA_AttackSelecter�� �Ѱ��� Ÿ�� ������ �����ɴϴ�.
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);

	if (!TargetActor)
	{
		RID_LOG(FColor::Red, TEXT("GA_MagicFighter_MeteorStrike Error: No Target found!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. ���� ��ġ(ImpactLocation)�� ����Ʈ ���� ��ġ(SpawnLocation) ���
	ImpactLocation = TargetActor->GetActorLocation(); // Ÿ���� ���� ��ġ�� ��� ������ ����
	FVector SpawnLocation = ImpactLocation + FVector(0.0f, 0.0f, SpawnHeight);

	// 3. '��������' ����Ʈ ���� (�ð� ȿ��)
	if (FallingEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FallingEffect, SpawnLocation, FRotator::ZeroRotator, true);
	}

	// 4. FallDuration(0.5��) �Ŀ� Explode �Լ��� ȣ���ϵ��� Ÿ�̸� ����
	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_MagicFighter_MeteorStrike::Explode,
		FallDuration,
		false
	);

	// �����Ƽ�� Explode �Լ����� ����
}

/** Ÿ�̸Ӱ� ����Ǿ��� �� ���� ������ �����ϴ� �Լ� */
void UGA_MagicFighter_MeteorStrike::Explode()
{
	// ���� �����Ƽ �ڵ� �� ���� ��������
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	// 1. ������(Caster)�� AbilitySystemComponent�� �����ɴϴ�.
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	if (!CasterASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. �������� ���� AttackDamage ���� �����ɴϴ�.
	float CasterAttackDamage = CasterASC->GetNumericAttribute(UMyAttributeSet::GetAttackDamageAttribute());

	// 3. �⺻ ������ ��� (���)
	float BaseDamage = 100.0f + (CasterAttackDamage * 0.2f);

	// --- [ �ڡڡ� ���� ��ų ġ��Ÿ ���� ���� �ڡڡ� ] ---

	// 3-1. ġ��Ÿ ������ '�� �� ��' �����մϴ�. (bIsSkillAttack: true)
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(CasterASC, true);

	// 3-2. ġ��Ÿ ���ο� ���� '���� ������'�� '�� �� ��' ����մϴ�.
	float FinalDamage = BaseDamage;
	if (bDidCrit)
	{
		FinalDamage = BaseDamage * URID_DamageStatics::GetCritMultiplier(CasterASC);
	}
	// --- [ �ڡڡ� ���� ���� �� �ڡڡ� ] ---


	// 4. '����' ����Ʈ ���� (�ð� ȿ��)
	if (ExplosionEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, ImpactLocation, FRotator::ZeroRotator, true);

	// 5. ImpactLocation���� 500 �ݰ� �� ���� ã��
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ImpactLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("Meteor Exploded! Damage: %.1f (Crit: %s), Found %d monsters."), FinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());

	// 6. ã�� ���͵鿡�� GE ����
	if (OverlappedActors.Num() > 0)
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

		// 6-1. ������ GE Spec�� '�� �� ��' �����մϴ�.
		FGameplayEffectSpecHandle DamageSpecHandle;
		if (DamageEffectClass && DamageByCallerTag.IsValid())
		{
			DamageSpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
			if (DamageSpecHandle.IsValid())
			{
				// 6-2. Spec�� '���� ������'�� '�� �� ��' �����մϴ�.
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
					// [���� 1] ������: �̸� ������ '������ Spec'�� �����մϴ�.
					if (DamageSpecHandle.IsValid())
					{
						TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
					}

					// [���� 2] ġ��Ÿ �ؽ�Ʈ: ġ��Ÿ�� �߻��ߴٸ�, �� ���Ϳ��� ��������Ʈ�� ����մϴ�.
					if (bDidCrit)
					{
						URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, FinalDamage);
					}

					// [���� 3] ����: ������ Ŭ������ �ٷ� �����մϴ�.
					if (StunEffectClass)
					{
						TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
					}
				}
			}
		}
	}

	// 7. �����Ƽ ����
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}