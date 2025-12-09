
#include "GA_UltimateSkill.h" 
#include "RamdomItemDefense.h" 
#include "RamdomItemDefenseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "MyPlayerState.h"

UGA_UltimateSkill::UGA_UltimateSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

// (★★★) CanActivateAbility 함수 구현
bool UGA_UltimateSkill::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	// 1. 부모 클래스의 CanActivateAbility를 먼저 확인합니다. (태그, 쿨다운, 코스트 등)
	// (이 함수는 const 함수이므로 로그를 남기려면 RID_LOG 대신 UE_LOG를 사용해야 할 수 있습니다)
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 2. ActorInfo가 유효한지 확인합니다.
	if (!ActorInfo)
	{
		return false;
	}

	// 3. (중요) 스택 확인 로직
	// (const 함수 내에서는 const 버전의 캐릭터와 PlayerState를 사용해야 합니다)
	const ARamdomItemDefenseCharacter* OwnerCharacter = Cast<const ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	if (!OwnerCharacter)
	{
		return false;
	}

	const AMyPlayerState* PS = OwnerCharacter->GetPlayerState<AMyPlayerState>();
	if (!PS)
	{
		return false;
	}

	// 4. 스택이 최대치인지 확인하여 true/false 반환
	return (PS->GetUltimateCharge() >= PS->GetMaxUltimateCharge());
}

void UGA_UltimateSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()) {
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 0. 캐릭터 및 PlayerState 가져오기
	AMyPlayerState* PS = nullptr;

	// 캐릭터로 캐스팅
	if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get()))
	{
		// [추가] 1. 궁극기 시전 사운드 재생
		// 캐릭터마다 설정된(전사, 궁수 등) 고유 소리를 재생합니다.
		if (OwnerCharacter->UltimateActivateSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, OwnerCharacter->UltimateActivateSound, OwnerCharacter->GetActorLocation());
		}

		// PlayerState 가져오기
		PS = OwnerCharacter->GetPlayerState<AMyPlayerState>();
	}

	// PlayerState가 없으면 종료
	if (!PS) {
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. (C++ 역할 2) 스택 리셋 (코스트 소모)
	PS->ResetUltimateCharge();

}

/** 몽타주가 어떤 이유로든 종료되었을 때 (파라미터 없음) */
void UGA_UltimateSkill::OnMontageEnded()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

/** 어빌리티가 종료될 때 (성공, 취소, 중단 모두 포함) */
void UGA_UltimateSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 2. (필수) 부모의 EndAbility를 호출하여 어빌리티를 완전히 종료합니다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}