// Source/RamdomItemDefense/Private/DarkPulseProjectile.cpp

#include "DarkPulseProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
// --- [�ڵ� ����] ---
// �� ����ü�� �� �̻� GAS/Attribute/Monster ������ �� �ʿ䰡 �����ϴ�.
// #include "AbilitySystemComponent.h"
// #include "AbilitySystemBlueprintLibrary.h"
// #include "MyAttributeSet.h"
// #include "MonsterBaseCharacter.h"
// #include "DrawDebugHelpers.h"
// #include "RamdomItemDefense.h"
// --- [�ڵ� ���� ��] ---

ADarkPulseProjectile::ADarkPulseProjectile()
{
	PrimaryActorTick.bCanEverTick = true; // Tick �Լ� Ȱ��ȭ (���� ������)

	InitialLifeSpan = 5.0f; // 5�� �� �ڵ� �ı�
	// bHasExploded = false; // (�� �̻� ���� ���� ����)
	// ExplosionRadius = 300.0f; // (GA�� ����)
	ArrivalTolerance = 50.0f; // ���� ��� ����

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetCollisionProfileName(TEXT("NoCollision")); // �浹 ����
	SphereComponent->SetGenerateOverlapEvents(false); // ������ ����

	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 1500.0f; // (�� ���� GA�� VisualProjectileSpeed�� ��ġ��Ű�� ���� ����)
	ProjectileMovement->MaxSpeed = 1500.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false; // ƨ���� ����
	ProjectileMovement->ProjectileGravityScale = 0.0f; // �߷� ����

	ProjectileMovement->bIsHomingProjectile = true; // ����ź
	ProjectileMovement->HomingAccelerationMagnitude = 5000.0f;
}
void ADarkPulseProjectile::BeginPlay()
{
	Super::BeginPlay();

	// --- [�ڵ� ����] ---
	// HomingTarget ���� ������ ���� GA�� ����ϹǷ� ���⼭�� �ʿ� �����ϴ�.
	// (���� GA�� HomingTargetComponent�� ������ ���� �ʾҴٸ� ���⼭ Destroy)
	if (ProjectileMovement && !ProjectileMovement->HomingTargetComponent.IsValid())
	{
		// 1�� �ڿ� �ٽ� Ȯ�� (GA�� ���� ���� ������ �ð��� ��)
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			if (ProjectileMovement && !ProjectileMovement->HomingTargetComponent.IsValid())
			{
				Destroy(); // ������ Ÿ���� ������ �ı�
			}
			});
	}
	// --- [�ڵ� ���� ��] ---
}

void ADarkPulseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- [�ڵ� ����] ---
	// ������ ����/���� ���� ��� ����
	// ���� Ÿ�ٿ� '����'�ߴ��� ���θ� Ȯ���Ͽ� ������ �ı�

	if (!ProjectileMovement || !ProjectileMovement->HomingTargetComponent.IsValid())
	{
		return; // Ÿ���� ������ �ƹ��͵� �� ��
	}

	// �̵��� �����߰�, (���� �ڵ�� ����)
	if (ProjectileMovement->Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		// �Ÿ��� ���� ��� ����(ArrivalTolerance)���� ���������
		const float DistanceToTarget = FVector::Dist(GetActorLocation(), ProjectileMovement->HomingTargetComponent->GetComponentLocation());
		if (DistanceToTarget <= ArrivalTolerance)
		{
			// "����" ��� "������ �ı�"
			// (���� ����Ʈ�� GA�� Ÿ�̸ӷ� ���� ����)
			TrailEffect->Deactivate(); // ���� ����Ʈ ����
			SetLifeSpan(0.1f);         // ��� �ı�
			ProjectileMovement->StopMovementImmediately();
		}
	}
	// --- [�ڵ� ���� ��] ---
}