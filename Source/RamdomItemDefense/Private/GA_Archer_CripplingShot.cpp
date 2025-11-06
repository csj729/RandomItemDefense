#include "GA_Archer_CripplingShot.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
#include "ProjectileBase.h" 
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"


UGA_Archer_CripplingShot::UGA_Archer_CripplingShot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;
	VisualProjectileSpeed = 2000.0f;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	// 부모 클래스의 변수에 기본값 설정
	DamageBase = 300.0f;
	DamageCoefficient = 1.5f; // (150%)
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
}

void UGA_Archer_CripplingShot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !DamageEffectClass || !StunEffectClass || !ArmorShredEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
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

	// 1. 투사체 이동 시간 계산
	FVector StartLocation = OwnerCharacter->GetActorLocation();
	FVector EndLocation = TargetActor->GetActorLocation();
	float Distance = FVector::Dist(StartLocation, EndLocation);
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? (Distance / VisualProjectileSpeed) : 0.0f;

	// 2. 시각 효과용 투사체 스폰
	FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, EndLocation);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileBase* VisualProjectile = GetWorld()->SpawnActorDeferred<AProjectileBase>(
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
			VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		}
		UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, StartLocation));
	}

	// 3. 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_Archer_CripplingShot::OnImpact,
		TravelTime,
		false
	);
}

/** 투사체 도착 시 실제 데미지/효과를 적용하는 함수 */
void UGA_Archer_CripplingShot::OnImpact()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

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

	// --- [ ★★★ 코드 수정 ★★★ ] ---
	// 1. 데미지 계산 (부모 변수 사용)
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	// 하드코딩된 값 대신 부모의 변수(DamageBase, DamageCoefficient) 사용
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);
	// --- [ ★★★ 코드 수정 끝 ★★★ ] ---
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor.Get(), true);

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();

	// 2. 데미지 GE 적용
	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
	if (DamageSpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}

	// 3. 4초 스턴 GE 적용
	TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);

	// 4. 방어력 감소 GE 적용
	TargetASC->ApplyGameplayEffectToSelf(ArmorShredEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}