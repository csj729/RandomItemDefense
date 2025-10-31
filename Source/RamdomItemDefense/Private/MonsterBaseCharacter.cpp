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
#include "RID_DamageStatics.h"         // ��������Ʈ ������
#include "DamageTextWidget.h"        // ������ ���� Ŭ����
#include "Blueprint/UserWidget.h"      // CreateWidget

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

	BaseMoveSpeed = 300.0f;
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

/** (AIController�� ���� ���ǵ� �� ȣ���) */
void AMonsterBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 1. AI ��Ʈ�ѷ� ĳ��
	MonsterAIController = Cast<AMonsterAIController>(NewController);

	// 2. ASC�� ��ȿ���� Ȯ�� (��ȿ�ؾ� ��)
	if (AbilitySystemComponent)
	{
		// 3. ASC �ʱ�ȭ (�߿�!)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 4. �±� ��������Ʈ ���ε�
		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.Stun")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnStunTagChanged);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.Slow")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnSlowTagChanged);

		// 5. �Ӽ� ��������Ʈ ���ε�
		if (AttributeSet)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleMoveSpeedChanged);

			// --- [ �ڡڡ� �ڵ� ���� �ڡڡ� ] ---
			// 6. (�߿�) ���� �Ӽ� ���� ������ *���* �� �̵� ������Ʈ�� ����
			// (GE�� �̹� ����� ���·� ������ �� �����Ƿ� �ʱⰪ ����)
			if (GetCharacterMovement())
			{
				// (BaseMoveSpeed * ���� MoveSpeed �Ӽ� ����)�� ���� �ӵ� ���
				// ���� GE_MonsterStatInit�� ���� ���� �ȵż� ������ 0�̸�, �ӵ��� 0�� �� (����)
				const float CurrentSpeedMultiplier = AttributeSet->GetMoveSpeed();
				GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * CurrentSpeedMultiplier;

				RID_LOG(FColor::Green, TEXT("%s PossessedBy. Initial MaxWalkSpeed set to: %.1f (Base: %.1f * Multi: %.2f)"),
					*GetName(),
					GetCharacterMovement()->MaxWalkSpeed,
					BaseMoveSpeed,
					CurrentSpeedMultiplier);
			}

			URID_DamageStatics::OnCritDamageOccurred.AddDynamic(this, &AMonsterBaseCharacter::OnCritDamageOccurred);
			// --- [ �ڵ� ���� �� ] ---
		}

		RID_LOG(FColor::Green, TEXT("%s PossessedBy. All ASC Delegates Bound."), *GetName());
	}
}


void AMonsterBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// --- [ �ڡڡ� �ڵ� ���� �ڡڡ� ] ---
	// ��������Ʈ ���ε� ������ PossessedBy�� ��� �̵��Ǿ����ϴ�.
	// BeginPlay������ ASC �ʱ�ȭ�� ȣ���ϴ� ���� �����մϴ�.
	if (AbilitySystemComponent)
	{
		// PossessedBy���� �̹� Init�� ������, ���� AI�� �ƴ� �÷��̾ �����ϴ� ��
		// �ٸ� ��Ȳ�� ����� BeginPlay������ Init�� ȣ���ϴ� ���� �����մϴ�.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	// --- [ �ڵ� ���� �� ] ---

	if (!HasAuthority() && CurrentWaveMaterial)
	{
		OnRep_WaveMaterial();
	}
}

void AMonsterBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// (������ ����, ��� ���� ������ MonsterAttributeSet::PostGameplayEffectExecute�� �̵���)
	if (Data.NewValue <= 0.f) {}
}

