// Source/RamdomItemDefense/Private/GA_MagicFighter_DarkPulse.cpp

#include "GA_MagicFighter_DarkPulse.h"
#include "RamdomItemDefenseCharacter.h"
// --- [�ڵ� ����] ---
// �ð� ȿ���� ����ü�� �ٽ� ������ ���̹Ƿ� ����� �����մϴ�.
#include "DarkPulseProjectile.h"
// --- [�ڵ� ���� ��] ---
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" // RID_LOG ����� ����

// ���� ������ �ʿ��� �����
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "DrawDebugHelpers.h"
// --- [�ڵ� �߰�] ---
// ����ü �̵� ������Ʈ ��� (���� ���� ���� �� �ʿ�)
#include "GameFramework/ProjectileMovementComponent.h"
// --- [�ڵ� �߰� ��] ---


UGA_MagicFighter_DarkPulse::UGA_MagicFighter_DarkPulse()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.2f; // (1.0f�� �����ϸ� 100% �ߵ�)

	VisualProjectileSpeed = 1500.0f;
	ExplosionRadius = 300.0f;
}

void UGA_MagicFighter_DarkPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. ��������, �ʿ��� ������(EffectClass, Target)�� �ִ��� Ȯ��
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DamageEffectClass || !ProjectileClass)
	{
		//UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_DarkPulse: DamageEffectClass or ProjectileClass is not set in Blueprint."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TriggerEventData || !TriggerEventData->Target)
	{
		//UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_DarkPulse: TriggerEventData or Target is NULL."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. Ȯ�� üũ
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());

	if (!OwnerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. ����/�� ��ġ, �̵� �ð� ���
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor); // ���� �� HomingTarget���� ���
	FVector StartLocation = OwnerCharacter->GetActorLocation();
	TargetImpactLocation = TargetActor->GetActorLocation(); // ��� ������ ����

	float Distance = FVector::Dist(StartLocation, TargetImpactLocation);
	float TravelTime = 0.0f;
	if (VisualProjectileSpeed > 0.0f)
	{
		TravelTime = Distance / VisualProjectileSpeed;
	}

	// 4. "���ư���" ����Ʈ ���� (���� ���� ������� ����)
	if (ProjectileClass)
	{
		FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetImpactLocation);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerCharacter;
		SpawnParams.Instigator = OwnerCharacter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // �浹 ���� �׻� ����

		// ADarkPulseProjectile�� �����մϴ�. (�� ����ü�� ���� '�ð� ȿ��'�� ����մϴ�)
		ADarkPulseProjectile* VisualProjectile = GetWorld()->SpawnActorDeferred<ADarkPulseProjectile>(
			ProjectileClass,
			FTransform(SpawnRotation, StartLocation),
			OwnerCharacter,
			OwnerCharacter,
			SpawnParams.SpawnCollisionHandlingOverride
		);

		if (VisualProjectile)
		{
			// --- [�ڵ� ����] ---
			// GetProjectileMovement() ��� ProjectileMovement ������ ���� �����մϴ�.
			if (VisualProjectile->ProjectileMovement)
			{
				VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
			}
			// --- [�ڵ� ���� ��] ---

			UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, StartLocation));
			//RID_LOG(FColor::Cyan, TEXT("GA_DarkPulse: Spawned VISUAL projectile."));
		}
	}

	// 5. �̵� �ð�(TravelTime) �Ŀ� Explode �Լ��� ȣ���ϵ��� Ÿ�̸� ����
	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_MagicFighter_DarkPulse::Explode,
		TravelTime, // ���� �̵� �ð�
		false
	);

	//RID_LOG(FColor::Green, TEXT("GA_DarkPulse: Timer SET. Waiting for Explode..."));
}

/** Ÿ�̸Ӱ� ����Ǿ��� �� ���� ������ �����ϴ� �Լ� */
void UGA_MagicFighter_DarkPulse::Explode()
{
	// 1. ���� �����Ƽ ���� ��������
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	// 2. ������(Caster) ASC �� AttributeSet ��������
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	const UMyAttributeSet* AttributeSet = OwnerCharacter ? OwnerCharacter->GetAttributeSet() : nullptr;

	if (!CasterASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. ���� ����Ʈ �� ���� ��� (����� TargetImpactLocation ���)
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, TargetImpactLocation);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, TargetImpactLocation);
	}

	// 4. ������ ���
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float FinalDamage = -(50.f + (OwnerAttackDamage * 0.3f));

	// 5. ����� ��ü �׸���
	DrawDebugSphere(
		GetWorld(),
		TargetImpactLocation,
		ExplosionRadius,
		12,
		FColor::Red,
		false,
		2.0f,
		0,
		1.0f
	);

	// 6. ���� �� ���� ã��
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		TargetImpactLocation,
		ExplosionRadius,
		ObjectTypes,
		AMonsterBaseCharacter::StaticClass(),
		{},
		OverlappedActors
	);

	RID_LOG(FColor::Cyan, TEXT("GA_DarkPulse Exploded (Delayed)! FinalDamage: %.1f, Hit %d monsters."), FinalDamage, OverlappedActors.Num());

	// 7. ã�� ���͵鿡�� ������ ����
	if (DamageByCallerTag.IsValid())
	{
		FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
		FGameplayEffectSpecHandle SpecHandle = CasterASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, FinalDamage);

			for (AActor* HitActor : OverlappedActors)
			{
				AMonsterBaseCharacter* HitMonster = Cast<AMonsterBaseCharacter>(HitActor);
				if (!HitMonster || HitMonster->IsDying())
				{
					continue;
				}

				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitMonster);
				if (TargetASC)
				{
					// [������ ����]
					CasterASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

					// --- [ �ڡڡ� ���ο� ȿ�� ���� �߰� �ڡڡ� ] ---
					if (SlowEffectClass)
					{
						// �������� ������ EffectContext�� ����Ͽ� ���ο� GE�� �����մϴ�.
						TargetASC->ApplyGameplayEffectToSelf(SlowEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
					}
					// --- [ �ڡڡ� �ڵ� �߰� �� �ڡڡ� ] ---
				}
			}
		}
	}
	else
	{
		RID_LOG(FColor::Red, TEXT("GA_DarkPulse: FAILED (DamageByCallerTag IS INVALID. Check Blueprint!)"));
	}

	// 8. �����Ƽ ����
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}