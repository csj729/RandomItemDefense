#include "GA_Archer_UltimateFocus.h"
#include "AbilitySystemComponent.h"
#include "MyPlayerState.h" 
#include "RamdomItemDefenseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "RamdomItemDefense.h" // 로그용
#include "Sound/SoundBase.h" // 사운드용

UGA_Archer_UltimateFocus::UGA_Archer_UltimateFocus()
{
	BuffDuration = 20.0f; // 20초 버프 지속
	BuffIsActiveTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Archer.FocusActive"));
	HeadBuffIsActiveTag = FGameplayTag::RequestGameplayTag(FName("State.Player.Archer.FocusActive.Head"));
}

void UGA_Archer_UltimateFocus::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 1. 부모의 ActivateAbility 호출 (스택 리셋)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

	if (!OwnerCharacter || !SourceASC || !UltimateBuffEffectClass)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_UltimateFocus: [ACTIVATE FAILED] OwnerCharacter, SourceASC, or UltimateBuffEffectClass is NULL."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_UltimateFocus: [ACTIVATED] Applying buff and visual effects."));

	// 2. 궁극기 버프 GE 적용
	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(UltimateBuffEffectClass, 1.0f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		UltimateBuffEffectHandle = SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	else
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_UltimateFocus: [ACTIVATE FAILED] Failed to create UltimateBuffEffectSpec."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!UltimateBuffEffectHandle.IsValid())
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("GA_Archer_UltimateFocus: [ACTIVATE FAILED] Applying UltimateBuffEffectSpec failed."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	// --- [ 시각 및 청각 효과 (3 파티클 + 1 사운드) ] ---

	// A. 1회성 발동 이펙트 스폰 (부착 안 함)
	if (ActivationEffect)
	{
		OwnerCharacter->Multicast_SpawnParticleAtLocation(
			ActivationEffect,
			OwnerCharacter->GetActorLocation(),
			OwnerCharacter->GetActorRotation(),
			FVector(1.0f)
		);
	}

	// 1. 몸 이펙트 (Body)
	if (BuffParticleSystem)
	{
		OwnerCharacter->Multicast_AddBuffEffect(
			BuffIsActiveTag,
			BuffParticleSystem,
			NAME_None,
			FVector::ZeroVector,
			FVector(1.5f) // Scale 1.5
		);
	}

	// 2. 머리 이펙트 (Head)
	if (HeadBuffParticleSystem)
	{
		// HeadBuffIsActiveTag가 유효한지 확인 (없으면 Body 태그와 겹쳐서 하나만 뜰 수 있음)
		if (HeadBuffIsActiveTag.IsValid())
		{
			OwnerCharacter->Multicast_AddBuffEffect(
				HeadBuffIsActiveTag,
				HeadBuffParticleSystem,
				TEXT("FX_Head"), // 소켓 이름
				FVector(50, 0, 0),
				FVector(1.0f)
			);
		}
	}

	// D. 사운드 재생
	if (BuffStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BuffStartSound, OwnerCharacter->GetActorLocation());
	}


	// 4. 버프 지속 시간 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		BuffDurationTimerHandle,
		this,
		&UGA_Archer_UltimateFocus::OnBuffDurationEnded,
		BuffDuration,
		false // 반복 없음
	);
}

/** 버프 지속 시간이 끝났을 때 호출되는 함수 */
void UGA_Archer_UltimateFocus::OnBuffDurationEnded()
{
	UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_UltimateFocus: [BUFF ENDED] Duration finished."));
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

/** 어빌리티 종료 시 호출 (버프 GE 및 파티클 제거) */
void UGA_Archer_UltimateFocus::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (OwnerCharacter)
	{
		OwnerCharacter->Multicast_RemoveBuffEffect(BuffIsActiveTag);
		OwnerCharacter->Multicast_RemoveBuffEffect(HeadBuffIsActiveTag);
	}

	// 타이머가 혹시라도 아직 돌고 있었다면 중지
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BuffDurationTimerHandle);
	}

	// 5. 부모의 EndAbility 호출 (필수)
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}