// --- [ �ڡڡ� �ڵ� �߰� �ڡڡ� ] ---
/** MoveSpeed �Ӽ��� ����� �� ȣ��Ǵ� �ݹ� (GE ����/���� ��) */
void AMonsterBaseCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		// 1. �� �Ӽ� �� (����)�� �����ɴϴ�. (��: 1.0, 0.5, 0.0)
		const float NewSpeedMultiplier = Data.NewValue;

		// 2. (BaseMoveSpeed * ����)�� ���� MaxWalkSpeed�� ����Ͽ� ������Ʈ�մϴ�.
		MoveComp->MaxWalkSpeed = BaseMoveSpeed * NewSpeedMultiplier;

		RID_LOG(FColor::Blue, TEXT("%s Final MoveSpeed set to: %.1f (Base: %.1f * Multiplier: %.2f)"),
			*GetName(),
			MoveComp->MaxWalkSpeed,
			BaseMoveSpeed,
			NewSpeedMultiplier);
	}
}
// --- [ �ڵ� �߰� �� ] ---


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

	// ����Ʈ ���
	if (EffectData->HitEffect)
	{
		FVector SpawnLocation = GetActorLocation();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EffectData->HitEffect, SpawnLocation);
	}

	// ���� ���
	if (EffectData->HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EffectData->HitSound, GetActorLocation());
	}
}

/** State.Stun �±װ� ����Ǿ��� �� ȣ��Ǵ� �ݹ� */
void AMonsterBaseCharacter::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bool bIsStunned = NewCount > 0;

	// (����: ���Ͱ� ������ �� �ϹǷ� AI ����(BT)�� ���� �ʿ䰡 ����)
	// (GE �������Ʈ���� MoveSpeed�� 0���� ����� ������ �̵��� �����մϴ�)

	// �ִϸ��̼�/����Ʈ ��� ���� �������Ʈ �̺�Ʈ ȣ��
	OnStunStateChanged(bIsStunned);

	if (bIsStunned)
	{
		RID_LOG(FColor::Red, TEXT("%s STUN STARTED (BP Event Called)"), *GetName());
	}
	else
	{
		RID_LOG(FColor::Green, TEXT("%s STUN ENDED (BP Event Called)"), *GetName());
	}
}

/** State.Slow �±װ� ����Ǿ��� �� ȣ��Ǵ� �ݹ� */
void AMonsterBaseCharacter::OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bool bIsSlowed = NewCount > 0;

	// �ִϸ��̼�/����Ʈ ��� ���� �������Ʈ �̺�Ʈ ȣ��
	OnSlowStateChanged(bIsSlowed);

	if (bIsSlowed)
	{
		RID_LOG(FColor::Cyan, TEXT("%s SLOW STARTED (BP Event Called)"), *GetName());
	}
	else
	{
		RID_LOG(FColor::Blue, TEXT("%s SLOW ENDED (BP Event Called)"), *GetName());
	}
}

/** ġ��Ÿ ��������Ʈ �ڵ鷯 ���� */
void AMonsterBaseCharacter::OnCritDamageOccurred(AActor* TargetActor, float CritDamageAmount)
{
	// 1. �̺�Ʈ ����� �� ���Ͱ� �ƴϰų�, ���� Ŭ������ ������ ����
	if (TargetActor != this || !DamageTextWidgetClass)
	{
		return;
	}

	// 2. �� ���Ͱ� ���� �÷��̾� ȭ�鿡 ������ �ϹǷ� ���� PC�� �ʿ�
	// (��Ƽ�÷��� ȯ�濡���� �� PC�� ���� �÷��̾����� �߰� Ȯ�� �ʿ�)
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	// 3. ���� ����
	UDamageTextWidget* DamageWidget = CreateWidget<UDamageTextWidget>(PC, DamageTextWidgetClass);
	if (DamageWidget)
	{
		// 4. ������ �ؽ�Ʈ ���� (��: "1050!")
		FString DamageString = FString::Printf(TEXT("%.0f!"), CritDamageAmount);
		DamageWidget->SetDamageText(FText::FromString(DamageString));

		// 5. ������ ���� ��ġ�� ��ũ�� ��ġ�� ��ȯ
		FVector2D ScreenPosition;
		if (PC->ProjectWorldLocationToScreen(GetActorLocation() + FVector(0, 0, 50.f), ScreenPosition)) // �Ӹ� �� 50cm
		{
			// 6. ����Ʈ�� �߰��ϰ� �ִϸ��̼� ���
			DamageWidget->SetPositionInViewport(ScreenPosition);
			DamageWidget->AddToViewport();
			DamageWidget->PlayRiseAndFade();
			UE_LOG(LogTemp, Warning, TEXT("CritDam Ocurred"));
		}
	}
}