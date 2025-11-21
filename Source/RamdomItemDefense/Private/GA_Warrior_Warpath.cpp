// Private/GA_Warrior_Warpath.cpp (수정)

#include "GA_Warrior_Warpath.h"
#include "RamdomItemDefense.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "RamdomItemDefenseCharacter.h"
#include "MyPlayerState.h"
#include "MonsterSpawner.h"
#include "MonsterBaseCharacter.h"
#include "MyAttributeSet.h"
#include "RID_DamageStatics.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h" // 추가: GE 제거 태스크 헤더

UGA_Warrior_Warpath::UGA_Warrior_Warpath()
{
	DamageRadius = 10000.0f; // 맵 전체
	DamageBase = 1000.0f;
	DamageCoefficient = 3.0f; // 공격력 300%
	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value"));

	// --- [ ★★★ 추가 ★★★ ] ---
	BuffEffectAttachSocketName = FName("Root"); // 기본 소켓
	BuffIsActiveTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Warrior.WarpathBuffActive"));
	// --- [ ★★★ 추가 끝 ★★★ ] ---
}

void UGA_Warrior_Warpath::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 1. 부모 ActivateAbility (스택 리셋)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 2. 시전자, PS, 스포너, ASC 가져오기
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	AMyPlayerState* PS = OwnerCharacter ? OwnerCharacter->GetPlayerState<AMyPlayerState>() : nullptr;
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!OwnerCharacter || !PS || !SourceASC || !UltimateStateEffectClass || !UltimateBuffEffectClass || !DamageEffectClass)
	{
		RID_LOG(FColor::Red, TEXT("GA_Warrior_Warpath: Missing required components or classes."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. (애니메이션 락) 궁극기 사용 중 상태 GE 적용
	if (UltimateStateEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UltimateStateEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			UltimateStateEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// 4. (자가 버프) 스킬 확률 증가 버프 GE 적용
	if (UltimateBuffEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UltimateBuffEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			UltimateBuffEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			RID_LOG(FColor::Green, TEXT("GA_Warrior_Warpath: Applied 20s Skill Chance Buff."));

			if (UltimateBuffEffectHandle.IsValid())
			{
				// --- [ ★★★ 수정: 파티클 시스템 멀티캐스트 호출 ★★★ ] ---
				// 캐릭터에게 모든 클라이언트에서 파티클을 켜라고 명령
				if (BuffEffect)
				{
					// 앞서 만든 Multicast_AddBuffEffect (캐스케이드용) 호출
					OwnerCharacter->Multicast_AddBuffEffect(
						BuffIsActiveTag,
						BuffEffect,
						BuffEffectAttachSocketName,
						FVector::ZeroVector,
						FVector(1.0f) // 기본 스케일
					);
				}
				// -------------------------------------------------------

				// 버프 제거 감지 태스크 등록
				UAbilityTask_WaitGameplayEffectRemoved* WaitEffectRemovedTask = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, UltimateBuffEffectHandle);
				WaitEffectRemovedTask->OnRemoved.AddDynamic(this, &UGA_Warrior_Warpath::OnBuffEffectRemoved);
				WaitEffectRemovedTask->ReadyForActivation();
			}
		}
	}

	// 5. (광역 데미지) ... (데미지 로직은 변경 없음) ...
	FVector CasterLocation = OwnerCharacter->GetActorLocation();
	FVector DamageCenter = CasterLocation;
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	const float TotalBaseDamage = DamageBase + (AttributeSet->GetAttackDamage() * DamageCoefficient);
	const bool bDidCrit = URID_DamageStatics::CheckForCrit(SourceASC, true);
	float TotalFinalDamage = TotalBaseDamage;
	if (bDidCrit)
	{
		TotalFinalDamage = TotalBaseDamage * URID_DamageStatics::GetCritMultiplier(SourceASC);
	}
	FGameplayEffectContextHandle DamageContextHandle = MakeEffectContext(Handle, ActorInfo);
	FGameplayEffectSpecHandle DamageSpecHandle;
	if (DamageDataTag.IsValid())
	{
		DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, DamageContextHandle);
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -TotalFinalDamage);
	}
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), DamageCenter, DamageRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);
	RID_LOG(FColor::Cyan, TEXT("GA_Warrior_Warpath: Damage: %.1f (Crit: %s), Hit %d monsters."), TotalFinalDamage, bDidCrit ? TEXT("YES") : TEXT("NO"), OverlappedActors.Num());
	if (DamageSpecHandle.IsValid())
	{
		for (AActor* TargetActor : OverlappedActors)
		{
			AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
			if (Monster && !Monster->IsDying())
			{
				UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
				if (TargetASC)
				{
					TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpecHandle.Data.Get());
					if (bDidCrit)
					{
						URID_DamageStatics::OnCritDamageOccurred.Broadcast(Monster, TotalFinalDamage);
					}
				}
			}
		}
	}

	// 6. 시각 효과 (캐릭터 - '1회성' 발동 이펙트)
	if (CasterEffect)
	{
		OwnerCharacter->Multicast_SpawnParticleAtLocation(
			CasterEffect,
			OwnerCharacter->GetActorLocation(), // 현재 위치에 스폰
			FRotator::ZeroRotator,              // 회전 없음
			FVector(1.0f)                       // 스케일 1.0
		);
	}

	// 7. 몽타주 재생 및 종료 바인딩
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, UltimateMontage);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_Warrior_Warpath::OnMontageFinished);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_Warrior_Warpath::OnMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_Warrior_Warpath::OnMontageCancelled);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_Warrior_Warpath::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}
	else
	{
		// 몽타주가 없으면 애니메이션 락은 즉시 해제
		OnMontageFinished();
	}
}

