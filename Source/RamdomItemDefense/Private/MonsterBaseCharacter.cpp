// Source/RamdomItemDefense/Private/MonsterBaseCharacter.cpp

#include "MonsterBaseCharacter.h"
#include "MonsterAttributeSet.h"
#include "MonsterSpawner.h"
#include "MonsterAIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "BehaviorTree/BlackboardComponent.h" 
#include "AbilitySystemComponent.h"
#include "RID_DamageStatics.h"         
#include "DamageTextWidget.h"        
#include "Blueprint/UserWidget.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"


AMonsterBaseCharacter::AMonsterBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UMonsterAttributeSet>(TEXT("AttributeSet"));

	GoldOnDeath = 10;
	bIsDying = false;
	bIsBoss = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	CurrentWaveMaterial = nullptr;

	BaseMoveSpeed = 300.0f;
	SpawnWaveIndex = 1;

	bIsCounterAttackMonster = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	bReplicates = true;
	SetReplicateMovement(true);

	// 네트워크 컬링 거리 최적화
	bAlwaysRelevant = false;
	NetCullDistanceSquared = 4500000000.0f;

	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 33.0f;
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

void AMonsterBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	MonsterAIController = Cast<AMonsterAIController>(NewController);

	if (MonsterAIController.IsValid() && MySpawner && MySpawner->PatrolPathActor)
	{
		MonsterAIController->SetPatrolPath(MySpawner->PatrolPathActor);
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		if (AttributeSet)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleMoveSpeedChanged);
			if (GetCharacterMovement())
			{
				const float CurrentSpeedMultiplier = AttributeSet->GetMoveSpeed();
				GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * CurrentSpeedMultiplier;
			}
			URID_DamageStatics::OnCritDamageOccurred.AddDynamic(this, &AMonsterBaseCharacter::OnCritDamageOccurred);
		}
	}
}

void AMonsterBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.Stun")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnStunTagChanged);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.Slow")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnSlowTagChanged);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.ArmorShred")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnArmorShredTagChanged);
	}

	FGameplayTag SlowTag = FGameplayTag::RequestGameplayTag(FName("State.Slow"));
	if (AbilitySystemComponent->GetTagCount(SlowTag) > 0)
	{
		OnSlowTagChanged(SlowTag, 1);
	}

	FGameplayTag ArmorTag = FGameplayTag::RequestGameplayTag(FName("State.ArmorShred"));
	if (AbilitySystemComponent->GetTagCount(ArmorTag) > 0)
	{
		OnArmorShredTagChanged(ArmorTag, 1);
	}

	if (!HasAuthority() && CurrentWaveMaterial)
	{
		OnRep_WaveMaterial();
	}
}

void AMonsterBaseCharacter::Multicast_PlaySpawnEffect_Implementation()
{
	if (SpawnEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SpawnEffect, GetActorLocation());
	}
}

void AMonsterBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data) {}

void AMonsterBaseCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		const float NewSpeedMultiplier = Data.NewValue;
		MoveComp->MaxWalkSpeed = BaseMoveSpeed * NewSpeedMultiplier;
	}
}

void AMonsterBaseCharacter::SetSpawner(AMonsterSpawner* InSpawner)
{
	MySpawner = InSpawner;

	if (MySpawner)
	{
		// [디버그 1] SetSpawner 호출 확인
		RID_LOG(FColor::Yellow, TEXT("[Debug] SetSpawner Called for %s. Binding Delegate..."), *GetName());

		// 델리게이트 바인딩 (이미 바인딩된 경우 중복 방지)
		MySpawner->OnBossStateChanged.RemoveDynamic(this, &AMonsterBaseCharacter::OnBossStateChanged);
		MySpawner->OnBossStateChanged.AddDynamic(this, &AMonsterBaseCharacter::OnBossStateChanged);

		// 이미 보스가 살아있는지 확인하여 수동 호출
		if (MySpawner->IsBossAlive())
		{
			RID_LOG(FColor::Yellow, TEXT("[Debug] Boss is already alive. Manually calling OnBossStateChanged."));
			OnBossStateChanged(true);
		}
	}
	else
	{
		RID_LOG(FColor::Red, TEXT("[Error] SetSpawner called with NULL Spawner!"));
	}

	if (!MonsterAIController.IsValid())
	{
		MonsterAIController = Cast<AMonsterAIController>(GetController());
	}
	if (MonsterAIController.IsValid() && MySpawner->PatrolPathActor)
	{
		MonsterAIController->SetPatrolPath(MySpawner->PatrolPathActor);
	}

}

void AMonsterBaseCharacter::Multicast_PlayMontage_Implementation(UAnimMontage* MontageToPlay)
{
	if (MontageToPlay)
	{
		PlayAnimMontage(MontageToPlay);
	}
}

