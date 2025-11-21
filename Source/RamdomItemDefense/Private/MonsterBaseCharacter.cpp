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

		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.ArmorShred")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnArmorShredTagChanged);

		if (AttributeSet)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleMoveSpeedChanged);
			if (GetCharacterMovement())
			{
				const float CurrentSpeedMultiplier = AttributeSet->GetMoveSpeed();
				GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * CurrentSpeedMultiplier;
				/*RID_LOG(FColor::Green, TEXT("%s PossessedBy. Initial MaxWalkSpeed set to: %.1f (Base: %.1f * Multi: %.2f)"),
					*GetName(),
					GetCharacterMovement()->MaxWalkSpeed,
					BaseMoveSpeed,
					CurrentSpeedMultiplier);*/
			}
			URID_DamageStatics::OnCritDamageOccurred.AddDynamic(this, &AMonsterBaseCharacter::OnCritDamageOccurred);
		}
		//RID_LOG(FColor::Green, TEXT("%s PossessedBy. All ASC Delegates Bound."), *GetName());
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
	}
}

void AMonsterBaseCharacter::SetSpawner(AMonsterSpawner* InSpawner)
{
	MySpawner = InSpawner;

	// [추가] 스포너에 설정된 경로를 AI 컨트롤러에게 전달
	if (InSpawner && InSpawner->PatrolPathActor)
	{
		// 1. AI 컨트롤러 캐싱 확인 (혹시 null일 경우 대비)
		if (!MonsterAIController.IsValid())
		{
			MonsterAIController = Cast<AMonsterAIController>(GetController());
		}

		// 2. AI 컨트롤러에게 경로 주입
		if (MonsterAIController.IsValid())
		{
			MonsterAIController->SetPatrolPath(InSpawner->PatrolPathActor);
		}
	}
}

void AMonsterBaseCharacter::Multicast_PlayMontage_Implementation(UAnimMontage* MontageToPlay)
{
	// 서버와 모든 클라이언트에서 실행됨
	if (MontageToPlay)
	{
		PlayAnimMontage(MontageToPlay);
	}
}

void AMonsterBaseCharacter::Die(AActor* Killer)
{
	if (bIsDying) { return; }
	bIsDying = true;

	if (MySpawner) { MySpawner->OnMonsterKilled(); }

	OnStunStateChanged(false);
	OnSlowStateChanged(false);
	OnArmorShredStateChanged(false);
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

	if (HasAuthority())
	{
		// 파티클 템플릿 인자는 제거 시 필요 없으므로 nullptr 전달
		Multicast_SetStatusEffectState(FGameplayTag::RequestGameplayTag(FName("State.Slow")), false, nullptr);
		Multicast_SetStatusEffectState(FGameplayTag::RequestGameplayTag(FName("State.ArmorShred")), false, nullptr);
	}

	// 캡슐 콜리전 비활성화 (트레이스, 물리 모두)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// --- [ ★★★ 수정된 타이머 로직 ★★★ ] --- (기존 파일 내용)

	// 랙돌 전환까지의 기본 지연 시간 (몽타주가 없을 경우)
	float RagdollDelay = 0.1f;

	if (DeathMontage)
	{
		Multicast_PlayMontage(DeathMontage);

		const float DeathAnimLength = DeathMontage->GetPlayLength(); // 몽타주 길이 직접 조회
		const float BlendOutTime = DeathMontage->BlendOut.GetBlendTime();
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

void AMonsterBaseCharacter::GoRagdoll()
{
	// 1. 모든 클라이언트(본인 포함)에게 랙돌 명령 전송
	Multicast_GoRagdoll();

	// 2. (서버 전용) 액터 소멸 타이머 설정 (랙돌 전환 2초 후 삭제)
	SetLifeSpan(2.0f);
}

// [추가] 실제 랙돌 로직 (서버 + 모든 클라이언트 실행)
void AMonsterBaseCharacter::Multicast_GoRagdoll_Implementation()
{
	// 1. 캡슐 콜리전 완전 제거 (시체에 길막힘 방지)
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 2. 메쉬 물리 시뮬레이션 활성화
	if (GetMesh())
	{
		// 랙돌용 콜리전 프로필 설정 (PhysicsBody 등)
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		// 물리 시뮬레이션 켜기
		GetMesh()->SetSimulatePhysics(true);
		// (선택) 애니메이션 인스턴스의 몽타주 강제 중지
		if (GetMesh()->GetAnimInstance())
		{
			GetMesh()->GetAnimInstance()->StopAllMontages(0.0f);
		}
	}

	// 3. 무브먼트 컴포넌트 정지 (혹시 모를 이동 방지)
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
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
				//RID_LOG(FColor::Red, TEXT("%s AI PAUSED (Stun)"), *GetName());
			}
			else
			{
				Brain->ResumeLogic(TEXT("StunEnded"));
				//RID_LOG(FColor::Green, TEXT("%s AI RESUMED (Stun End)"), *GetName());
			}
		}
	}
	OnStunStateChanged(bIsStunned);
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

// 1. 멀티캐스트 함수 구현 (나이아가라 버전)
void AMonsterBaseCharacter::Multicast_SetStatusEffectState_Implementation(FGameplayTag StatusTag, bool bIsActive, UNiagaraSystem* EffectTemplate)
{
	if (bIsActive)
	{
		// [켜기] 이미 켜져있지 않다면 이펙트 생성
		if (!ActiveStatusParticles.Contains(StatusTag) && EffectTemplate && GetMesh())
		{
			// 나이아가라 스폰 (bAutoDestroy = false)
			UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAttached(
				EffectTemplate,
				GetMesh(),
				NAME_None, // 필요시 소켓 이름 지정 (예: "Root")
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				false // bAutoDestroy = false (지속 이펙트이므로 수동 관리)
			);

			if (NC)
			{
				ActiveStatusParticles.Add(StatusTag, NC);
			}
		}
	}
	else
	{
		// [끄기] 맵에 있다면 찾아서 제거
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

// 2. OnSlowTagChanged 수정
void AMonsterBaseCharacter::OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (bIsDying) return;

	bool bIsSlowed = NewCount > 0;

	// [서버] 멀티캐스트 호출
	if (HasAuthority())
	{
		Multicast_SetStatusEffectState(Tag, bIsSlowed, SlowEffectTemplate);
	}

	OnSlowStateChanged(bIsSlowed);
}

// 3. OnArmorShredTagChanged 수정
void AMonsterBaseCharacter::OnArmorShredTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (bIsDying) return;

	bool bIsShredded = NewCount > 0;

	// [서버] 멀티캐스트 호출
	if (HasAuthority())
	{
		Multicast_SetStatusEffectState(Tag, bIsShredded, ArmorShredEffectTemplate);
	}

	OnArmorShredStateChanged(bIsShredded);
}

