// Copyright Epic Games, Inc. All Rights Reserved.

#include "RamdomItemDefensePlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "RamdomItemDefenseCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "MonsterBaseCharacter.h"
#include "Engine/Engine.h"
#include "AttackComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.h"
#include "RoundChoiceWidget.h"
#include "MyPlayerState.h"
#include "Engine/LocalPlayer.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

ARamdomItemDefensePlayerController::ARamdomItemDefensePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

void ARamdomItemDefensePlayerController::BeginPlay()
{
	Super::BeginPlay();
	// EnhancedInput 및 UI 생성 로직은 로컬 플레이어 컨트롤러에서만 실행되어야 합니다.
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		// --- UI 생성 ---
		UWorld* World = GetWorld();
		if (World)
		{
			// 1. 메인 HUD 생성 및 뷰포트 추가
			if (MainHUDWidgetClass)
			{
				MainHUDInstance = CreateWidget<UMainHUDWidget>(this, MainHUDWidgetClass);
				if (MainHUDInstance)
				{
					MainHUDInstance->AddToViewport();
				}
			}

			// 2. 스탯 강화창 생성 (숨겨진 상태로)
			if (StatUpgradeWidgetClass)
			{
				StatUpgradeInstance = CreateWidget<UUserWidget>(this, StatUpgradeWidgetClass);
				// (필요시 StatUpgradeInstance->SetVisibility(ESlateVisibility::Hidden);)
			}

			// 3. 인벤토리창 생성 (숨겨진 상태로)
			if (InventoryWidgetClass)
			{
				InventoryInstance = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
			}

			// 4. 라운드 선택 위젯 생성 (화면에 바로 추가하지 않음)
			if (RoundChoiceWidgetClass)
			{
				RoundChoiceInstance = CreateWidget<URoundChoiceWidget>(this, RoundChoiceWidgetClass);
			}
		}
	}
}

void ARamdomItemDefensePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 로컬 플레이어 컨트롤러인 경우에만 PlayerState 델리게이트 바인딩
	if (IsLocalPlayerController())
	{
		MyPlayerStateRef = GetPlayerState<AMyPlayerState>();
		if (MyPlayerStateRef)
		{
			// PlayerState의 선택 횟수 변경 델리게이트에 컨트롤러 함수 바인딩
			MyPlayerStateRef->OnChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged);

			// 초기 상태 반영 (게임 시작 시 ChoiceCount가 0 이상일 경우 대비)
			OnPlayerChoiceCountChanged(MyPlayerStateRef->GetChoiceCount());
		}
		else
		{
			// 아직 PlayerState가 준비되지 않았다면 잠시 후 재시도
			// (보통 OnPossess 시점에는 유효하지만 안전하게 처리)
			GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
				MyPlayerStateRef = GetPlayerState<AMyPlayerState>();
				if (MyPlayerStateRef)
				{
					MyPlayerStateRef->OnChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged);
					OnPlayerChoiceCountChanged(MyPlayerStateRef->GetChoiceCount());
				}
				});
		}
	}
}

void ARamdomItemDefensePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ARamdomItemDefensePlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ARamdomItemDefensePlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ARamdomItemDefensePlayerController::OnSetDestinationReleased);

		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &ARamdomItemDefensePlayerController::OnTouchTriggered);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &ARamdomItemDefensePlayerController::OnTouchReleased);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &ARamdomItemDefensePlayerController::OnTouchReleased);
	}
}

void ARamdomItemDefensePlayerController::OnInputStarted()
{
	StopMovement();
}

void ARamdomItemDefensePlayerController::OnSetDestinationTriggered()
{
	FollowTime += GetWorld()->GetDeltaSeconds();

	FHitResult Hit;
	bool bHitSuccessful = bIsTouch ? GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit)
		: GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void ARamdomItemDefensePlayerController::OnSetDestinationReleased()
{
	if (FollowTime <= ShortPressThreshold)
	{
		FHitResult Hit;
		GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

		if (Hit.bBlockingHit)
		{
			ARamdomItemDefenseCharacter* MyCharacter = Cast<ARamdomItemDefenseCharacter>(GetPawn());
			if (!MyCharacter)
			{
				FollowTime = 0.f;
				return;
			}

			UAttackComponent* AttackComp = MyCharacter->GetAttackComponent();
			if (!AttackComp)
			{
				FollowTime = 0.f;
				return;
			}

			AMonsterBaseCharacter* ClickedMonster = Cast<AMonsterBaseCharacter>(Hit.GetActor());

			if (ClickedMonster)
			{
				AttackComp->OrderAttack(ClickedMonster);
			}
			else
			{
				AttackComp->ClearAllTargets();
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.Location);
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, Hit.Location, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
			}
		}
	}

	FollowTime = 0.f;
}

void ARamdomItemDefensePlayerController::OnTouchTriggered()
{
	bIsTouch = true;
	OnSetDestinationTriggered();
}

void ARamdomItemDefensePlayerController::OnTouchReleased()
{
	bIsTouch = false;
	OnSetDestinationReleased();
}

void ARamdomItemDefensePlayerController::ToggleStatUpgradeWidget()
{
	if (StatUpgradeInstance)
	{
		if (StatUpgradeInstance->IsInViewport())
		{
			StatUpgradeInstance->RemoveFromParent();
		}
		else
		{
			StatUpgradeInstance->AddToViewport();
		}
	}
}

void ARamdomItemDefensePlayerController::ToggleInventoryWidget()
{
	if (InventoryInstance)
	{
		if (InventoryInstance->IsInViewport())
		{
			InventoryInstance->RemoveFromParent();
		}
		else
		{
			InventoryInstance->AddToViewport();
		}
	}
}

void ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged(int32 NewCount)
{
	if (!IsLocalPlayerController() || !RoundChoiceInstance) return;

	if (NewCount > 0)
	{
		// 선택 횟수가 0보다 크면 위젯을 화면에 추가 (이미 있다면 아무 동작 안 함)
		if (!RoundChoiceInstance->IsInViewport())
		{
			RoundChoiceInstance->AddToViewport();
		}
	}
	else
	{
		// 선택 횟수가 0 이하면 위젯을 화면에서 제거 (이미 없다면 아무 동작 안 함)
		if (RoundChoiceInstance->IsInViewport())
		{
			RoundChoiceInstance->RemoveFromParent();
		}
	}
}