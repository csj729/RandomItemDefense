#include "GA_Archer_ArrowShower.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystem.h"

UGA_Archer_ArrowShower::UGA_Archer_ArrowShower()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	BaseActivationChance = 0.1f; // (궁수 스킬 2 확률 예시)
	InitialDelay = 0.5f;
	Radius = 400.0f;
}

void UGA_Archer_ArrowShower::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo) || !TriggerEventData || !TriggerEventData->Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타겟의 위치에 효과 적용
	TargetLocation = TriggerEventData->Target.Get()->GetActorLocation();

	// 시각 효과 즉시 스폰
	if (ArrowShowerEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ArrowShowerEffect, TargetLocation, FRotator::ZeroRotator, true);
	}

	// 0.5초 딜레이 후 ApplyEffect 함수 호출
	GetWorld()->GetTimerManager().SetTimer(
		EffectTimerHandle,
		this,
		&UGA_Archer_ArrowShower::ApplyEffect,
		InitialDelay,
		false
	);
}

void UGA_Archer_ArrowShower::ApplyEffect()
{
	FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	UAbilitySystemComponent* CasterASC = ActorInfo->AbilitySystemComponent.Get();
	if (!CasterASC || !ArrowShowerEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 범위 내 몬스터 탐색
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), TargetLocation, Radius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);

	for (AActor* TargetActor : OverlappedActors)
	{
		AMonsterBaseCharacter* Monster = Cast<AMonsterBaseCharacter>(TargetActor);
		if (Monster && !Monster->IsDying())
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Monster);
			if (TargetASC)
			{
				// 3초간 지속되는 DoT 및 슬로우 GE 적용
				TargetASC->ApplyGameplayEffectToSelf(ArrowShowerEffectClass->GetDefaultObject<UGameplayEffect>(), 1.0f, ContextHandle);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}