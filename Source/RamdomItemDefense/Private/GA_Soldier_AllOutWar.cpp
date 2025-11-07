// Private/GA_Soldier_AllOutWar.cpp (수정)

#include "GA_Soldier_AllOutWar.h"
#include "RamdomItemDefense.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h"

UGA_Soldier_AllOutWar::UGA_Soldier_AllOutWar()
{
	AttackSpeedBuffTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Soldier.AttackSpeed"));
	BuffEffectAttachSocketName = FName("Root");
	BuffEffectComponent = nullptr;

	// --- [ ★★★ 추가 ★★★ ] ---
	// 스케일 기본값 설정
	BuffEffectScale = FVector(1.0f);
	// --- [ ★★★ 추가 끝 ★★★ ] ---
}

void UGA_Soldier_AllOutWar::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!OwnerCharacter || !SourceASC || !UltimateStateEffectClass || !AllOutWarBuffEffectClass || !AttackSpeedBuffTag.IsValid())
	{
		RID_LOG(FColor::Red, TEXT("GA_Soldier_AllOutWar: Missing required components or classes/tags. AttackSpeedBuffTag is: %s"), *AttackSpeedBuffTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. (애니메이션 락) 궁극기 사용 중 상태 GE 적용
	if (UltimateStateEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UltimateStateEffectClass, 1.0f, MakeEffectContext(Handle, ActorInfo));
		if (SpecHandle.IsValid())
		{
			UltimateStateEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// 2. (자가 버프) 30초 버프 GE 적용
	FGameplayEffectSpecHandle BuffSpecHandle = SourceASC->MakeOutgoingSpec(AllOutWarBuffEffectClass, 1.0f, MakeEffectContext(Handle, ActorInfo));
	if (BuffSpecHandle.IsValid())
	{
		const float CasterAttackDamage = AttributeSet->GetAttackDamage();
		const float AttackSpeedBuffValue = 0.5f + (CasterAttackDamage * 0.1f);

		BuffSpecHandle.Data.Get()->SetSetByCallerMagnitude(AttackSpeedBuffTag, AttackSpeedBuffValue);

		UltimateBuffEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*BuffSpecHandle.Data.Get());
		RID_LOG(FColor::Green, TEXT("GA_Soldier_AllOutWar: Applied 30s Buff. AttackSpeed Bonus: %.2f"), AttackSpeedBuffValue);

		if (UltimateBuffEffectHandle.IsValid())
		{
			// 2-4. 지속 이펙트 스폰
			if (BuffEffect)
			{
				// --- [ ★★★ 수정 ★★★ ] ---
				// UGameplayStatics::SpawnEmitterAttached 호출 시 스케일 변수 적용
				BuffEffectComponent = UGameplayStatics::SpawnEmitterAttached(
					BuffEffect,
					OwnerCharacter->GetMesh(),
					BuffEffectAttachSocketName,
					FVector(0.f, 0.f, 50.f), // Relative Location
					FRotator::ZeroRotator, // Relative Rotation
					BuffEffectScale, // (★수정★) 스케일 적용
					EAttachLocation::SnapToTarget,
					true // bAutoDestroy
				);
				// --- [ ★★★ 수정 끝 ★★★ ] ---
			}

			// 2-5. 버프 제거 감지 태스크 등록
			UAbilityTask_WaitGameplayEffectRemoved* WaitEffectRemovedTask = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, UltimateBuffEffectHandle);
			WaitEffectRemovedTask->OnRemoved.AddDynamic(this, &UGA_Soldier_AllOutWar::OnBuffEffectRemoved);
			WaitEffectRemovedTask->ReadyForActivation();
		}
	}

	// 3. 몽타주 재생
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, UltimateMontage);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_Soldier_AllOutWar::OnMontageFinished);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_Soldier_AllOutWar::OnMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_Soldier_AllOutWar::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_Soldier_AllOutWar::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}
	else
	{
		OnMontageFinished();
	}
}

// (이하 EndAbility, OnMontageFinished, OnMontageCancelled, OnBuffEffectRemoved 함수는 변경 사항 없음)
// ...
void UGA_Soldier_AllOutWar::OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo)
{
	if (BuffEffectComponent && BuffEffectComponent->IsValidLowLevel())
	{
		BuffEffectComponent->DestroyComponent();
		BuffEffectComponent = nullptr;
		RID_LOG(FColor::Yellow, TEXT("GA_Soldier_AllOutWar: Buff Effect removed (Duration Ended)."));
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Soldier_AllOutWar::OnMontageFinished()
{
	if (UltimateStateEffectHandle.IsValid() && GetCurrentActorInfo() && GetCurrentActorInfo()->AbilitySystemComponent.IsValid())
	{
		GetCurrentActorInfo()->AbilitySystemComponent->RemoveActiveGameplayEffect(UltimateStateEffectHandle);
		RID_LOG(FColor::Yellow, TEXT("GA_Soldier_AllOutWar: Montage Finished, removed State lock. Buff and Ability Instance continue."));
	}
}

void UGA_Soldier_AllOutWar::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_Soldier_AllOutWar::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (BuffEffectComponent && BuffEffectComponent->IsValidLowLevel())
	{
		BuffEffectComponent->DestroyComponent();
		BuffEffectComponent = nullptr;
	}

	if (SourceASC)
	{
		if (UltimateStateEffectHandle.IsValid())
		{
			SourceASC->RemoveActiveGameplayEffect(UltimateStateEffectHandle);
		}
		if (UltimateBuffEffectHandle.IsValid())
		{
			SourceASC->RemoveActiveGameplayEffect(UltimateBuffEffectHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}