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
	PrimaryActorTick.bCanEverTick = true; // Tick �Լ� Ȱ��ȭ

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

	// �̹� �����߰ų�, Ÿ���� ������ų�, �����ڰ� ������ �ƹ��͵� ���� ����
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

	// --- [ �ڡڡ� ���� ���� �ڡڡ� ] ---
	// ����ü�� ������ �̵��� �����ߴ��� Ȯ�� (�ӵ��� 0���� ū��)
	// SizeSquared()�� ������ ����� ���ϹǷ� �ణ �� ȿ�����Դϴ�. ���� ��(KINDA_SMALL_NUMBER)���� ū�� ���մϴ�.
	if (ProjectileMovement && ProjectileMovement->Velocity.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		// �̵��� �����ߴٸ�, Ÿ�ٰ��� �Ÿ� ���
		const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());

		// �Ÿ��� ���� ��� ����(ArrivalTolerance)���� ��������� ����
		if (DistanceToTarget <= ArrivalTolerance)
		{
			Explode();
		}
	}
	// --- [ ���� �� ] ---
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

	// --- [ �ڡڡ� ����� ��ü �׸��� �߰� �ڡڡ� ] ---
	// ������ ��ü�� 2�� ���� ǥ���մϴ�. (12���� �������� ����)
	DrawDebugSphere(
		GetWorld(),           // ���� ���ؽ�Ʈ
		ExplosionLocation,    // ��ü�� �߽� ��ġ
		ExplosionRadius,      // ��ü�� �ݰ� (BP ������ ���)
		12,                   // ��ü�� ������ ���� �� (���� �������� �ε巯��)
		FColor::Red,          // ��ü�� ����
		false,                // ���Ӽ� ���� (false = ������ �ð� ���ȸ� ǥ��)
		2.0f,                 // ǥ�� �ð� (��)
		0,                    // �켱 ���� (0 = ����)
		1.0f                  // �� �β�
	);
	// --- [ ����� ��ü �׸��� �� ] ---

	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), ExplosionLocation, ExplosionRadius, ObjectTypes, AMonsterBaseCharacter::StaticClass(), {}, OverlappedActors);

	RID_LOG(FColor::Cyan, TEXT("Dark Pulse Exploded! FinalDamage: %.1f, Hit %d monsters."), FinalDamage, OverlappedActors.Num());

	FGameplayTag DamageTag = FGameplayTag::RequestGameplayTag(FName("Skill.Damage.Value")); // �Ǵ� Skill.Damage.DarkPulse

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