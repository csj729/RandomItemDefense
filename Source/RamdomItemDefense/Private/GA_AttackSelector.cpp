#include "GA_AttackSelector.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h" // SendGameplayEventToActor
#include "GameplayEffectTypes.h" // FGameplayEventData
#include "RamdomItemDefense.h"

UGA_AttackSelector::UGA_AttackSelector()
{
	// 이 어빌리티는 Event.Attack.Perform 태그로 활성화됩니다. (BP에서 설정 필요)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_AttackSelector::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// TriggerEventData가 유효한지 확인
	if (!TriggerEventData)
	{
		RID_LOG(FColor::Red, TEXT("GA_AttackSelector: TriggerEventData is NULL!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 블루프린트에서 구현된 SelectAttackType 함수 호출하여 실행할 공격 태그 결정
	// TriggerEventData를 복사하여 전달 (const 문제 회피)
	FGameplayEventData TempEventData = *TriggerEventData;
	FGameplayTag SelectedAttackTag = SelectAttackType(TempEventData);

	// 유효한 태그가 반환되었는지 확인
	if (SelectedAttackTag.IsValid())
	{
		// --- [ ★★★ 코드 수정 ★★★ ] ---
		// 대상 액터 가져오기 (const_cast 사용)
		const AActor* ConstTargetActor = TriggerEventData->Target;
		AActor* TargetActor = const_cast<AActor*>(ConstTargetActor);
		// --- [ 수정 끝 ] ---

		if (TargetActor)
		{
			// 결정된 공격 실행 이벤트 전송
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

	// 선택 로직 완료 후 어빌리티 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// 블루프린트에서 호출할 이벤트 전송 함수 구현
void UGA_AttackSelector::SendExecuteAttackEvent(AActor* TargetActor, FGameplayTag ExecuteTag)
{
	// --- [디버깅 1] ---
	// 함수가 호출되었는지, 파라미터가 올바르게 들어왔는지 확인
	// GetNameSafe()는 액터가 Null일 경우 "None"을, 유효할 경우 액터 이름을 안전하게 반환합니다.
	RID_LOG(FColor::Cyan, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: CALLED. Target=[%s], ExecuteTag=[%s]"),
		*GetNameSafe(TargetActor),
		*ExecuteTag.ToString()
	);

	if (!TargetActor || !ExecuteTag.IsValid())
	{
		// --- [디버깅 2] ---
		// 유효성 검사에 실패한 이유 출력
		RID_LOG(FColor::Red, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: FAILED VALIDATION. TargetActor is %s, ExecuteTag is %s"),
			TargetActor ? TEXT("VALID") : TEXT("NULL"),
			ExecuteTag.IsValid() ? TEXT("VALID") : TEXT("INVALID")
		);
		return;
	}

	// 대상 정보 등을 담을 Payload 생성
	FGameplayEventData Payload;
	Payload.Target = TargetActor;
	Payload.Instigator = GetAvatarActorFromActorInfo();

	// --- [디버깅 3] ---
	// Payload가 올바르게 생성되었는지 확인
	RID_LOG(FColor::Green, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: Payload created. Instigator=[%s], Target=[%s]"),
		*GetNameSafe(Payload.Instigator),
		*GetNameSafe(Payload.Target)
	);

	// 현재 어빌리티 소유자에게 Gameplay Event 전송
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		// --- [디버깅 4] ---
		// 시전자(AvatarActor)를 찾을 수 없는 치명적 오류
		RID_LOG(FColor::Red, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: FAILED. AvatarActor is NULL!"));
		return;
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, ExecuteTag, Payload);

	// --- [디버깅 5] ---
	// 이벤트가 성공적으로 전송되었음을 확인
	RID_LOG(FColor::Green, TEXT("[GA_AttackSelector] SendExecuteAttackEvent: SUCCESS. Event [%s] SENT to [%s]"),
		*ExecuteTag.ToString(),
		*GetNameSafe(AvatarActor)
	);
}