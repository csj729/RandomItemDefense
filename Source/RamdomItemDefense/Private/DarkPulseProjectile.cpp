#include "DarkPulseProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "MyAttributeSet.h"
#include "MonsterBaseCharacter.h"
#include "DrawDebugHelpers.h"
#include "RamdomItemDefense.h"

ADarkPulseProjectile::ADarkPulseProjectile()
{
	PrimaryActorTick.bCanEverTick = true; // Tick 함수 활성화

	InitialLifeSpan = 5.0f;
	bHasExploded = false;
	ExplosionRadius = 300.0f;
	ArrivalTolerance = 50.0f;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetCollisionProfileName(TEXT("NoCollision"));
	SphereComponent->SetGenerateOverlapEvents(false);

	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = RootComponent;
	ProjectileMovement->InitialSpeed = 1500.0f;
	ProjectileMovement->MaxSpeed = 1500.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = 5000.0f;
}

void ADarkPulseProjectile::Initialize(AActor* InTargetActor, UAbilitySystemComponent* InOwnerASC, TSubclassOf<UGameplayEffect> InDamageEffectClass)
{
	TargetActor = InTargetActor;
	OwnerASC = InOwnerASC;
	DamageEffectClass = InDamageEffectClass;
}

void ADarkPulseProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TargetActor.IsValid())
	{
		ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
	}
	else
	{
		Destroy();
	}
}

void ADarkPulseProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 이미 폭발했거나, 타겟이 사라졌거나, 소유자가 없으면 아무것도 하지 않음
	if (bHasExploded || !TargetActor.IsValid() || !OwnerASC.IsValid())
	{
		return;
	}

	AMonsterBaseCharacter* TargetMonster = Cast<AMonsterBaseCharacter>(TargetActor.Get());
	if (TargetMonster && TargetMonster->IsDying())
	{
		Explode();
		return;
	}

	// --- [ ★★★ 수정 시작 ★★★ ] ---
	// 투사체가 실제로 이동을 시작했는지 확인 (속도가 0보다 큰지)
	// SizeSquared()는 제곱근 계산을 피하므로 약간 더 효율적입니다. 작은 값(KINDA_SMALL_NUMBER)보다 큰지 비교합니다.
	if (ProjectileMovement && ProjectileMovement->Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		// 이동을 시작했다면, 타겟과의 거리 계산
		const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());

		// 거리가 도착 허용 오차(ArrivalTolerance)보다 가까워지면 폭발
		if (DistanceToTarget <= ArrivalTolerance)
		{
			Explode();
		}
	}
	// --- [ 수정 끝 ] ---
}


void ADarkPulseProjectile::Explode()
{
	if (bHasExploded || !OwnerASC.IsValid() || !DamageEffectClass)
	{
		return;
	}
	bHasExploded = true;

	ProjectileMovement->StopMovementImmediately();
	TrailEffect->Deactivate();
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FVector ExplosionLocation = GetActorLocation();

	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, ExplosionLocation);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionLocation);
	}

	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	UAbilitySystemComponent* SourceASC = OwnerASC.Get();

	float OwnerAttackDamage = SourceASC->GetNumericAttribute(UMyAttributeSet::GetAttackDamageAttribute());
	float FinalDamage = -(50.f + (OwnerAttackDamage * 0.3f));

	// --- [ ★★★ 디버그 구체 그리기 추가 ★★★ ] ---
	// 빨간색 구체를 2초 동안 표시합니다. (12개의 선분으로 구성)
	DrawDebugSphere(
		GetWorld(),           // 월드 컨텍스트
		ExplosionLocation,    // 구체의 중심 위치
		ExplosionRadius,      // 구체의 반경 (BP 설정값 사용)
		12,                   // 구체를 구성할 선분 수 (값이 높을수록 부드러움)
		FColor::Red,          // 구체의 색상
		false,                // 지속성 여부 (false = 지정된 시간 동안만 표시)
		2.0f,                 // 표시 시간 (초)
		0,                    // 우선 순위 (0 = 보통)
		1.0f                  // 선 두께
	);
	// --- [ 디버그 구체 그리기 끝 ] ---

	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ExplosionLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("Dark Pulse Exploded! FinalDamage: %.1f, Hit %d monsters."), FinalDamage, OverlappedActors.Num());

	FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value")); // 또는 Skill.Damage.DarkPulse

	for (AActor* HitActor : OverlappedActors)
	{
		AMonsterBaseCharacter* HitMonster = Cast<AMonsterBaseCharacter>(HitActor);
		if (!HitMonster || HitMonster->IsDying())
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitMonster);
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageTag, FinalDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}

	SetLifeSpan(2.0f);
}