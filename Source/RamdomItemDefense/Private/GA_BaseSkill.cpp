// Source/RamdomItemDefense/Private/GA_BaseSkill.cpp (수정)
#include "GA_BaseSkill.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefenseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "RamdomItemDefenseCharacter.h"
#include "SoldierDrone.h" // 드론 헤더
#include "Components/SceneComponent.h"

UGA_BaseSkill::UGA_BaseSkill()
{
	// 모든 스킬은 기본적으로 InstancedPerActor로 설정 (타이머 등 사용 가능)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

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
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		// 1. 캐릭터인 경우: 멀티캐스트로 모든 클라이언트 동기화 (부착)
		if (ARamdomItemDefenseCharacter* OwnerCharacter = Cast<ARamdomItemDefenseCharacter>(AvatarActor))
		{
			OwnerCharacter->Multicast_SpawnParticleAttached(
				MuzzleFlashEffect,
				MuzzleSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				FVector(1.0f)
			);
		}
		// 2. 드론인 경우: (드론 클래스에 멀티캐스트가 없다면) 서버에서만이라도 재생
		else if (ASoldierDrone* OwnerDrone = Cast<ASoldierDrone>(AvatarActor))
		{
			// 이제 드론도 멀티캐스트 함수를 가집니다.
			OwnerDrone->Multicast_SpawnParticleAttached(
				MuzzleFlashEffect,
				MuzzleSocketName,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				FVector(1.0f)
			);
		}
	}

	// 2. (필수) 부모(UGameplayAbility)의 ActivateAbility를 호출합니다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}