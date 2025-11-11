// Private/GA_Soldier_Snipe.cpp

#include "GA_Soldier_Snipe.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RID_DamageStatics.h"
#include "ProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "SoldierDrone.h" // [ ★★★ 포함 필수 ★★★ ] (드론 캐스팅용)

UGA_Soldier_Snipe::UGA_Soldier_Snipe()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;
	VisualProjectileSpeed = 10000.0f;
	ProjectileSpawnSocketName = FName("MuzzleSocket"); // [ ★★★ 코드 추가 ★★★ ]

	DamageBase = 300.0f;
	DamageCoefficient = 2.5f;
}

void UGA_Soldier_Snipe::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!AvatarActor || !SourceASC || !AvatarActor->HasAuthority() || !DamageEffectClass || !StunEffectClass || !ProjectileClass || !TriggerEventData || !TriggerEventData->Target)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_Soldier_Snipe: ActivateAbility FAILED. Invalid data."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (!TargetActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 타겟 위치를 먼저 가져옵니다.
	FVector TargetLocation = TargetActor->GetActorLocation();

	// 2. 스폰 위치를 찾습니다. (기본값 = 액터 위치)
	FVector SpawnLocation = AvatarActor->GetActorLocation();

	// 3. 부착할 메쉬(캐릭터 또는 드론)를 찾습니다.
	USceneComponent* AttachComponent = nullptr;
	if (ARamdomItemDefenseCharacter* CharacterActor = Cast<ARamdomItemDefenseCharacter>(AvatarActor))
	{
		AttachComponent = CharacterActor->GetMesh();
	}
	else if (ASoldierDrone* DroneActor = Cast<ASoldierDrone>(AvatarActor))
	{
		AttachComponent = DroneActor->GetMesh();
	}

	// 4. 소켓이 존재하면 스폰 위치를 '소켓의 월드 위치'로 덮어씁니다.
	if (AttachComponent && ProjectileSpawnSocketName != NAME_None && AttachComponent->DoesSocketExist(ProjectileSpawnSocketName))
	{
		SpawnLocation = AttachComponent->GetSocketLocation(ProjectileSpawnSocketName);
		UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Soldier_Snipe: Spawning from Socket '%s'"), *ProjectileSpawnSocketName.ToString());
	}
	else
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_Soldier_Snipe: Could not find socket '%s'. Spawning from Actor Origin."), *ProjectileSpawnSocketName.ToString());
	}

	// 5. (가장 중요) 스폰 '위치'와 타겟 '위치'를 기반으로
	//    '정확한 발사 방향'을 *지금* 계산합니다.
	FRotator SpawnRotation = UKismetMathLibrary::FindLookAtRotation(SpawnLocation, TargetLocation);

	// (기존의 GetSocketRotation() 호출을 완전히 제거합니다)
	// --- [ ★★★ 수정 끝 ★★★ ] ---

	float Distance = FVector::Dist(SpawnLocation, TargetLocation);
	float TravelTime = (VisualProjectileSpeed > 0.0f) ? FMath::Max(0.01f, Distance / VisualProjectileSpeed) : 0.01f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileBase* VisualProjectile = GetWorld()->SpawnActorDeferred<AProjectileBase>(
		ProjectileClass,
		FTransform(SpawnRotation, SpawnLocation), // <-- 수정된 SpawnRotation 사용
		SpawnParams.Owner,
		SpawnParams.Instigator,
		SpawnParams.SpawnCollisionHandlingOverride
	);

	if (VisualProjectile)
	{
		if (VisualProjectile->ProjectileMovement)
		{
			VisualProjectile->ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		}
		VisualProjectile->SetLifeSpan(TravelTime);
		UGameplayStatics::FinishSpawningActor(VisualProjectile, FTransform(SpawnRotation, SpawnLocation));
	}

	GetWorld()->GetTimerManager().SetTimer(
		ImpactTimerHandle,
		this,
		&UGA_Soldier_Snipe::OnImpact,
		TravelTime,
		false
	);
}

void UGA_Soldier_Snipe::OnImpact()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!SourceASC || !TargetActor.IsValid() || !DamageEffectClass || !StunEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get());

	// [ ★★★ 수정 ★★★ ] (ASC에서 AttributeSet 직접 가져오기)
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));

	if (!TargetASC || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// (이하 데미지 계산 로직은 AttributeSet이 유효하므로 정상 작동)
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor.Get(), true);

	UE_LOG(LogRamdomItemDefense, Warning, TEXT("### DRONE SNIPE (SKILL3) ###: Attacker=[%s] -> Target=[%s], Damage=%.1f"),
		*GetNameSafe(ActorInfo->AvatarActor.Get()),
		*GetNameSafe(TargetActor.Get()),
		FinalDamage
	);

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();

	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
	if (DamageSpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}

	TargetASC->ApplyGameplayEffectToSelf(StunEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}