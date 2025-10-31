// Source/RamdomItemDefense/Private/GA_MagicFighter_ArcaneBind.cpp (수정)

#include "GA_MagicFighter_ArcaneBind.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RamdomItemDefense.h"
// --- [코드 추가] ---
#include "RID_DamageStatics.h" // 데미지 계산 헬퍼 포함
// --- [코드 추가 끝] ---

UGA_MagicFighter_ArcaneBind::UGA_MagicFighter_ArcaneBind()
{
	// 블루프린트에서 Activation Required Tags에 Event.Attack.Execute.Skill3 추가 필요
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;
}

void UGA_MagicFighter_ArcaneBind::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	RID_LOG(FColor::Magenta, TEXT("==== GA_ArcaneBind: ActivateAbility Called! ===="));

	// 1. 서버인지 확인
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 필요한 데이터 확인 (GE 클래스, 타겟)
	if (!DamageEffectClass || !StunEffectClass)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_ArcaneBind: DamageEffectClass or StunEffectClass is not set in Blueprint."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!TriggerEventData || !TriggerEventData->Target)
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_ArcaneBind: TriggerEventData or Target is NULL."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 소유자 ASC, 타겟, 타겟 ASC, 속성셋 가져오기
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	const AActor* ConstTargetActor = TriggerEventData->Target.Get();
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	const UMyAttributeSet* AttributeSet = OwnerCharacter->GetAttributeSet();

	if (!SourceASC || !TargetActor || !TargetASC || !AttributeSet)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_ArcaneBind: Failed to get ASC, Target, or AttributeSet."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 4. 최종 데미지 계산
	// --- [코드 수정] ---
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();

	// 4-1. 기본 데미지 계산 (양수)
	const float BaseDamage = 200.0f + (OwnerAttackDamage * 0.5f);

	// 4-2. 치명타 적용 (bIsSkillAttack: true)
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, true);

	RID_LOG(FColor::Cyan, TEXT("GA_ArcaneBind: Applying Damage: %.1f (Base: 200, AD: %.1f * 0.5) -> CritApplied: %.1f"), BaseDamage, OwnerAttackDamage, FinalDamage);
	// --- [코드 수정 끝] ---

	// 5. 데미지 GE Spec 생성 및 SetByCaller로 최종 데미지 값 주입
	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (DamageSpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		// 5-1. 최종 데미지를 음수로 변환하여 적용
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);

		// 6. 대상에게 데미지 GE 적용
		RID_LOG(FColor::Green, TEXT("GA_ArcaneBind: Applying Damage Spec to %s..."), *GetNameSafe(TargetActor));
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
	}
	else
	{
		if (!DamageByCallerTag.IsValid())
		{
			UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_ArcaneBind: DamageByCallerTag is Invalid! Check Blueprint settings."));
		}
	}

	// 7. 스턴 GE Spec 생성 및 적용
	FGameplayEffectSpecHandle StunSpecHandle = SourceASC->MakeOutgoingSpec(StunEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (StunSpecHandle.IsValid())
	{
		RID_LOG(FColor::Green, TEXT("GA_ArcaneBind: Applying Stun Spec to %s..."), *GetNameSafe(TargetActor));
		SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpecHandle.Data.Get(), TargetASC);
	}
	else
	{
		UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_ArcaneBind: Failed to make Stun SpecHandle."));
	}

	// 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}