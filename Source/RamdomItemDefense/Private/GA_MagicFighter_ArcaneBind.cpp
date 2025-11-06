// Source/RamdomItemDefense/Private/GA_MagicFighter_ArcaneBind.cpp (수정)

#include "GA_MagicFighter_ArcaneBind.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RamdomItemDefense.h"
#include "RID_DamageStatics.h" 

UGA_MagicFighter_ArcaneBind::UGA_MagicFighter_ArcaneBind()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	// 부모 클래스의 변수에 기본값 설정
	DamageBase = 200.0f;
	DamageCoefficient = 0.5f; // (50%)
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
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

	// --- [ ★★★ 코드 수정 ★★★ ] ---
	// 4. 최종 데미지 계산 (부모 변수 사용)
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();

	// 4-1. 기본 데미지 계산 (양수)
	// 하드코딩된 값 대신 부모의 변수(DamageBase, DamageCoefficient) 사용
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);

	// 4-2. 치명타 적용 (bIsSkillAttack: true)
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, true);

	RID_LOG(FColor::Cyan, TEXT("GA_ArcaneBind: Applying Damage: %.1f (Base: %.1f, AD: %.1f * %.1f) -> CritApplied: %.1f"), BaseDamage, DamageBase, OwnerAttackDamage, DamageCoefficient, FinalDamage);
	// --- [ ★★★ 코드 수정 끝 ★★★ ] ---

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