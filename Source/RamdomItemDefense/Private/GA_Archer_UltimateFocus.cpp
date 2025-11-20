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
	ActiveBuffParticleComponent = nullptr;
	ActiveHeadBuffParticleComponent = nullptr;
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

	// B. 몸 지속 이펙트 스폰 (부착 + 핸들 저장)
	if (BuffParticleSystem && OwnerCharacter->GetMesh())
	{
		ActiveBuffParticleComponent = UGameplayStatics::SpawnEmitterAttached(
			BuffParticleSystem,
			OwnerCharacter->GetMesh(),
			NAME_None, // 소켓 이름 (몸 전체)
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			FVector(1.5f), // 스케일 조정 (더 크게)
			EAttachLocation::SnapToTarget,
			true // <-- bAutoDestroy = true (여기서 이미 설정됨)
		);
	}

	// C. 머리 지속 이펙트 스폰 (부착 + 핸들 저장)
	if (HeadBuffParticleSystem && OwnerCharacter->GetMesh())
	{
		// 'head' 소켓이 있다면 거기에 부착
		ActiveHeadBuffParticleComponent = UGameplayStatics::SpawnEmitterAttached(
			HeadBuffParticleSystem,
			OwnerCharacter->GetMesh(),
			TEXT("FX_Head"), // 스켈레탈 메쉬에 'head' 소켓이 있는지 확인 필요
			FVector(50, 0, 0), // 머리 위로 약간 올림
			FRotator::ZeroRotator,
			FVector(1.0f),
			EAttachLocation::SnapToTarget,
			true // <-- bAutoDestroy = true (여기서 이미 설정됨)
		);

		// 'head' 소켓이 없거나 실패했다면, 몸 기준으로 높이 띄움
		if (!ActiveHeadBuffParticleComponent)
		{
			UE_LOG(LogRamdomItemDefense, Warning, TEXT("GA_Archer_UltimateFocus: 'head' socket not found. Attaching to root with offset."));
			ActiveHeadBuffParticleComponent = UGameplayStatics::SpawnEmitterAttached(
				HeadBuffParticleSystem,
				OwnerCharacter->GetMesh(),
				NAME_None, // 소켓 대신 루트
				FVector(100, 0, 0), // 몸통 기준 150cm 위
				FRotator::ZeroRotator,
				FVector(1.0f),
				EAttachLocation::KeepRelativeOffset, // 상대 위치 유지
				true // <-- bAutoDestroy = true (여기서 이미 설정됨)
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
	// 1. 버프 GE 제거 (부모 클래스의 UltimateStateEffectHandle도 여기서 처리됨)
	if (UltimateBuffEffectHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(UltimateBuffEffectHandle);
		UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_UltimateFocus: Removed UltimateBuffEffect."));
	}

	// --- [ ★★★ 코드 수정 (SetAutoDestroy 제거) ★★★ ] ---
	// 2. 몸 지속 파티클 컴포넌트 제거
	if (ActiveBuffParticleComponent)
	{
		ActiveBuffParticleComponent->Deactivate();
		ActiveBuffParticleComponent = nullptr;
		UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_UltimateFocus: Deactivated BuffParticleComponent."));
	}
	// 3. 머리 지속 파티클 컴포넌트 제거
	if (ActiveHeadBuffParticleComponent)
	{
		ActiveHeadBuffParticleComponent->Deactivate();
		ActiveHeadBuffParticleComponent = nullptr;
		UE_LOG(LogRamdomItemDefense, Log, TEXT("GA_Archer_UltimateFocus: Deactivated HeadBuffParticleComponent."));
	}
	// --- [ ★★★ 코드 수정 끝 ★★★ ] ---

	// 4. 타이머가 혹시라도 아직 돌고 있었다면 중지
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BuffDurationTimerHandle);
	}

	// 5. 부모의 EndAbility 호출 (필수)
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}