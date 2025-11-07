// Private/GA_Soldier_DronePassive.cpp (수정)

#include "GA_Soldier_DronePassive.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h" // TryActivateAbility용

UGA_Soldier_DronePassive::UGA_Soldier_DronePassive()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

/** 어빌리티가 부여될 때 자동으로 활성화를 시도합니다. */
void UGA_Soldier_DronePassive::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// 서버에서만, 그리고 아직 활성화되지 않았을 때
	if (ActorInfo && ActorInfo->IsNetAuthority() && !IsActive())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

void UGA_Soldier_DronePassive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// (이하 스폰 로직은 동일)
	if (!HasAuthority(&ActivationInfo) || !DroneClassToSpawn || !DroneStatInitEffect)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: Failed to activate. Missing DroneClass or DroneStatInitEffect."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (SpawnedDrone != nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* OwnerASC = ActorInfo->AbilitySystemComponent.Get();
	const UMyAttributeSet* OwnerAttributeSet = OwnerCharacter ? OwnerCharacter->GetAttributeSet() : nullptr;

	if (!OwnerCharacter || !OwnerASC || !OwnerAttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + FVector(100.f, 100.f, 0.f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedDrone = GetWorld()->SpawnActor<APawn>(DroneClassToSpawn, SpawnLocation, OwnerCharacter->GetActorRotation(), SpawnParams);

	if (!SpawnedDrone)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: Failed to spawn Drone Actor."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* DroneASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SpawnedDrone);
	if (!DroneASC)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Soldier_DronePassive: Spawned Drone does not have an AbilitySystemComponent!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FGameplayEffectSpecHandle StatSpecHandle = OwnerASC->MakeOutgoingSpec(DroneStatInitEffect, 1.0f, MakeEffectContext(Handle, ActorInfo));
	if (StatSpecHandle.IsValid())
	{
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Item.Stat.AttackDamage")), OwnerAttributeSet->GetAttackDamage() * 0.25f);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Item.Stat.AttackSpeed")), OwnerAttributeSet->GetAttackSpeed() * 0.25f);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Item.Stat.CritChance")), OwnerAttributeSet->GetCritChance() * 0.25f);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Item.Stat.CritDamage")), OwnerAttributeSet->GetCritDamage() * 0.25f);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Item.Stat.SkillActivationChance")), OwnerAttributeSet->GetSkillActivationChance() * 0.25f);
		StatSpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Item.Stat.ArmorReduction")), OwnerAttributeSet->GetArmorReduction() * 0.25f);

		DroneASC->ApplyGameplayEffectSpecToSelf(*StatSpecHandle.Data.Get());
		RID_LOG(FColor::Green, TEXT("GA_Soldier_DronePassive: Drone Spawned and Stats Transferred."));
	}

	for (TSubclassOf<UGameplayAbility> AbilityClass : DroneAbilities)
	{
		if (AbilityClass)
		{
			DroneASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, SpawnedDrone));
		}
	}

	// 패시브 어빌리티 자체는 할 일을 다 했으므로 종료 (드론은 계속 살아있음)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Soldier_DronePassive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (SpawnedDrone && HasAuthority(&ActivationInfo))
	{
		SpawnedDrone->Destroy();
		SpawnedDrone = nullptr;
		RID_LOG(FColor::Yellow, TEXT("GA_Soldier_DronePassive: Ability ended, Drone destroyed."));
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}