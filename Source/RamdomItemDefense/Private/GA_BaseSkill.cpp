// Source/RamdomItemDefense/Private/GA_BaseSkill.cpp (수정)
#include "GA_BaseSkill.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefenseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "SoldierDrone.h" // 드론 헤더
#include "Components/SceneComponent.h"

UGA_BaseSkill::UGA_BaseSkill()
{
	// 모든 스킬은 기본적으로 InstancedPerActor로 설정 (타이머 등 사용 가능)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 기본값 설정 (자식 클래스에서 덮어쓸 수 있음)
	BaseActivationChance = 0.f;

	DamageBase = 0.f;
	DamageCoefficient = 0.f;

	MuzzleSocketName = FName("MuzzlePoint"); // 기본 소켓 이름
	MuzzleFlashEffect = nullptr;
}

/** * 어빌리티 활성화 시 (자식 클래스의 ActivateAbility보다 먼저) 호출됩니다.
 * MuzzleFlash 이펙트를 스폰하는 공통 로직을 처리합니다.
 */
void UGA_BaseSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 1. (최우선) MuzzleFlashEffect가 유효하면 스폰합니다.
	if (MuzzleFlashEffect)
	{
		// --- [ ★★★ 로직 수정 ★★★ ] ---
		USceneComponent* AttachComponent = nullptr;
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		// 1. 시전자가 캐릭터인지 확인
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(AvatarActor))
		{
			AttachComponent = OwnerCharacter->GetMesh();
		}
		// 2. 시전자가 드론인지 확인
		else if (ASoldierDrone* OwnerDrone = Cast<ASoldierDrone>(AvatarActor))
		{
			// 드론의 Mesh는 private/protected일 수 있으니, 안전하게 RootComponent에 부착합니다.
			// (드론 블루프린트에서 "MuzzlePoint" 소켓을 RootComponent에 추가해야 합니다)
			AttachComponent = OwnerDrone->GetRootComponent(); //
		}

		// 3. 부착할 컴포넌트를 찾았다면 이펙트 스폰
		if (AttachComponent)
		{
			// 캐스케이드 이펙트 스폰
			UGameplayStatics::SpawnEmitterAttached(
				MuzzleFlashEffect,
				AttachComponent, // ACharacter->GetMesh() 또는 ASoldierDrone->GetRootComponent()
				MuzzleSocketName,
				FVector::ZeroVector,    // Location Offset
				FRotator::ZeroRotator,  // Rotation Offset
				FVector(1.0f),          // Scale
				EAttachLocation::SnapToTarget,
				true                    // Auto Destroy
			);
		}
		// --- [ ★★★ 로직 수정 끝 ★★★ ] ---
	}

	// 2. (필수) 부모(UGameplayAbility)의 ActivateAbility를 호출합니다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}