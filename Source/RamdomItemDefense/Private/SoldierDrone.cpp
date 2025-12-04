#include "SoldierDrone.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MyAttributeSet.h"
#include "AttackComponent.h"
#include "RamdomItemDefenseCharacter.h" 
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

ASoldierDrone::ASoldierDrone()
{
	PrimaryActorTick.bCanEverTick = true; // Tick을 활성화합니다.
	bReplicates = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMyAttributeSet>(TEXT("AttributeSet"));

	AttackComponent = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackComponent"));

	FollowOffset = FVector(0.f, -50.f, 80.f); // 기본 오프셋 (좌측 상단)
	FollowInterpSpeed = 5.0f; // 기본 추적 속도
}

UAbilitySystemComponent* ASoldierDrone::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASoldierDrone::SetOwnerCharacter(ARamdomItemDefenseCharacter* InOwner)
{
	OwnerCharacter = InOwner;
	SetOwner(InOwner); // Pawn의 Owner도 설정

	if (AttackComponent)
	{
		// AttackComponent의 OwnerCharacter 변수(public으로 수정함)에 접근
		AttackComponent->OwnerCharacter = InOwner;
	}
}

void ASoldierDrone::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->InitAbilityActorInfo(this, this);
		}

		if (AttackComponent && AbilitySystemComponent && AttributeSet)
		{
			// AttackComponent 초기화 함수 호출
			AttackComponent->Initialize(AbilitySystemComponent, AttributeSet);
		}
	}
}

void ASoldierDrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Tick에서 직접 위치 갱신 (주인 따라다니기)
	if (OwnerCharacter.IsValid())
	{
		FVector OwnerLocation = OwnerCharacter->GetActorLocation();
		FRotator OwnerRotation = OwnerCharacter->GetActorRotation();
		FVector TargetLocation = OwnerLocation + OwnerRotation.RotateVector(FollowOffset);

		FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, FollowInterpSpeed);

		SetActorLocation(NewLocation, false); // Sweep: false

		FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(NewLocation, OwnerLocation);
		SetActorRotation(TargetRotation);
	}
}

void ASoldierDrone::Multicast_SpawnParticleAtLocation_Implementation(UParticleSystem* EmitterTemplate, FVector Location, FRotator Rotation, FVector Scale)
{
	if (EmitterTemplate)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EmitterTemplate, Location, Rotation, Scale, true);
	}
}

void ASoldierDrone::Multicast_SpawnParticleAttached_Implementation(UParticleSystem* EmitterTemplate, FName SocketName, FVector LocationOffset, FRotator RotationOffset, FVector Scale)
{
	if (EmitterTemplate && GetMesh())
	{
		UGameplayStatics::SpawnEmitterAttached(
			EmitterTemplate,
			GetMesh(),
			SocketName,
			LocationOffset,
			RotationOffset,
			Scale,
			EAttachLocation::SnapToTarget,
			true // bAutoDestroy
		);
	}
}