#include "GA_Archer_UltimateFocus.h" // 이름 변경
#include "AbilitySystemComponent.h"
#include "MyPlayerState.h" 
#include "RamdomItemDefenseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

UGA_Archer_UltimateFocus::UGA_Archer_UltimateFocus() // 이름 변경
{
	BuffEffectComponent = nullptr;
}

void UGA_Archer_UltimateFocus::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) // 이름 변경
{
	// 1. 부모의 ActivateAbility 호출 (스택 리셋, 'IsUsingUltimate' 태그 적용)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!OwnerCharacter || !SourceASC || !FocusBuffEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 20초간 지속되는 실제 스탯 버프 GE 적용
	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FocusBuffEffectHandle = SourceASC->ApplyGameplayEffectToSelf(FocusBuffEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);

	if (!FocusBuffEffectHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. '사용 시' 1회성 이펙트 스폰
	if (ActivationEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ActivationEffect, OwnerCharacter->GetActorLocation(), OwnerCharacter->GetActorRotation());
	}

	// 4. '지속 중' 파티클을 캐릭터에게 부착
	if (BuffEffect && OwnerCharacter->GetMesh())
	{
		BuffEffectComponent = UGameplayStatics::SpawnEmitterAttached(
			BuffEffect,
			OwnerCharacter->GetMesh(),
			NAME_None, // (소켓 이름 필요시 여기에)
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget
		);
	}

	// 5. 20초 뒤에 OnBuffDurationEnded 함수를 호출하도록 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		BuffDurationTimerHandle,
		this,
		&UGA_Archer_UltimateFocus::OnBuffDurationEnded,
		20.0f, // 20초
		false
	);
}

/** 20초가 만료되었을 때 호출될 함수 */
void UGA_Archer_UltimateFocus::OnBuffDurationEnded() // 이름 변경 (부모)
{
	// 어빌리티를 정상적으로 종료시킵니다.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

/** 궁극기가 종료될 때 (20초 만료 또는 취소 시) */
void UGA_Archer_UltimateFocus::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) // 이름 변경
{
	// --- [ 클린업 로직 ] ---

	// 1. 부착된 파티클 이펙트가 있다면 제거(Deactivate)
	if (BuffEffectComponent)
	{
		BuffEffectComponent->Deactivate();
		BuffEffectComponent = nullptr;
	}

	// 2. 스탯 버프 GE가 적용된 상태라면 제거
	if (FocusBuffEffectHandle.IsValid())
	{
		if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(FocusBuffEffectHandle);
		}
	}

	// 3. 타이머가 혹시 돌고 있었다면 중지
	GetWorld()->GetTimerManager().ClearTimer(BuffDurationTimerHandle);

	// 4. (필수) 부모의 EndAbility를 호출합니다.
	// (이 함수가 'IsUsingUltimate' 태그가 있는 UltimateStateEffectHandle을 제거해줍니다)
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}