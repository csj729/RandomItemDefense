#include "MonsterBaseCharacter.h"
#include "MonsterAttributeSet.h"
#include "MonsterSpawner.h"
#include "MonsterAIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
// --- [코드 수정] ---
#include "Particles/ParticleSystem.h" // Niagara -> Particle System
// --- [코드 수정 끝] ---
#include "Kismet/GameplayStatics.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"


AMonsterBaseCharacter::AMonsterBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UMonsterAttributeSet>(TEXT("AttributeSet"));

	GoldOnDeath = 10;
	bIsDying = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	CurrentWaveMaterial = nullptr;
}

void AMonsterBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMonsterBaseCharacter, CurrentWaveMaterial);
}

UAbilitySystemComponent* AMonsterBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AMonsterBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		if (AttributeSet)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
		}
	}

	if (!HasAuthority() && CurrentWaveMaterial)
	{
		OnRep_WaveMaterial();
	}
}

void AMonsterBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f) {}
}

void AMonsterBaseCharacter::SetSpawner(AMonsterSpawner* InSpawner)
{
	MySpawner = InSpawner;
}

void AMonsterBaseCharacter::Die(AActor* Killer)
{
	if (bIsDying) { return; }
	bIsDying = true;

	if (MySpawner) { MySpawner->OnMonsterKilled(); }

	AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController());
	if (AIController && AIController->GetBrainComponent())
	{
		AIController->GetBrainComponent()->StopLogic(TEXT("Died"));
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	float DeathAnimLength = 0.1f;
	if (DeathMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Death"));
		DeathAnimLength = PlayAnimMontage(DeathMontage);
	}

	if (DeathAnimLength > 0.f) { SetLifeSpan(DeathAnimLength); }
	else { SetLifeSpan(0.1f); }
}

void AMonsterBaseCharacter::SetWaveMaterial(UMaterialInterface* WaveMaterial)
{
	if (HasAuthority())
	{
		CurrentWaveMaterial = WaveMaterial;
		OnRep_WaveMaterial();
	}
}

void AMonsterBaseCharacter::OnRep_WaveMaterial()
{
	if (CurrentWaveMaterial && GetMesh())
	{
		GetMesh()->SetMaterial(0, CurrentWaveMaterial);
	}
}

void AMonsterBaseCharacter::PlayHitEffect(const FGameplayTagContainer& EffectTags)
{
	if (!HasAuthority()) { return; }

	FGameplayTag SelectedTag = FGameplayTag::EmptyTag;
	for (const auto& Elem : HitEffectsMap)
	{
		if (EffectTags.HasTag(Elem.Key))
		{
			SelectedTag = Elem.Key;
			break;
		}
	}

	if (SelectedTag.IsValid())
	{
		Multicast_PlayHitEffect(SelectedTag);
	}
}

void AMonsterBaseCharacter::Multicast_PlayHitEffect_Implementation(const FGameplayTag& HitTag)
{
	const FHitEffectData* EffectData = HitEffectsMap.Find(HitTag);
	if (!EffectData) { return; }

	// --- [코드 수정] ---
	// 이펙트 재생
	if (EffectData->HitEffect)
	{
		FVector SpawnLocation = GetActorLocation();
		// Niagara -> GameplayStatics
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EffectData->HitEffect, SpawnLocation);
	}
	// --- [코드 수정 끝] ---

	// 사운드 재생
	if (EffectData->HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EffectData->HitSound, GetActorLocation());
	}
}