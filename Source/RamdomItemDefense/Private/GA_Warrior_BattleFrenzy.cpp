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
	const float AttackSpeedBuffValue = CasterAttackDamage * AttackSpeedCoefficient;

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

	// 6. [갱신] 이미 버프가 활성화되어 있는지(BuffIsActiveTag 태그가 있는지) 확인
	if (CasterASC->HasMatchingGameplayTag(BuffIsActiveTag))
	{
		// 6-1. [갱신] 이미 태그가 있다면, 이 어빌리티는 버프 '갱신'만 담당
		// (GE 블루프린트 설정 덕분에) 단순히 GE를 다시 적용하는 것만으로
		// 기존 GE의 지속시간과 SetByCaller 값이 갱신됩니다.
		CasterASC->ApplyGameplayEffectSpecToSelf(*BuffSpecHandle.Data.Get());

		RID_LOG(FColor::Green, TEXT("GA_Warrior_BattleFrenzy: [Refreshed] AttackSpeed Buff: +%.2f (%.1f AD * %.2f Coeff) for %.1fs"),
			AttackSpeedBuffValue, CasterAttackDamage, AttackSpeedCoefficient, BuffDuration);

		// 6-2. [갱신] 이 어빌리티(새 인스턴스)는 할 일을 다 했으므로 즉시 종료
		// (기존 버프를 관리하던 *이전* 어빌리티 인스턴스는 계속 살아있음)
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
	else
	{
		// 7-1. [신규] 버프 GE 적용
		ActiveBuffEffectHandle = CasterASC->ApplyGameplayEffectSpecToSelf(*BuffSpecHandle.Data.Get());

		// 7-2. [신규] 버프 이펙트 스폰
		if (BuffEffect)
		{
			ActiveBuffFXComponent = UGameplayStatics::SpawnEmitterAttached(
				BuffEffect,
				OwnerCharacter->GetMesh(),
				AttachSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true
			);
		}

		// 7-3. [신규] 버프 GE가 제거될 때까지 대기하는 태스크 등록
		if (ActiveBuffEffectHandle.IsValid())
		{
			UAbilityTask_WaitGameplayEffectRemoved* WaitEffectRemovedTask = UAbilityTask_WaitGameplayEffectRemoved::WaitForGameplayEffectRemoved(this, ActiveBuffEffectHandle);
			WaitEffectRemovedTask->OnRemoved.AddDynamic(this, &UGA_Warrior_BattleFrenzy::OnBuffEffectRemoved);
			WaitEffectRemovedTask->ReadyForActivation();
		}

		RID_LOG(FColor::Green, TEXT("GA_Warrior_BattleFrenzy: [Applied New] AttackSpeed Buff: +%.2f (%.1f AD * %.2f Coeff) for %.1fs"),
			AttackSpeedBuffValue, CasterAttackDamage, AttackSpeedCoefficient, BuffDuration);

		// 7-4. [신규] 어빌리티를 종료하지 않고 태스크가 완료(버프가 제거)되기를 기다림
		// (EndAbility() 호출 없음)
	}
	// --- [ ★★★ 로직 수정 끝 ★★★ ] ---
}

void UGA_Warrior_BattleFrenzy::OnBuffEffectRemoved(const FGameplayEffectRemovalInfo& EffectRemovalInfo)
{
	// 버프 GE가 제거될 때 (지속시간이 끝나거나, 갱신을 위해 수동 제거되거나)
	// 이펙트를 제거하고, 이 어빌리티 인스턴스를 종료합니다.

	if (ActiveBuffFXComponent && ActiveBuffFXComponent->IsValidLowLevel())
	{
		ActiveBuffFXComponent->DestroyComponent();
		ActiveBuffFXComponent = nullptr;
		RID_LOG(FColor::Yellow, TEXT("GA_Warrior_BattleFrenzy: Buff Effect removed."));
	}

	// --- [ ★★★ 추가 ★★★ ] ---
	// 이 어빌리티 인스턴스를 정상 종료
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	// --- [ ★★★ 추가 끝 ★★★ ] ---
}

void UGA_Warrior_BattleFrenzy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// bWasCancelled이 true일 경우, 즉 어빌리티가 중간에 취소된 경우
	if (bWasCancelled)
	{
		UAbilitySystemComponent* CasterASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (CasterASC && ActiveBuffEffectHandle.IsValid())
		{
			// 적용했던 GE를 강제로 제거
			CasterASC->RemoveActiveGameplayEffect(ActiveBuffEffectHandle);
			ActiveBuffEffectHandle.Invalidate();
		}
		// 이펙트도 강제 제거
		if (ActiveBuffFXComponent && ActiveBuffFXComponent->IsValidLowLevel())
		{
			ActiveBuffFXComponent->DestroyComponent();
			ActiveBuffFXComponent = nullptr;
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}