void AMonsterBaseCharacter::Die(AActor* Killer)
{
	if (bIsDying) { return; }
	bIsDying = true;

	if (MySpawner) { MySpawner->OnMonsterKilled(IsBoss()); }

	OnStunStateChanged(false);
	OnSlowStateChanged(false);
	OnArmorShredStateChanged(false);

	if (MonsterAIController.IsValid())
	{
		MonsterAIController->UnPossess();
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	if (HasAuthority())
	{
		SetStatusEffectState(FGameplayTag::RequestGameplayTag(FName("State.Slow")), false, nullptr);
		SetStatusEffectState(FGameplayTag::RequestGameplayTag(FName("State.ArmorShred")), false, nullptr);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	float RagdollDelay = 0.1f;

	if (DeathMontage)
	{
		Multicast_PlayMontage(DeathMontage);

		const float DeathAnimLength = DeathMontage->GetPlayLength();
		const float BlendOutTime = DeathMontage->BlendOut.GetBlendTime();
		RagdollDelay = FMath::Max(0.01f, DeathAnimLength - BlendOutTime - 0.05f);
	}

	GetWorldTimerManager().SetTimer(
		RagdollTimerHandle,
		this,
		&AMonsterBaseCharacter::GoRagdoll,
		RagdollDelay,
		false
	);
}

void AMonsterBaseCharacter::GoRagdoll()
{
	Multicast_GoRagdoll();
	SetLifeSpan(2.0f);
}

void AMonsterBaseCharacter::Multicast_GoRagdoll_Implementation()
{
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (GetMesh())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
		if (GetMesh()->GetAnimInstance())
		{
			GetMesh()->GetAnimInstance()->StopAllMontages(0.0f);
		}
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
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
	if (EffectData->HitEffect)
	{
		FVector SpawnLocation = GetActorLocation();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EffectData->HitEffect, SpawnLocation);
	}
	if (EffectData->HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EffectData->HitSound, GetActorLocation());
	}
}

void AMonsterBaseCharacter::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (bIsDying) return;
	bool bIsStunned = NewCount > 0;
	if (MonsterAIController.IsValid())
	{
		UBrainComponent* Brain = MonsterAIController->GetBrainComponent();
		if (Brain)
		{
			if (bIsStunned)
				Brain->PauseLogic(TEXT("Stunned"));
			else
				Brain->ResumeLogic(TEXT("StunEnded"));
		}
	}
	OnStunStateChanged(bIsStunned);
}

void AMonsterBaseCharacter::OnCritDamageOccurred(AActor* TargetActor, float CritDamageAmount)
{
	if (TargetActor != this || !DamageTextWidgetClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	UDamageTextWidget* DamageWidget = CreateWidget<UDamageTextWidget>(PC, DamageTextWidgetClass);
	if (DamageWidget)
	{
		FString DamageString = FString::Printf(TEXT("%.0f!"), CritDamageAmount);
		DamageWidget->SetDamageText(FText::FromString(DamageString));
		FVector2D ScreenPosition;
		if (PC->ProjectWorldLocationToScreen(GetActorLocation() + FVector(0, 0, 50.f), ScreenPosition))
		{
			DamageWidget->SetPositionInViewport(ScreenPosition);
			DamageWidget->AddToViewport();
			DamageWidget->PlayRiseAndFade();
		}
	}
}

void AMonsterBaseCharacter::SetStatusEffectState(FGameplayTag StatusTag, bool bIsActive, UNiagaraSystem* EffectTemplate)
{
	if (bIsActive)
	{
		if (!ActiveStatusParticles.Contains(StatusTag) && EffectTemplate && GetMesh())
		{
			UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
				EffectTemplate,
				GetMesh(),
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				false
			);

			if (NC)
			{
				ActiveStatusParticles.Add(StatusTag, NC);
			}
		}
	}
	else
	{
		if (TObjectPtr<UNiagaraComponent>* FoundNC = ActiveStatusParticles.Find(StatusTag))
		{
			UNiagaraComponent* NC = *FoundNC;
			if (::IsValid(NC))
			{
				NC->Deactivate();
				NC->DestroyComponent();
			}
			ActiveStatusParticles.Remove(StatusTag);
		}
	}
}

void AMonsterBaseCharacter::OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (bIsDying) return;
	bool bIsSlowed = NewCount > 0;
	if (HasAuthority())
	{
		SetStatusEffectState(Tag, bIsSlowed, SlowEffectTemplate);
	}
	OnSlowStateChanged(bIsSlowed);
}

void AMonsterBaseCharacter::OnArmorShredTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (bIsDying) return;
	bool bIsShredded = NewCount > 0;
	if (HasAuthority())
	{
		SetStatusEffectState(Tag, bIsShredded, ArmorShredEffectTemplate);
	}
	OnArmorShredStateChanged(bIsShredded);
}

void AMonsterBaseCharacter::OnBossStateChanged(bool bIsBossAlive)
{
	if (IsBoss() || !AttributeSet)
	{
		return;
	}

	if (bIsBossAlive)
	{
		if (CurrentBossBuffArmor == 0.0f)
		{
			int32 CurrentWave = GetSpawnWaveIndex();

			// [수정] 테스트를 위해 10웨이브 미만도 1스테이지로 강제 설정
			int32 BossStage = (CurrentWave < 10) ? 1 : (CurrentWave / 10);

			if (BossStage > 0)
			{
				float BonusArmor = (float)BossStage * 30.0f;
				float OldArmor = AttributeSet->GetArmor();
				float NewArmor = OldArmor + BonusArmor;

				AttributeSet->SetArmor(NewArmor);
				CurrentBossBuffArmor = BonusArmor;

				//RID_LOG(FColor::Cyan, TEXT("[Buff Applied] %s : Armor +%.1f (%.1f -> %.1f)"),
				//	*GetName(), BonusArmor, OldArmor, NewArmor);
			}
			else
			{
				/*RID_LOG(FColor::Orange, TEXT("[Debug] BossStage is 0. No Buff applied."));*/
			}
		}
	}
	else
	{
		if (CurrentBossBuffArmor > 0.0f)
		{
			float OldArmor = AttributeSet->GetArmor();
			float NewArmor = OldArmor - CurrentBossBuffArmor;

			AttributeSet->SetArmor(NewArmor);
			CurrentBossBuffArmor = 0.0f;

			//RID_LOG(FColor::Cyan, TEXT("[Buff Removed] %s : Armor Reverted (%.1f -> %.1f)"),
			//	*GetName(), OldArmor, NewArmor);
		}
	}
}