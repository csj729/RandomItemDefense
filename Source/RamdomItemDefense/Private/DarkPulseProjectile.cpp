// Source/RamdomItemDefense/Private/DarkPulseProjectile.cpp

#include "DarkPulseProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
// --- [코드 수정] ---
// 이 투사체는 더 이상 GAS/Attribute/Monster 로직을 알 필요가 없습니다.
// #include "AbilitySystemComponent.h"
// #include "AbilitySystemBlueprintLibrary.h"
// #include "MyAttributeSet.h"
// #include "MonsterBaseCharacter.h"
// #include "DrawDebugHelpers.h"
// #include "RamdomItemDefense.h"
// --- [코드 수정 끝] ---

ADarkPulseProjectile::ADarkPulseProjectile()
{
	PrimaryActorTick.bCanEverTick = true; // Tick 함수 활성화 (도착 감지용)

	InitialLifeSpan = 5.0f; // 5초 뒤 자동 파괴
	// bHasExploded = false; // (더 이상 폭발 로직 없음)
	// ExplosionRadius = 300.0f; // (GA가 관리)
	ArrivalTolerance = 50.0f; // 도착 허용 오차

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetCollisionProfileName(TEXT("NoCollision")); // 충돌 없음
	SphereComponent->SetGenerateOverlapEvents(false); // 오버랩 없음

	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 1500.0f; // (이 값은 GA의 VisualProjectileSpeed와 일치시키는 것이 좋음)
	ProjectileMovement->MaxSpeed = 1500.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false; // 튕기지 않음
	ProjectileMovement->ProjectileGravityScale = 0.0f; // 중력 없음

	ProjectileMovement->bIsHomingProjectile = true; // 유도탄
	ProjectileMovement->HomingAccelerationMagnitude = 5000.0f;
}
void ADarkPulseProjectile::BeginPlay()
{
	Super::BeginPlay();

	// --- [코드 수정] ---
	// HomingTarget 설정 로직은 이제 GA가 담당하므로 여기서는 필요 없습니다.
	// (만약 GA가 HomingTargetComponent를 설정해 주지 않았다면 여기서 Destroy)
	if (ProjectileMovement && !ProjectileMovement->HomingTargetComponent.IsValid())
	{
		// 1초 뒤에 다시 확인 (GA가 스폰 직후 설정할 시간을 줌)
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			if (ProjectileMovement && !ProjectileMovement->HomingTargetComponent.IsValid())
			{
				Destroy(); // 여전히 타겟이 없으면 파괴
			}
			});
	}
	// --- [코드 수정 끝] ---
}

void ADarkPulseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- [코드 수정] ---
	// 데미지 로직/폭발 로직 모두 제거
	// 오직 타겟에 '도착'했는지 여부만 확인하여 스스로 파괴

	if (!ProjectileMovement || !ProjectileMovement->HomingTargetComponent.IsValid())
	{
		return; // 타겟이 없으면 아무것도 안 함
	}

	// 이동을 시작했고, (이전 코드와 동일)
	if (ProjectileMovement->Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		// 거리가 도착 허용 오차(ArrivalTolerance)보다 가까워지면
		const float DistanceToTarget = FVector::Dist(GetActorLocation(), ProjectileMovement->HomingTargetComponent->GetComponentLocation());
		if (DistanceToTarget <= ArrivalTolerance)
		{
			// "폭발" 대신 "스스로 파괴"
			// (폭발 이펙트는 GA가 타이머로 별도 실행)
			TrailEffect->Deactivate(); // 꼬리 이펙트 끄기
			SetLifeSpan(0.1f);         // 즉시 파괴
			ProjectileMovement->StopMovementImmediately();
		}
	}
	// --- [코드 수정 끝] ---
}