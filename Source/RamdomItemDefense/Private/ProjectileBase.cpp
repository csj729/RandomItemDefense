// Source/RamdomItemDefense/Private/ProjectileBase.cpp (수정)

#include "ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// [중요] 투사체가 클라이언트에게도 보이도록 복제 설정 활성화
	bReplicates = true;
	// 투사체 움직임도 복제하려면 아래 설정 필요 (ProjectileMovementComponent가 있다면 보통 자동 처리되나 명시 권장)
	SetReplicateMovement(true);

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