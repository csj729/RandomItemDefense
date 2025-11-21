// Private/GA_Warrior_BattleFrenzy.cpp (수정)

#include "GA_Warrior_BattleFrenzy.h"
#include "RamdomItemDefense.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h" 
#include "Kismet/GameplayStatics.h" 
#include "Particles/ParticleSystemComponent.h" 
#include "Abilities/Tasks/AbilityTask_WaitGameplayEffectRemoved.h" 

UGA_Warrior_BattleFrenzy::UGA_Warrior_BattleFrenzy()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.10f;
	BuffDuration = 5.0f;
	AttackSpeedCoefficient = 0.1f;
	AttackSpeedBuffTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Warrior.AttackSpeed"));
	AttachSocketName = FName("FX_Head");

	// --- [ ★★★ 추가 ★★★ ] ---
	// 이 어빌리티가 활성화되었음을 나타내는 태그
	BuffIsActiveTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Warrior.AttackSpeed.Active"));
	// --- [ ★★★ 추가 끝 ★★★ ] ---

	DamageBase = 0.f;
	DamageCoefficient = 0.f;

	ActiveBuffFXComponent = nullptr;
}

void UGA_Warrior_BattleFrenzy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 서버 확인
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. GE 클래스 유효성 검사
	if (!AttackSpeedBuffEffectClass || !AttackSpeedBuffTag.IsValid() || !BuffIsActiveTag.IsValid())
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Warrior_BattleFrenzy: AttackSpeedBuffEffectClass or AttackSpeedBuffTag (%s) or BuffIsActiveTag (%s) is not set properly!"),
			*AttackSpeedBuffTag.ToString(), *BuffIsActiveTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 시전자 ASC 및 속성셋 가져오기
	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(CasterASC->GetAttributeSet(UMyAttributeSet::StaticClass()));

	if (!CasterASC || !AttributeSet || !OwnerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 4. 버프량 계산 (갱신이든 신규든 항상 최신값으로 계산)
	const float CasterAttackDamage = AttributeSet->GetAttackDamage();
	const float AttackSpeedBuffValue = 0.5f + CasterAttackDamage * AttackSpeedCoefficient;

	// 5. 버프 GE Spec 생성
	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
	FGameplayEffectSpecHandle BuffSpecHandle = CasterASC->MakeOutgoingSpec(AttackSpeedBuffEffectClass, 1.0f, ContextHandle);

	if (!BuffSpecHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 5-1. SetByCaller로 공속 버프량 주입
	BuffSpecHandle.Data.Get()->SetSetByCallerMagnitude(AttackSpeedBuffTag, AttackSpeedBuffValue);

	// --- [ ★★★ 핵심 로직 수정 ★★★ ] ---

// 6. [갱신] 이미 버프가 활성화되어 있는지 확인
	if (CasterASC->HasMatchingGameplayTag(BuffIsActiveTag))
	{
		// 6-1. [갱신] GE만 다시 적용 (지속시간 갱신)
		CasterASC->ApplyGameplayEffectSpecToSelf(*BuffSpecHandle.Data.Get());

		// **중요**: 이미 이펙트가 떠 있을 것이므로, 이펙트 재생 로직은 건너뜁니다.
		RID_LOG(FColor::Green, TEXT("GA_Warrior_BattleFrenzy: [Refreshed]"));

		// 6-2. 종료
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
	else
	{
		// 7-1. [신규] 버프 GE 적용
		ActiveBuffEffectHandle = CasterASC->ApplyGameplayEffectSpecToSelf(*BuffSpecHandle.Data.Get());

		// 7-2. [신규] ★★★ 캐릭터에게 모든 클라이언트에서 이펙트를 켜라고 명령 ★★★
		if (BuffEffect)
		{
			// BuffIsActiveTag를 키(Key)로 사용하여 이펙트를 등록합니다.
			OwnerCharacter->Multicast_AddBuffEffect(
				BuffIsActiveTag,
				BuffEffect,
				AttachSocketName,
				FVector::ZeroVector,
				FVector(1.0f)
			);
		}

		// 7-3. [신규] 버프 GE가 제거될 때까지 대기
		if (ActiveBuffEffectHandle.IsValid())
		{
			UAbilityTask_WaitGameplayEffectRemoved* WaitEffectRemovedTask = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, ActiveBuffEffectHandle);
			WaitEffectRemovedTask->OnRemoved.AddDynamic(this, &UGA_Warrior_BattleFrenzy::OnBuffEffectRemoved);
			WaitEffectRemovedTask->ReadyForActivation();
		}

		RID_LOG(FColor::Green, TEXT("GA_Warrior_BattleFrenzy: [New Start]"));
	}
}

void UGA_Warrior_BattleFrenzy::OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo)
{
	// [수정] 로컬 변수 제어 대신 캐릭터에게 이펙트 제거 명령
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(GetAvatarActorFromActorInfo());
	if (OwnerCharacter)
	{
		OwnerCharacter->Multicast_RemoveBuffEffect(BuffIsActiveTag);
	}

	RID_LOG(FColor::Yellow, TEXT("GA_Warrior_BattleFrenzy: Buff Removed. FX Cleared via Multicast."));

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Warrior_BattleFrenzy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// [수정] 취소되었을 때도 이펙트 확실히 제거
	if (bWasCancelled)
	{
		UAbilitySystemComponent* CasterASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (CasterASC && ActiveBuffEffectHandle.IsValid())
		{
			CasterASC->RemoveActiveGameplayEffect(ActiveBuffEffectHandle);
		}

		// 캐릭터에게 이펙트 제거 명령
		ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
		if (OwnerCharacter)
		{
			OwnerCharacter->Multicast_RemoveBuffEffect(BuffIsActiveTag);
		}
	}

	// (기존 로직: 로컬 변수 ActiveBuffFXComponent->DestroyComponent() 는 삭제)

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}