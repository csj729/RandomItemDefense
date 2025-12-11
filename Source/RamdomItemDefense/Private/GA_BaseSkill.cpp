// Source/RamdomItemDefense/Private/GA_BaseSkill.cpp
#include "GA_BaseSkill.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefenseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "SoldierDrone.h"
#include "Components/SceneComponent.h"
#include "MyAttributeSet.h"
#include "RID_DamageStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"

UGA_BaseSkill::UGA_BaseSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	BaseActivationChance = 0.f;
	DamageBase = 0.f;
	DamageCoefficient = 0.f;

	MuzzleSocketName = FName("MuzzlePoint");
	MuzzleFlashEffect = nullptr;
}

void UGA_BaseSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (MuzzleFlashEffect)
	{
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(AvatarActor))
		{
			OwnerCharacter->Multicast_SpawnParticleAttached(
				MuzzleFlashEffect, MuzzleSocketName, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.0f));
		}
		else if (ASoldierDrone* OwnerDrone = Cast<ASoldierDrone>(AvatarActor))
		{
			OwnerDrone->Multicast_SpawnParticleAttached(
				MuzzleFlashEffect, MuzzleSocketName, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.0f));
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

// --- [ ★★★ 리팩토링 구현부 ★★★ ] ---

void UGA_BaseSkill::GetMuzzleTransform(AActor* AvatarActor, FVector& OutLocation, FRotator& OutRotation, AActor* TargetActor)
{
	if (!AvatarActor) return;

	OutLocation = AvatarActor->GetActorLocation();

	// 1. 소켓 위치 가져오기 (캐릭터 vs 드론)
	USceneComponent* AttachComponent = nullptr;
	if (ARamdomItemDefenseCharacter* Character = Cast<ARamdomItemDefenseCharacter>(AvatarActor))
	{
		AttachComponent = Character->GetMesh();
	}
	else if (ASoldierDrone* Drone = Cast<ASoldierDrone>(AvatarActor))
	{
		AttachComponent = Drone->GetMesh();
	}

	// MuzzleSocketName을 사용 (자식 클래스에서 덮어쓴 이름이 있다면 그것 사용)
	if (AttachComponent && MuzzleSocketName != NAME_None && AttachComponent->DoesSocketExist(MuzzleSocketName))
	{
		OutLocation = AttachComponent->GetSocketLocation(MuzzleSocketName);
	}

	// 2. 회전 계산 (Target이 있으면 LookAt, 없으면 ActorRotation)
	if (TargetActor)
	{
		OutRotation = UKismetMathLibrary::FindLookAtRotation(OutLocation, TargetActor->GetActorLocation());
	}
	else
	{
		OutRotation = AvatarActor->GetActorRotation();
	}
}

FGameplayEffectSpecHandle UGA_BaseSkill::MakeDamageEffectSpec(const FGameplayAbilityActorInfo* ActorInfo, float InBaseDmg, float InCoeff, float& OutFinalDamage, bool& OutDidCrit)
{
	OutFinalDamage = 0.0f;
	OutDidCrit = false;

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	if (!SourceASC || !DamageEffectClass) return FGameplayEffectSpecHandle();

	const UMyAttributeSet* AS = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!AS) return FGameplayEffectSpecHandle();

	// 1. 기본 데미지 계산
	const float OwnerAttackDamage = AS->GetAttackDamage();
	const float RawDamage = InBaseDmg + (OwnerAttackDamage * InCoeff);

	// 2. 치명타 계산 (시전자 스탯 기반)
	OutDidCrit = URID_DamageStatics::CheckForCrit(SourceASC, true);
	OutFinalDamage = RawDamage;

	if (OutDidCrit)
	{
		OutFinalDamage = RawDamage * URID_DamageStatics::GetCritMultiplier(SourceASC);
	}

	// 3. Spec 생성 및 값 설정
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);

	if (SpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -OutFinalDamage);
	}

	return SpecHandle;
}

bool UGA_BaseSkill::ApplyDamageToTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* Target, float InBaseDmg, float InCoeff)
{
	if (!Target) return false;

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC || !DamageEffectClass) return false;

	const UMyAttributeSet* AS = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!AS) return false;

	// 1. 데미지 계산 (타겟 정보 포함하여 치명타 계산)
	const float BaseVal = InBaseDmg + (AS->GetAttackDamage() * InCoeff);
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseVal, SourceASC, Target, true);

	// 2. Spec 생성 및 적용
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (SpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		return true;
	}

	return false;
}