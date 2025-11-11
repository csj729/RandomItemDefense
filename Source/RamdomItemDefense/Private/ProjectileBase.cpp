// Source/RamdomItemDefense/Private/ProjectileBase.cpp (수정)

#include "ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"

AProjectileBase::AProjectileBase()
{
	// [ ★★★ 수정 ★★★ ]
	// Tick 함수를 사용하지 않습니다.
	PrimaryActorTick.bCanEverTick = false;

	// [ ★★★ 제거 ★★★ ]
	// (GA가 수명을 설정할 것이므로 기본 수명 5초를 제거합니다)
	// InitialLifeSpan = 5.0f; 
	// (Tick을 안하므로 도착 반경도 제거합니다)
	// ArrivalTolerance = 50.0f; 

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetCollisionProfileName(TEXT("NoCollision"));
	SphereComponent->SetGenerateOverlapEvents(false);

	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = 10000.0f;
}
void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// (이 로직은 유효하므로 그대로 둡니다)
	if (ProjectileMovement && !ProjectileMovement->HomingTargetComponent.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			if (ProjectileMovement && !ProjectileMovement->HomingTargetComponent.IsValid())
			{
				Destroy();
			}
			});
	}
}

void AProjectileBase::Tick(float DeltaTime)
{

}