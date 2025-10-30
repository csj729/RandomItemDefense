#include "GA_MagicFighter_DarkPulse.h"
#include "RamdomItemDefenseCharacter.h"
#include "DarkPulseProjectile.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" // RID_LOG 사용을 위해

UGA_MagicFighter_DarkPulse::UGA_MagicFighter_DarkPulse()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SpawnDistanceForward = 100.0f;
	BaseActivationChance = 0.2f;
}

void UGA_MagicFighter_DarkPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ProjectileClass || !DamageEffectClass)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_DarkPulse: ProjectileClass or DamageEffectClass is not set in Blueprint."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* OwnerASC = ActorInfo->AbilitySystemComponent.Get();
	if (!OwnerCharacter || !OwnerASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UMyAttributeSet* AttributeSet = OwnerCharacter->GetAttributeSet();
	if (!AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float BonusSkillChance = AttributeSet->GetSkillActivationChance();
	const float SkillBaseChance = BaseActivationChance;
	const float FinalActivationChance = FMath::Clamp(SkillBaseChance + BonusSkillChance, 0.0f, 1.0f);

	RID_LOG(FColor::Yellow, TEXT("GA_DarkPulse: Checking FinalChance: %.2f (Base: %.2f + Bonus: %.2f)"),
		FinalActivationChance, SkillBaseChance, BonusSkillChance);

	if (FinalActivationChance <= 0.f || FMath::FRand() > FinalActivationChance)
	{
		RID_LOG(FColor::Orange, TEXT("GA_DarkPulse: Probability Check Failed (Rand > %.2f)"), FinalActivationChance);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	RID_LOG(FColor::Green, TEXT("GA_DarkPulse: Probability Check Success!"));


	if (!TriggerEventData)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_DarkPulse: TriggerEventData is NULL. Cannot get Target."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const AActor* ConstTargetActor = TriggerEventData->Target;

	if (!ConstTargetActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorForwardVector() * SpawnDistanceForward);
	FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, ConstTargetActor->GetActorLocation());

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	RID_LOG(FColor::Cyan, TEXT("GA_DarkPulse: Spawning Projectile Deferred... Class: %s"), *GetNameSafe(ProjectileClass));

	// 1단계: 액터 스폰 시작 (아직 BeginPlay 등은 호출되지 않음)
	ADarkPulseProjectile* DeferredProjectile = GetWorld()->SpawnActorDeferred<ADarkPulseProjectile>(
		ProjectileClass,
		FTransform(SpawnRotation, SpawnLocation), // SpawnLocation, SpawnRotation 대신 FTransform 사용
		OwnerCharacter,
		OwnerCharacter,
		SpawnParams.SpawnCollisionHandlingOverride
	);	

	if (DeferredProjectile)
	{
		RID_LOG(FColor::Yellow, TEXT("GA_DarkPulse: SpawnActorDeferred SUCCEEDED. Initializing..."));

		// 2단계: 필요한 초기화 수행 (Initialize 함수 호출 등)
		DeferredProjectile->Initialize(const_cast<AActor*>(ConstTargetActor), OwnerASC, DamageEffectClass);

		// 3단계: 액터 스폰 완료 (이제 BeginPlay 등이 호출됨)
		UGameplayStatics::FinishSpawningActor(DeferredProjectile, FTransform(SpawnRotation, SpawnLocation)); // SpawnTransform 다시 제공

		RID_LOG(FColor::Green, TEXT("GA_DarkPulse: FinishSpawningActor Called. Projectile should be valid."));
		// 여기서 DeferredProjectile 포인터가 여전히 유효한지 다시 확인 가능
	}
	else
	{
		// SpawnActorDeferred 자체에서 실패한 경우
		RID_LOG(FColor::Red, TEXT("GA_DarkPulse: SpawnActorDeferred FAILED! Class: %s"), *GetNameSafe(ProjectileClass));
	}
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}