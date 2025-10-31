#include "GA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h" // MyAttributeSet 헤더 포함
#include "RamdomItemDefenseCharacter.h" // 캐릭터 헤더 포함 (AttributeSet 가져오기 위해)
#include "GameplayEffectTypes.h" // FGameplayEventData 사용
#include "AbilitySystemBlueprintLibrary.h" // GetAbilitySystemComponent 사용
#include "RID_DamageStatics.h"
#include "RamdomItemDefense.h" // 로그 사용

UGA_BasicAttack::UGA_BasicAttack()
{
	// 블루프린트에서 Activation Required Tags에 Event.Attack.Execute.Basic 추가 필요
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	DamageCoefficient = 1.0f; // 기본 계수는 1.0 (공격력 그대로)
	// 기본 데이터 태그 설정 (BP에서 변경 가능)
	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value")); // 또는 Skill.Damage.BasicAttack
}

void UGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 필요한 데이터 확인 (데미지 이펙트, 타겟)
	if (!DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// --- [ ★★★ 수정 시작 ★★★ ] ---
	// TriggerEventData와 그 안의 Target 포인터 유효성 검사
	if (!TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// --- [ 수정 끝 ] ---

	// 2. 소유자 캐릭터, ASC, 타겟 액터 가져오기
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	// --- [ ★★★ 수정 시작 ★★★ ] ---
	// const AActor* 를 먼저 가져온 후 const_cast 적용
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
	// --- [ 수정 끝 ] ---

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor); // 수정된 TargetActor 사용

	if (!OwnerCharacter || !SourceASC || !TargetActor || !TargetASC)
	{
		// TargetActor 유효성 검사 추가 (const_cast 실패 대비는 아니지만 안전하게)
		if (!TargetActor) UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_BasicAttack: TargetActor became NULL after const_cast (Should not happen)."));

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 소유자의 공격력 가져오기
	const UMyAttributeSet* AttributeSet = OwnerCharacter->GetAttributeSet();
	if (!AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();

	// 4-1. 기본 데미지 계산
	const float BaseDamage = OwnerAttackDamage * DamageCoefficient;

	// 4-2. 치명타 적용 (bIsSkillAttack: false)
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, false);

	RID_LOG(FColor::White, TEXT("GA_BasicAttack: Applying Damage: %.1f (AD: %.1f * Coeff: %.1f) -> CritApplied: %.1f"), BaseDamage, OwnerAttackDamage, DamageCoefficient, FinalDamage);
	// 5. 데미지 GE Spec 생성 및 SetByCaller로 최종 데미지 값 주입
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (SpecHandle.IsValid() && DamageDataTag.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);

		// 6. 대상에게 GE 적용
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