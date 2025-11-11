// Source/RamdomItemDefense/Private/GA_BasicAttack.cpp

#include "GA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h" 
#include "RamdomItemDefenseCharacter.h" 
#include "GameplayEffectTypes.h" 
#include "AbilitySystemBlueprintLibrary.h" 
#include "RID_DamageStatics.h"
#include "RamdomItemDefense.h" 
#include "SoldierDrone.h" // [ ★★★ 포함 필수 ★★★ ] (드론 캐스팅용)
#include "Kismet/GameplayStatics.h" // [ ★★★ 포함 필수 ★★★ ] (이펙트 스폰용)
#include "GameFramework/Character.h" // [ ★★★ 포함 필수 ★★★ ] (캐릭터 캐스팅용)


UGA_BasicAttack::UGA_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	DamageCoefficient = 1.0f;
	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value"));
	MuzzleSocketName = FName("MuzzleSocket");
}

void UGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// [ ★★★ 수정된 로그 ( .IsValid() 및 .Get() 사용) ★★★ ]
	if (TriggerEventData && TriggerEventData->Target)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("### BASIC ATTACK ###: Attacker=[%s] -> Target=[%s]"),
			*GetNameSafe(ActorInfo->AvatarActor.Get()),
			*GetNameSafe(TriggerEventData->Target.Get()) // .Get()으로 실제 Actor 포인터를 가져옵니다.
		);
	}
	else
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_BasicAttack [%s]: FAILED. TriggerEventData or Target is NULL."), *GetNameSafe(ActorInfo->AvatarActor.Get()));
	}
	// [ ★★★ 로그 끝 ★★★ ]


	// --- [ ★★★ Muzzle Flash 스폰 로직 (드론/캐릭터 분기) ★★★ ] ---
	if (MuzzleFlashEffect && MuzzleSocketName != NAME_None)
	{
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();
		USceneComponent* AttachComponent = nullptr;

		// 1. 공격자가 캐릭터인지 확인
		if (ARamdomItemDefenseCharacter* CharacterActor = Cast<ARamdomItemDefenseCharacter>(AvatarActor))
		{
			AttachComponent = CharacterActor->GetMesh();
		}
		// 2. 캐릭터가 아니라면, 드론인지 확인
		else if (ASoldierDrone* DroneActor = Cast<ASoldierDrone>(AvatarActor))
		{
			AttachComponent = DroneActor->GetMesh();
		}

		// 3. 부착할 메쉬를 찾았는지 확인
		if (AttachComponent)
		{
			// 4. 이펙트 스폰
			UGameplayStatics::SpawnEmitterAttached(
				MuzzleFlashEffect,
				AttachComponent,
				MuzzleSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				FVector(1.0f),
				EAttachLocation::SnapToTarget,
				true // AutoDestroy
			);
		}
	}
	// --- [ ★★★ 로직 끝 ★★★ ] ---


	// 1. 필요한 데이터 확인 (데미지 이펙트, 타겟)
	if (!DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// --- [ ★★★ .IsValid() 체크로 수정 ★★★ ] ---
	if (!TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// --- [ 수정 끝 ] ---

	// 2. 소유자 ASC, 타겟 액터 가져오기
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	// --- [ ★★★ .Get() 호출로 수정 (핵심!) ★★★ ] ---
	const AActor* ConstTargetActor = TriggerEventData->Target.Get();
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
	// --- [ 수정 끝 ] ---

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!SourceASC || !TargetActor || !TargetASC)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_BasicAttack: FAILED. SourceASC, TargetActor, or TargetASC is NULL."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 소유자의 공격력 가져오기 (드론/캐릭터 공용)
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));

	if (!AttributeSet)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_BasicAttack [%s]: FAILED. Could not find UMyAttributeSet on ASC."), *GetNameSafe(ActorInfo->AvatarActor.Get()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// (이하 데미지 계산 및 적용 로직은 모두 동일)
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = OwnerAttackDamage * DamageCoefficient;
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, false);
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (SpecHandle.IsValid() && DamageDataTag.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
	else
	{
		if (!DamageDataTag.IsValid())
		{
			UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_BasicAttack: DamageDataTag is Invalid! Check Blueprint settings."));
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}