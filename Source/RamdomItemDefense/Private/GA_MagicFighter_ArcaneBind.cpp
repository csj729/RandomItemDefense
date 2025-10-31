// Source/RamdomItemDefense/Private/GA_MagicFighter_ArcaneBind.cpp (����)

#include "GA_MagicFighter_ArcaneBind.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "RamdomItemDefense.h"
// --- [�ڵ� �߰�] ---
#include "RID_DamageStatics.h" // ������ ��� ���� ����
// --- [�ڵ� �߰� ��] ---

UGA_MagicFighter_ArcaneBind::UGA_MagicFighter_ArcaneBind()
{
	// �������Ʈ���� Activation Required Tags�� Event.Attack.Execute.Skill3 �߰� �ʿ�
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.05f;
}

void UGA_MagicFighter_ArcaneBind::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	RID_LOG(FColor::Magenta, TEXT("==== GA_ArcaneBind: ActivateAbility Called! ===="));

	// 1. �������� Ȯ��
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. �ʿ��� ������ Ȯ�� (GE Ŭ����, Ÿ��)
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

	// 3. ������ ASC, Ÿ��, Ÿ�� ASC, �Ӽ��� ��������
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

	// 4. ���� ������ ���
	// --- [�ڵ� ����] ---
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();

	// 4-1. �⺻ ������ ��� (���)
	const float BaseDamage = 200.0f + (OwnerAttackDamage * 0.5f);

	// 4-2. ġ��Ÿ ���� (bIsSkillAttack: true)
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, true);

	RID_LOG(FColor::Cyan, TEXT("GA_ArcaneBind: Applying Damage: %.1f (Base: 200, AD: %.1f * 0.5) -> CritApplied: %.1f"), BaseDamage, OwnerAttackDamage, FinalDamage);
	// --- [�ڵ� ���� ��] ---

	// 5. ������ GE Spec ���� �� SetByCaller�� ���� ������ �� ����
	FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (DamageSpecHandle.IsValid() && DamageByCallerTag.IsValid())
	{
		// 5-1. ���� �������� ������ ��ȯ�Ͽ� ����
		DamageSpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageByCallerTag, -FinalDamage);

		// 6. ��󿡰� ������ GE ����
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

	// 7. ���� GE Spec ���� �� ����
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

	// �����Ƽ ����
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}