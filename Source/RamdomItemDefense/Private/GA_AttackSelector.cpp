#include "GA_AttackSelector.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h" // SendGameplayEventToActor
#include "GameplayEffectTypes.h" // FGameplayEventData
#include "RamdomItemDefense.h"

UGA_AttackSelector::UGA_AttackSelector()
{
	// �� �����Ƽ�� Event.Attack.Perform �±׷� Ȱ��ȭ�˴ϴ�. (BP���� ���� �ʿ�)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_AttackSelector::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// TriggerEventData�� ��ȿ���� Ȯ��
	if (!TriggerEventData)
	{
		RID_LOG(FColor::Red, TEXT("GA_AttackSelector: TriggerEventData is NULL!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// �������Ʈ���� ������ SelectAttackType �Լ� ȣ���Ͽ� ������ ���� �±� ����
	// TriggerEventData�� �����Ͽ� ���� (const ���� ȸ��)
	FGameplayEventData TempEventData = *TriggerEventData;
	FGameplayTag SelectedAttackTag = SelectAttackType(TempEventData);

	// ��ȿ�� �±װ� ��ȯ�Ǿ����� Ȯ��
	if (SelectedAttackTag.IsValid())
	{
		// --- [ �ڡڡ� �ڵ� ���� �ڡڡ� ] ---
		// ��� ���� �������� (const_cast ���)
		const AActor* ConstTargetActor = TriggerEventData->Target;
		AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
		// --- [ ���� �� ] ---

		if (TargetActor)
		{
			// ������ ���� ���� �̺�Ʈ ����
			SendExecuteAttackEvent(TargetActor, SelectedAttackTag);
			RID_LOG(FColor::Cyan, TEXT("GA_AttackSelector: Sending execute event: %s"), *SelectedAttackTag.ToString());
		}
		else
		{
			RID_LOG(FColor::Yellow, TEXT("GA_AttackSelector: TargetActor is NULL in TriggerEventData!"));
		}
	}
	else
	{
		RID_LOG(FColor::Orange, TEXT("GA_AttackSelector: SelectAttackType returned Invalid Tag. No attack executed."));
	}

	// ���� ���� �Ϸ� �� �����Ƽ ����
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// �������Ʈ���� ȣ���� �̺�Ʈ ���� �Լ� ����
void UGA_AttackSelector::SendExecuteAttackEvent(AActor* TargetActor, FGameplayTag ExecuteTag)
{
	// --- [����� 1] ---
	// �Լ��� ȣ��Ǿ�����, �Ķ���Ͱ� �ùٸ��� ���Դ��� Ȯ��
	// GetNameSafe()�� ���Ͱ� Null�� ��� "None"��, ��ȿ�� ��� ���� �̸��� �����ϰ� ��ȯ�մϴ�.
	RID_LOG(FColor::Cyan, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: CALLED. Target=[%s], ExecuteTag=[%s]"),
		*GetNameSafe(TargetActor),
		*ExecuteTag.ToString()
	);

	if (!TargetActor || !ExecuteTag.IsValid())
	{
		// --- [����� 2] ---
		// ��ȿ�� �˻翡 ������ ���� ���
		RID_LOG(FColor::Red, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: FAILED VALIDATION. TargetActor is %s, ExecuteTag is %s"),
			TargetActor ? TEXT("VALID") : TEXT("NULL"),
			ExecuteTag.IsValid() ? TEXT("VALID") : TEXT("INVALID")
		);
		return;
	}

	// ��� ���� ���� ���� Payload ����
	FGameplayEventData Payload;
	Payload.Target = TargetActor;
	Payload.Instigator = GetAvatarActorFromActorInfo();

	// --- [����� 3] ---
	// Payload�� �ùٸ��� �����Ǿ����� Ȯ��
	RID_LOG(FColor::Green, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: Payload created. Instigator=[%s], Target=[%s]"),
		*GetNameSafe(Payload.Instigator),
		*GetNameSafe(Payload.Target)
	);

	// ���� �����Ƽ �����ڿ��� Gameplay Event ����
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		// --- [����� 4] ---
		// ������(AvatarActor)�� ã�� �� ���� ġ���� ����
		RID_LOG(FColor::Red, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: FAILED. AvatarActor is NULL!"));
		return;
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, ExecuteTag, Payload);

	// --- [����� 5] ---
	// �̺�Ʈ�� ���������� ���۵Ǿ����� Ȯ��
	RID_LOG(FColor::Green, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: SUCCESS. Event [%s] SENT to [%s]"),
		*ExecuteTag.ToString(),
		*GetNameSafe(AvatarActor)
	);
}