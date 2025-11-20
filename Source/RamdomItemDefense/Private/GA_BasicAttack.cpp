	// Source/RamdomItemDefense/Private/GA_BasicAttack.cpp (수정)

#include "GA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h" // MyAttributeSet 헤더 포함
#include "RamdomItemDefenseCharacter.h" // 캐릭터 헤더 포함 (AttributeSet 가져오기 위해)
#include "GameplayEffectTypes.h" // FGameplayEventData 사용
#include "AbilitySystemBlueprintLibrary.h" // GetAbilitySystemComponent 사용
#include "RID_DamageStatics.h"
#include "RamdomItemDefense.h" // 로그 사용
#include "GameplayTagContainer.h" 

UGA_BasicAttack::UGA_BasicAttack()
{
	// 블루프린트에서 Activation Required Tags에 
	// .Execute.Basic 추가 필요
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 부모로부터 상속받은 변수들의 기본값 설정
	DamageBase = 0.0f; // 기본 공격은 기본 데미지 0
	DamageCoefficient = 1.0f; // 기본 계수는 1.0 (공격력 그대로)

	// 기본 데이터 태그 설정 (BP에서 변경 가능)
	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value")); // 또는 Skill.Damage.BasicAttack
}

/** * 어빌리티 활성화 시 (자식 클래스의 ActivateAbility보다 먼저) 호출됩니다.
 * MuzzleFlash 이펙트를 스폰하는 공통 로직을 처리합니다.
 */
void UGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// (GA_BaseSkill::ActivateAbility가 MuzzleFlash를 처리)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 필요한 데이터 확인 (데미지 이펙트, 타겟)
	if (!DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// --- [ ★★★ 로직 수정 ★★★ ] ---
	// 2. ASC 및 타겟 액터 가져오기
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!SourceASC || !TargetActor || !TargetASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 소유자(캐릭터 또는 드론)의 AttributeSet을 ASC에서 직접 가져오기
	const UMyAttributeSet* AttributeSet = Cast<const UMyAttributeSet>(SourceASC->GetAttributeSet(UMyAttributeSet::StaticClass()));
	if (!AttributeSet)
	{
		// (이전에 실패했던 지점)
		RID_LOG(FColor::Red, TEXT("GA_BasicAttack: FAILED to get UMyAttributeSet from SourceASC."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// --- [ ★★★ 로직 수정 끝 ★★★ ] ---

	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();

	// 4-1. 기본 데미지 계산 (새 공식 적용)
	const float BaseDamage = DamageBase + (OwnerAttackDamage * DamageCoefficient);

	// 4-2. 치명타 적용 (bIsSkillAttack: false)
	// (RID_DamageStatics::ApplyCritDamage는 이미 수정된 GetAttributeSetFromASC를 사용하므로 안전)
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, false);

	RID_LOG(FColor::White, TEXT("GA_BasicAttack: Applying Damage: %.1f (AD: %.1f * Coeff: %.1f) -> CritApplied: %.1f"), BaseDamage, OwnerAttackDamage, DamageCoefficient, FinalDamage);

	// 5. 데미지 GE Spec 생성 및 SetByCaller로 최종 데미지 값 주입
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (SpecHandle.IsValid() && DamageDataTag.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
	else
	{
		if (!DamageDataTag.IsValid())
		{
			UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_BasicAttack: DamageDataTag is Invalid! Check Blueprint settings."));
		}
	}

	// 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}