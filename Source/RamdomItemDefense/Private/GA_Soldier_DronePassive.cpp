// Private/GA_Soldier_DronePassive.cpp (수정)

#include "GA_Soldier_DronePassive.h"
#include "SoldierDrone.h" 
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h" // [ ★★★ UE_LOG 사용을 위해 필요할 수 있음 ★★★ ]

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
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: FAILED. Missing DroneClass or DroneStatInitEffect."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (SpawnedDrone != nullptr) { return; }

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	OwnerASC_Cache = ActorInfo->AbilitySystemComponent.Get();
	const UMyAttributeSet* OwnerAttributeSet = OwnerCharacter ? OwnerCharacter->GetAttributeSet() : nullptr;

	if (!OwnerCharacter || !OwnerASC_Cache.IsValid() || !OwnerAttributeSet)
	{
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: FAILED. Missing OwnerCharacter, ASC, or AttributeSet."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 드론 스폰 (ASoldierDrone 타입으로)
	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorRightVector() * -150.f) + FVector(0.f, 0.f, 80.f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASoldierDrone* Drone = GetWorld()->SpawnActor<ASoldierDrone>(DroneClassToSpawn, SpawnLocation, OwnerCharacter->GetActorRotation(), SpawnParams);
	SpawnedDrone = Drone;

	if (!Drone)
	{
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: FAILED to spawn Drone Actor (ASoldierDrone). Check BP inheritance."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// [ ★★★ UE_LOG 추가 ★★★ ]
	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Soldier_DronePassive: Drone Actor '%s' Spawned."), *GetNameSafe(Drone));

	Drone->SetOwnerCharacter(OwnerCharacter);

	// 2. 드론 ASC 캐시 및 어빌리티 부여
	DroneASC_Cache = Drone->GetAbilitySystemComponent();
	if (!DroneASC_Cache.IsValid())
	{
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: Spawned Drone does not have an AbilitySystemComponent!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// [ ★★★ UE_LOG 추가 ★★★ ]
	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Soldier_DronePassive: Found Drone ASC. Giving %d abilities..."), DroneAbilities.Num());

	for (TSubclassOf<UGameplayAbility> AbilityClass : DroneAbilities)
	{
		if (AbilityClass)
		{
			DroneASC_Cache->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, Drone));
			// [ ★★★ UE_LOG 추가 ★★★ ]
			UE_LOG(LogRamdomItemDefense, Log, TEXT("  -> Gave Ability: %s"), *GetNameSafe(AbilityClass));
		}
		else
		{
			// [ ★★★ UE_LOG 추가 ★★★ ]
			UE_LOG(LogRamdomItemDefense, Warning, TEXT("  -> Found NULL entry in DroneAbilities array."));
		}
	}

	// 3. 소유자 스탯 변경 델리게이트 바인딩 (사용자님이 추가한 AttackRange 포함)
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackDamageAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritChanceAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetCritDamageAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetSkillActivationChanceAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetArmorReductionAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));
	AttributeDelegateHandles.Add(OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackRangeAttribute()).AddUObject(this, &UGA_Soldier_DronePassive::OnOwnerStatChanged));

	// 4. 즉시 1회 스탯 갱신
	UpdateDroneStats();

	// [ ★★★ UE_LOG 수정 ★★★ ]
	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Soldier_DronePassive: Activated. Listening for owner stat changes."));
}

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
		// [ ★★★ UE_LOG 추가 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("UpdateDroneStats: FAILED. OwnerASC or DroneASC is invalid."));
		return;
	}

	const UMyAttributeSet* OwnerAttributeSet = Cast<const UMyAttributeSet>(OwnerASC_Cache->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!OwnerAttributeSet)
	{
		// [ ★★★ UE_LOG 추가 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("UpdateDroneStats: FAILED. OwnerAttributeSet is NULL."));
		return;
	}

	if (DroneStatEffectHandle.IsValid())
	{
		DroneASC_Cache->RemoveActiveGameplayEffect(DroneStatEffectHandle);
	}

	FGameplayEffectSpecHandle StatSpecHandle = OwnerASC_Cache->MakeOutgoingSpec(DroneStatInitEffect, 1.0f, OwnerASC_Cache->MakeEffectContext());
	if (StatSpecHandle.IsValid())
	{
		const float StatMultiplier = 1.0f / 3.0f; // 1/3

		// [ ★★★ 로그용 변수 선언 ★★★ ]
		float TransferredAD = OwnerAttributeSet->GetAttackDamage() * StatMultiplier;
		float TransferredAS = OwnerAttributeSet->GetAttackSpeed() * StatMultiplier;
		float TransferredCritChance = OwnerAttributeSet->GetCritChance() * StatMultiplier;
		float TransferredCritDmg = OwnerAttributeSet->GetCritDamage() * StatMultiplier;
		float TransferredSkillChance = OwnerAttributeSet->GetSkillActivationChance() * StatMultiplier;
		float TransferredArmorRed = OwnerAttributeSet->GetArmorReduction() * StatMultiplier;

		// [ ★★★ 공격 사거리(AttackRange) 스탯 계산 (버그 수정됨) ★★★ ]
		float TransferredAtkRange = OwnerAttributeSet->GetAttackRange();


		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.AttackDamage")), TransferredAD);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.AttackSpeed")), TransferredAS);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.CritChance")), TransferredCritChance);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.CritDamage")), TransferredCritDmg);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.SkillActivationChance")), TransferredSkillChance);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.ArmorReduction")), TransferredArmorRed);

		// [ ★★★ 공격 사거리(AttackRange) 스탯 전달 (버그 수정됨) ★★★ ]
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Drone.Stat.AttackRange")), TransferredAtkRange);


		DroneStatEffectHandle = DroneASC_Cache->ApplyGameplayEffectSpecToSelf(*StatSpecHandle.Data.Get());

		// [ ★★★ UE_LOG 수정 (모든 스탯 출력) ★★★ ]
		UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Soldier_DronePassive: Stats Updated. AD: %.1f, AS: %.2f, CritChance: %.2f, AtkRange: %.1f"),
			TransferredAD, TransferredAS, TransferredCritChance, TransferredAtkRange);
	}
	else
	{
		// [ ★★★ UE_LOG 추가 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Error, TEXT("UpdateDroneStats: FAILED. MakeOutgoingSpec failed for DroneStatInitEffect."));
	}
}

void UGA_Soldier_DronePassive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// [ ★★★ UE_LOG 추가 ★★★ ]
	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Soldier_DronePassive: EndAbility called. Cleaning up..."));

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
			// [ ★★★ 공격 사거리(AttackRange) 델리게이트 해제 추가 (사용자님 코드 반영) ★★★ ]
			OwnerASC_Cache->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetAttackRangeAttribute()).Remove(DelegateHandle);
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
		// [ ★★★ UE_LOG 수정 ★★★ ]
		UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Soldier_DronePassive: Ability ended, Drone destroyed."));
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}