// --- [ ★★★ 추가 ★★★ ] ---
/** 버프 GE가 제거될 때 (지속시간 20초 만료 등) */
void UGA_Warrior_Warpath::OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo)
{
	// [★★★ 수정 ★★★] 캐릭터에게 이펙트 제거 명령
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(GetAvatarActorFromActorInfo());
	if (OwnerCharacter)
	{
		OwnerCharacter->Multicast_RemoveBuffEffect(BuffIsActiveTag);
	}

	RID_LOG(FColor::Yellow, TEXT("GA_Warrior_Warpath: Buff Removed. FX Cleared via Multicast."));

	// 2. 이 어빌리티 인스턴스를 정상 종료
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

/** 몽타주 재생이 '성공적으로' 끝났을 때 */
void UGA_Warrior_Warpath::OnMontageFinished()
{
	// --- [ ★★★ 수정 ★★★ ] ---
	// 몽타주가 끝나면 '애니메이션 락(State)'만 해제합니다.
	// 어빌리티 자체는 버프가 끝날 때까지(OnBuffEffectRemoved) 종료되지 않습니다.
	if (UltimateStateEffectHandle.IsValid() && GetCurrentActorInfo() && GetCurrentActorInfo()->AbilitySystemComponent.IsValid())
	{
		GetCurrentActorInfo()->AbilitySystemComponent->RemoveActiveGameplayEffect(UltimateStateEffectHandle);
		RID_LOG(FColor::Yellow, TEXT("GA_Warrior_Warpath: Montage Finished, removed State lock. Buff and Ability Instance continue."));
	}
	// EndAbility() 호출 제거
	// --- [ ★★★ 수정 끝 ★★★ ] ---
}

/** 몽타주가 '중단/취소'되었을 때 */
void UGA_Warrior_Warpath::OnMontageCancelled()
{
	// 몽타주가 취소되면 버프도 즉시 제거합니다.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

/** 어빌리티가 어떤 이유로든 종료될 때 (주로 취소 시 또는 버프 만료 시) */
void UGA_Warrior_Warpath::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	// (기존 로컬 BuffEffectComponent 정리 코드 삭제)

	// [★★★ 수정 ★★★] 취소되었을 때도 이펙트 확실히 제거
	if (bWasCancelled)
	{
		ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
		if (OwnerCharacter)
		{
			OwnerCharacter->Multicast_RemoveBuffEffect(BuffIsActiveTag);
		}
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