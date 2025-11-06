#include "GA_Archer_SingleShot.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
// --- [코드 추가] ---
#include "DarkPulseProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
// --- [코드 추가 끝] ---

UGA_Archer_SingleShot::UGA_Archer_SingleShot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.15f;
	VisualProjectileSpeed = 2500.0f; // 기본값 설정
}

void UGA_Archer_SingleShot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !DamageEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (!TargetActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 시작/끝 위치, 이동 시간 계산 (DarkPulse와 동일)
	FVector StartLocation = OwnerCharacter->GetActorLocation();
	FVector EndLocation = TargetActor->GetActorLocation();
	float Distance = FVector::Dist(StartLocation, EndLocation);
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? (Distance / VisualProjectileSpeed) : 0.0f;

	// 2. 시각 효과용 투사체 스폰 (DarkPulse와 동일)
	FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, EndLocation);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADarkPulseProjectile* VisualProjectile = GetWorld()->SpawnActorDeferred<ADarkPulseProjectile>(
		ProjectileClass,
		FTransform(SpawnRotation, StartLocation),
		OwnerCharacter,
		OwnerCharacter,
		SpawnParams.SpawnCollisionHandlingOverride
	);

	if (VisualProjectile)
	{
		if (VisualProjectile->ProjectileMovement)
		{
			// 유도 기능 활성화
			VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		}
		UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, StartLocation));
	}

	// 3. 이동 시간(TravelTime) 후에 OnImpact 함수를 호출하도록 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_Archer_SingleShot::OnImpact,
		TravelTime,
		false
	);

	// 어빌리티는 OnImpact에서 종료
}

/** 투사체 도착 시 실제 데미지를 적용하는 함수 */
void UGA_Archer_SingleShot::OnImpact()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	// 타이머가 만료되기 전에 타겟이 죽었는지 확인
	if (!OwnerCharacter || !SourceASC || !TargetActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());
	const UMyAttributeSet* AttributeSet = OwnerCharacter->GetAttributeSet();

	if (!TargetASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// (데미지 계산: BP의 GE에서 설정하거나, 여기서 C++로 계수 적용)
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = 100.0f + (OwnerAttackDamage * 0.8f); // (예: 단일 대상 계수 0.8)
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor.Get(), true);

	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (DamageSpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}