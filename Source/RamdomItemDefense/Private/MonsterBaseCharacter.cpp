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
#include "RID_DamageStatics.h"         // 델리게이트 구독용
#include "DamageTextWidget.h"        // 데미지 위젯 클래스
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

/** (AIController가 폰에 빙의될 때 호출됨) */
void AMonsterBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 1. AI 컨트롤러 캐시
	MonsterAIController = Cast<AMonsterAIController>(NewController);

	// 2. ASC가 유효한지 확인 (유효해야 함)
	if (AbilitySystemComponent)
	{
		// 3. ASC 초기화 (중요!)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 4. 태그 델리게이트 바인딩
		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.Stun")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnStunTagChanged);

		AbilitySystemComponent->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("State.Slow")),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &AMonsterBaseCharacter::OnSlowTagChanged);

		// 5. 속성 델리게이트 바인딩
		if (AttributeSet)
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleMoveSpeedChanged);

			// --- [ ★★★ 코드 수정 ★★★ ] ---
			// 6. (중요) 현재 속성 값을 가져와 *계산* 후 이동 컴포넌트에 적용
			// (GE가 이미 적용된 상태로 스폰될 수 있으므로 초기값 설정)
			if (GetCharacterMovement())
			{
				// (BaseMoveSpeed * 현재 MoveSpeed 속성 배율)로 최종 속도 계산
				// 만약 GE_MonsterStatInit가 아직 적용 안돼서 배율이 0이면, 속도는 0이 됨 (정상)
				const float CurrentSpeedMultiplier = AttributeSet->GetMoveSpeed();
				GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * CurrentSpeedMultiplier;

				RID_LOG(FColor::Green, TEXT("%s PossessedBy. Initial MaxWalkSpeed set to: %.1f (Base: %.1f * Multi: %.2f)"),
					*GetName(),
					GetCharacterMovement()->MaxWalkSpeed,
					BaseMoveSpeed,
					CurrentSpeedMultiplier);
			}

			URID_DamageStatics::OnCritDamageOccurred.AddDynamic(this, &AMonsterBaseCharacter::OnCritDamageOccurred);
			// --- [ 코드 수정 끝 ] ---
		}

		RID_LOG(FColor::Green, TEXT("%s PossessedBy. All ASC Delegates Bound."), *GetName());
	}
}


void AMonsterBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// --- [ ★★★ 코드 수정 ★★★ ] ---
	// 델리게이트 바인딩 로직은 PossessedBy로 모두 이동되었습니다.
	// BeginPlay에서는 ASC 초기화만 호출하는 것이 안전합니다.
	if (AbilitySystemComponent)
	{
		// PossessedBy에서 이미 Init을 했지만, 만약 AI가 아닌 플레이어가 빙의하는 등
		// 다른 상황을 대비해 BeginPlay에서도 Init을 호출하는 것은 안전합니다.
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	// --- [ 코드 수정 끝 ] ---

	if (!HasAuthority() && CurrentWaveMaterial)
	{
		OnRep_WaveMaterial();
	}
}

void AMonsterBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// (기존과 동일, 사망 판정 로직은 MonsterAttributeSet::PostGameplayEffectExecute로 이동함)
	if (Data.NewValue <= 0.f) {}
}

// --- [ ★★★ 코드 추가 ★★★ ] ---
/** MoveSpeed 속성이 변경될 때 호출되는 콜백 (GE 적용/해제 시) */
void AMonsterBaseCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (MoveComp)
	{
		// 1. 새 속성 값 (배율)을 가져옵니다. (예: 1.0, 0.5, 0.0)
		const float NewSpeedMultiplier = Data.NewValue;

		// 2. (BaseMoveSpeed * 배율)로 최종 MaxWalkSpeed를 계산하여 업데이트합니다.
		MoveComp->MaxWalkSpeed = BaseMoveSpeed * NewSpeedMultiplier;

		RID_LOG(FColor::Blue, TEXT("%s Final MoveSpeed set to: %.1f (Base: %.1f * Multiplier: %.2f)"),
			*GetName(),
			MoveComp->MaxWalkSpeed,
			BaseMoveSpeed,
			NewSpeedMultiplier);
	}
}
// --- [ 코드 추가 끝 ] ---


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

	// 이펙트 재생
	if (EffectData->HitEffect)
	{
		FVector SpawnLocation = GetActorLocation();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EffectData->HitEffect, SpawnLocation);
	}

	// 사운드 재생
	if (EffectData->HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EffectData->HitSound, GetActorLocation());
	}
}

/** State.Stun 태그가 변경되었을 때 호출되는 콜백 */
void AMonsterBaseCharacter::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bool bIsStunned = NewCount > 0;

	// (참고: 몬스터가 공격을 안 하므로 AI 로직(BT)을 멈출 필요가 없음)
	// (GE 블루프린트에서 MoveSpeed를 0으로 만드는 것으로 이동을 제어합니다)

	// 애니메이션/이펙트 제어를 위해 블루프린트 이벤트 호출
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

/** State.Slow 태그가 변경되었을 때 호출되는 콜백 */
void AMonsterBaseCharacter::OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bool bIsSlowed = NewCount > 0;

	// 애니메이션/이펙트 제어를 위해 블루프린트 이벤트 호출
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

/** 치명타 델리게이트 핸들러 구현 */
void AMonsterBaseCharacter::OnCritDamageOccurred(AActor* TargetActor, float CritDamageAmount)
{
	// 1. 이벤트 대상이 이 몬스터가 아니거나, 위젯 클래스가 없으면 무시
	if (TargetActor != this || !DamageTextWidgetClass)
	{
		return;
	}

	// 2. 이 몬스터가 로컬 플레이어 화면에 보여야 하므로 로컬 PC가 필요
	// (멀티플레이 환경에서는 이 PC가 로컬 플레이어인지 추가 확인 필요)
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	// 3. 위젯 생성
	UDamageTextWidget* DamageWidget = CreateWidget<UDamageTextWidget>(PC, DamageTextWidgetClass);
	if (DamageWidget)
	{
		// 4. 데미지 텍스트 설정 (예: "1050!")
		FString DamageString = FString::Printf(TEXT("%.0f!"), CritDamageAmount);
		DamageWidget->SetDamageText(FText::FromString(DamageString));

		// 5. 몬스터의 월드 위치를 스크린 위치로 변환
		FVector2D ScreenPosition;
		if (PC->ProjectWorldLocationToScreen(GetActorLocation() + FVector(0, 0, 50.f), ScreenPosition)) // 머리 위 50cm
		{
			// 6. 뷰포트에 추가하고 애니메이션 재생
			DamageWidget->SetPositionInViewport(ScreenPosition);
			DamageWidget->AddToViewport();
			DamageWidget->PlayRiseAndFade();
			UE_LOG(LogTemp, Warning, TEXT("CritDam Ocurred"));
		}
	}
}