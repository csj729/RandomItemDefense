#include "GA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "MyAttributeSet.h" // MyAttributeSet ��� ����
#include "RamdomItemDefenseCharacter.h" // ĳ���� ��� ���� (AttributeSet �������� ����)
#include "GameplayEffectTypes.h" // FGameplayEventData ���
#include "AbilitySystemBlueprintLibrary.h" // GetAbilitySystemComponent ���
#include "RID_DamageStatics.h"
#include "RamdomItemDefense.h" // �α� ���

UGA_BasicAttack::UGA_BasicAttack()
{
	// �������Ʈ���� Activation Required Tags�� Event.Attack.Execute.Basic �߰� �ʿ�
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	DamageCoefficient = 1.0f; // �⺻ ����� 1.0 (���ݷ� �״��)
	// �⺻ ������ �±� ���� (BP���� ���� ����)
	DamageDataTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value")); // �Ǵ� Skill.Damage.BasicAttack
}

void UGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. �ʿ��� ������ Ȯ�� (������ ����Ʈ, Ÿ��)
	if (!DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// --- [ �ڡڡ� ���� ���� �ڡڡ� ] ---
	// TriggerEventData�� �� ���� Target ������ ��ȿ�� �˻�
	if (!TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// --- [ ���� �� ] ---

	// 2. ������ ĳ����, ASC, Ÿ�� ���� ��������
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	// --- [ �ڡڡ� ���� ���� �ڡڡ� ] ---
	// const AActor* �� ���� ������ �� const_cast ����
	const AActor* ConstTargetActor = TriggerEventData->Target;
	AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
	// --- [ ���� �� ] ---

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor); // ������ TargetActor ���

	if (!OwnerCharacter || !SourceASC || !TargetActor || !TargetASC)
	{
		// TargetActor ��ȿ�� �˻� �߰� (const_cast ���� ���� �ƴ����� �����ϰ�)
		if (!TargetActor) UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_BasicAttack: TargetActor became NULL after const_cast (Should not happen)."));

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. �������� ���ݷ� ��������
	const UMyAttributeSet* AttributeSet = OwnerCharacter->GetAttributeSet();
	if (!AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	const float OwnerAttackDamage = AttributeSet->GetAttackDamage();

	// 4-1. �⺻ ������ ���
	const float BaseDamage = OwnerAttackDamage * DamageCoefficient;

	// 4-2. ġ��Ÿ ���� (bIsSkillAttack: false)
	const float FinalDamage = URID_DamageStatics::ApplyCritDamage(BaseDamage, SourceASC, TargetActor, false);

	RID_LOG(FColor::White, TEXT("GA_BasicAttack: Applying Damage: %.1f (AD: %.1f * Coeff: %.1f) -> CritApplied: %.1f"), BaseDamage, OwnerAttackDamage, DamageCoefficient, FinalDamage);
	// 5. ������ GE Spec ���� �� SetByCaller�� ���� ������ �� ����
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (SpecHandle.IsValid() && DamageDataTag.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageDataTag, -FinalDamage);

		// 6. ��󿡰� GE ����
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
	else
	{
		if (!DamageDataTag.IsValid())
		{
			UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_BasicAttack: DamageDataTag is Invalid! Check Blueprint settings."));
		}
	}

	// �����Ƽ ����
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}