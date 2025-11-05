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
}

// ... (GetLifetimeReplicatedProps, GetAbilitySystemComponent, PossessedBy, BeginPlay, HandleHealthChanged, HandleMoveSpeedChanged, SetSpawner 함수는 모두 동일) ...

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
		if (AttributeSet)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleMoveSpeedChanged);
			if (GetCharacterMovement())
			{
				const float CurrentSpeedMultiplier = AttributeSet->GetMoveSpeed();
				GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * CurrentSpeedMultiplier;
				RID_LOG(FColor::Green, TEXT("%s PossessedBy. Initial MaxWalkSpeed set to: %.1f (Base: %.1f * Multi: %.2f)"),
					*GetName(),
					GetCharacterMovement()->MaxWalkSpeed,
					BaseMoveSpeed,
					CurrentSpeedMultiplier);
			}
			URID_DamageStatics::OnCritDamageOccurred.AddDynamic(this, &AMonsterBaseCharacter::OnCritDamageOccurred);
		}
		RID_LOG(FColor::Green, TEXT("%s PossessedBy. All ASC Delegates Bound."), *GetName());
	}
}

void AMonsterBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	if (!HasAuthority() && CurrentWaveMaterial)
	{
		OnRep_WaveMaterial();
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
		RID_LOG(FColor::Blue, TEXT("%s Final MoveSpeed set to: %.1f (Base: %.1f * Multiplier: %.2f)"),
			*GetName(),
			GetCharacterMovement()->MaxWalkSpeed,
			BaseMoveSpeed,
			NewSpeedMultiplier);
	}
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

	// --- [ ★★★ 수정 2: AI와 이동 즉시 중지 ★★★ ] ---
	// AI 컨트롤러의 빙의를 해제 (요청하신 사항)
	if (MonsterAIController.IsValid())
	{
		MonsterAIController->UnPossess();
	}

	// (추가) 캐릭터 이동을 즉시 멈추고 비활성화합니다.
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	// --- [ ★★★ 수정 2 끝 ★★★ ] ---

	// 캡슐 콜리전 비활성화 (트레이스, 물리 모두)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// --- [ ★★★ 수정된 타이머 로직 ★★★ ] --- (기존 파일 내용)

	// 랙돌 전환까지의 기본 지연 시간 (몽타주가 없을 경우)
	float RagdollDelay = 0.1f;

	if (DeathMontage)
	{
		float DeathAnimLength = PlayAnimMontage(DeathMontage);

		// (★★★) 몽타주의 블렌드 아웃 시간을 가져옵니다.
		const float BlendOutTime = DeathMontage->BlendOut.GetBlendTime();

		// (★★★) 랙돌 지연 시간 = (전체 길이 - 블렌드 아웃 시간) - (안전 버퍼)
		// 이렇게 하면 블렌드 아웃이 시작되기 *직전에* 랙돌이 활성화됩니다.
		RagdollDelay = FMath::Max(0.01f, DeathAnimLength - BlendOutTime - 0.05f);
	}

	// 랙돌 로직을 LifeSpan 대신 보정된 'RagdollDelay' 타이머로 실행합니다.
	GetWorldTimerManager().SetTimer(
		RagdollTimerHandle,
		this,
		&AMonsterBaseCharacter::GoRagdoll,
		RagdollDelay, // 보정된 지연 시간 사용
		false
	);
	// --- [ ★★★ 수정 끝 ★★★ ] ---
}

// (★★★) 몽타주 종료 후 랙돌로 전환하는 함수 (수정됨)
void AMonsterBaseCharacter::GoRagdoll()
{
	if (GetMesh())	
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetSimulatePhysics(true);
	}
	// 5. 랙돌이 2초(요청대로) 뒤에 사라지도록 최종 LifeSpan 설정
	SetLifeSpan(2.0f);
}


void AMonsterBaseCharacter::SetWaveMaterial(UMaterialInterface* WaveMaterial)
// ... (이하 SetWaveMaterial, OnRep_WaveMaterial, PlayHitEffect, Multicast_PlayHitEffect, OnStunTagChanged, OnSlowTagChanged, OnCritDamageOccurred 함수는 모두 동일) ...
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
	if (bIsDying)
	{
		return;
	}
	bool bIsStunned = NewCount > 0;
	if (MonsterAIController.IsValid())
	{
		UBrainComponent* Brain = MonsterAIController->GetBrainComponent();
		if (Brain)
		{
			if (bIsStunned)
			{
				Brain->PauseLogic(TEXT("Stunned"));
				RID_LOG(FColor::Red, TEXT("%s AI PAUSED (Stun)"), *GetName());
			}
			else
			{
				Brain->ResumeLogic(TEXT("StunEnded"));
				RID_LOG(FColor::Green, TEXT("%s AI RESUMED (Stun End)"), *GetName());
			}
		}
	}
	OnStunStateChanged(bIsStunned);
}

void AMonsterBaseCharacter::OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (bIsDying)
	{
		return;
	}
	bool bIsSlowed = NewCount > 0;
	OnSlowStateChanged(bIsSlowed);
}

void AMonsterBaseCharacter::OnCritDamageOccurred(AActor* TargetActor, float CritDamageAmount)
{
	if (TargetActor != this || !DamageTextWidgetClass)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}
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
			UE_LOG(LogTemp, Warning, TEXT("CritDam Ocurred"));
		}
	}
}