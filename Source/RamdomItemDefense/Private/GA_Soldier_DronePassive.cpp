// Private/GA_Soldier_DronePassive.cpp (수정)

#include "GA_Soldier_DronePassive.h"
#include "SoldierDrone.h" // (추가) ASoldierDrone 헤더 포함
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h" 

UGA_Soldier_DronePassive::UGA_Soldier_DronePassive()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Soldier_DronePassive::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);
	if (ActorInfo && ActorInfo->IsNetAuthority() && !IsActive())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

void UGA_Soldier_DronePassive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo) || !DroneClassToSpawn || !DroneStatInitEffect)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: Failed to activate. Missing DroneClass or DroneStatInitEffect."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (SpawnedDrone != nullptr) { return; }

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	OwnerASC_Cache = ActorInfo->AbilitySystemComponent.Get();
	const UMyAttributeSet* OwnerAttributeSet = OwnerCharacter ? OwnerCharacter->GetAttributeSet() : nullptr;

	if (!OwnerCharacter || !OwnerASC_Cache.IsValid() || !OwnerAttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 드론 스폰 (ASoldierDrone 타입으로)
	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorRightVector() * -150.f) + FVector(0.f, 0.f, 80.f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// (수정) ASoldierDrone 타입으로 스폰 및 캐스팅
	// DroneClassToSpawn 변수 자체는 TSubclassOf<APawn>이지만, 
	// 실제로는 ASoldierDrone BP가 할당되어야 함
	ASoldierDrone* Drone = GetWorld()->SpawnActor<ASoldierDrone>(DroneClassToSpawn, SpawnLocation, OwnerCharacter->GetActorRotation(), SpawnParams);
	SpawnedDrone = Drone; // 부모 APawn 타입 변수에 저장

	if (!Drone)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: Failed to spawn Drone Actor (ASoldierDrone). Check DroneClassToSpawn BP (must inherit from ASoldierDrone)."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// (추가) 드론에게 소유자(솔져)가 누구인지 알려줌
	Drone->SetOwnerCharacter(OwnerCharacter);

	// 2. 드론 ASC 캐시 및 어빌리티 부여
	DroneASC_Cache = Drone->GetAbilitySystemComponent(); // 직접 가져옴
	if (!DroneASC_Cache.IsValid())
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: Spawned Drone does not have an AbilitySystemComponent!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	for (TSubclassOf<UGameplayAbility> AbilityClass : DroneAbilities)
	{
		if (AbilityClass)
		{
			DroneASC_Cache->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, Drone));
		}
	}

	// 3. 소유자 스탯 변경 델리게이트 바인딩 (이전과 동일)
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackDamageAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritChanceAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritDamageAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetSkillActivationChanceAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetArmorReductionAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));

	// 4. 즉시 1회 스탯 갱신
	UpdateDroneStats();

	RID_LOG(FColor::Green, TEXT("GA_Soldier_DronePassive: Activated. Listening for owner stat changes."));
}

// ... (OnOwnerStatChanged, UpdateDroneStats, EndAbility 함수는 이전 "실시간 연동" 버전과 완벽하게 동일합니다) ...

void UGA_Soldier_DronePassive::OnOwnerStatChanged(const FOnAttributeChangeData& Data)
{
	if (IsActive())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGA_Soldier_DronePassive::UpdateDroneStats);
	}
}

void UGA_Soldier_DronePassive::UpdateDroneStats()
{
	if (!OwnerASC_Cache.IsValid() || !DroneASC_Cache.IsValid())
	{
		return;
	}

	const UMyAttributeSet* OwnerAttributeSet = Cast<const UMyAttributeSet>(OwnerASC_Cache->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!OwnerAttributeSet)
	{
		return;
	}

	if (DroneStatEffectHandle.IsValid())
	{
		DroneASC_Cache->RemoveActiveGameplayEffect(DroneStatEffectHandle);
	}

	FGameplayEffectSpecHandle StatSpecHandle = OwnerASC_Cache->MakeOutgoingSpec(DroneStatInitEffect, 1.0f, OwnerASC_Cache->MakeEffectContext());
	if (StatSpecHandle.IsValid())
	{
		const float StatMultiplier = 0.2f; // 1/5

		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.AttackDamage")), OwnerAttributeSet->GetAttackDamage() * StatMultiplier);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.AttackSpeed")), OwnerAttributeSet->GetAttackSpeed() * StatMultiplier);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.CritChance")), OwnerAttributeSet->GetCritChance() * StatMultiplier);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.CritDamage")), OwnerAttributeSet->GetCritDamage() * StatMultiplier);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.SkillActivationChance")), OwnerAttributeSet->GetSkillActivationChance() * StatMultiplier);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.ArmorReduction")), OwnerAttributeSet->GetArmorReduction() * StatMultiplier);

		DroneStatEffectHandle = DroneASC_Cache->ApplyGameplayEffectSpecToSelf(*StatSpecHandle.Data.Get());

		RID_LOG(FColor::Cyan, TEXT("GA_Soldier_DronePassive: Stats Updated. New AD: %.1f"), OwnerAttributeSet->GetAttackDamage() * StatMultiplier);
	}
}

void UGA_Soldier_DronePassive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (OwnerASC_Cache.IsValid())
	{
		for (FDelegateHandle DelegateHandle : AttributeDelegateHandles)
		{
			OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackDamageAttribute()).Remove(DelegateHandle);
			OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).Remove(DelegateHandle);
			OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritChanceAttribute()).Remove(DelegateHandle);
			OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritDamageAttribute()).Remove(DelegateHandle);
			OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetSkillActivationChanceAttribute()).Remove(DelegateHandle);
			OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetArmorReductionAttribute()).Remove(DelegateHandle);
		}
	}
	AttributeDelegateHandles.Empty();

	if (DroneASC_Cache.IsValid() && DroneStatEffectHandle.IsValid())
	{
		DroneASC_Cache->RemoveActiveGameplayEffect(DroneStatEffectHandle);
	}

	if (SpawnedDrone && HasAuthority(&ActivationInfo))
	{
		SpawnedDrone->Destroy();
		SpawnedDrone = nullptr;
		RID_LOG(FColor::Yellow, TEXT("GA_Soldier_DronePassive: Ability ended, Drone destroyed."));
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}