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
	// EnhancedInput �� UI ���� ������ ���� �÷��̾� ��Ʈ�ѷ������� ����Ǿ�� �մϴ�.
	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}

		// --- UI ���� ---
		UWorld* World = GetWorld();
		if (World)
		{
			// 1. ���� HUD ���� �� ����Ʈ �߰�
			if (MainHUDWidgetClass)
			{
				MainHUDInstance = CreateWidget<UMainHUDWidget>(this, MainHUDWidgetClass);
				if (MainHUDInstance)
				{
					MainHUDInstance->AddToViewport();
				}
			}

			// 2. ���� ��ȭâ ���� (������ ���·�)
			if (StatUpgradeWidgetClass)
			{
				StatUpgradeInstance = CreateWidget<UUserWidget>(this, StatUpgradeWidgetClass);
				// (�ʿ�� StatUpgradeInstance->SetVisibility(ESlateVisibility::Hidden);)
			}

			// 3. �κ��丮â ���� (������ ���·�)
			if (InventoryWidgetClass)
			{
				InventoryInstance = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
			}

			// 4. ���� ���� ���� ���� (ȭ�鿡 �ٷ� �߰����� ����)
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

	// ���� �÷��̾� ��Ʈ�ѷ��� ��쿡�� PlayerState ��������Ʈ ���ε�
	if (IsLocalPlayerController())
	{
		MyPlayerStateRef = GetPlayerState<AMyPlayerState>();
		if (MyPlayerStateRef)
		{
			// PlayerState�� ���� Ƚ�� ���� ��������Ʈ�� ��Ʈ�ѷ� �Լ� ���ε�
			MyPlayerStateRef->OnChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged);

			// �ʱ� ���� �ݿ� (���� ���� �� ChoiceCount�� 0 �̻��� ��� ���)
			OnPlayerChoiceCountChanged(MyPlayerStateRef->GetChoiceCount());
		}
		else
		{
			// ���� PlayerState�� �غ���� �ʾҴٸ� ��� �� ��õ�
			// (���� OnPossess �������� ��ȿ������ �����ϰ� ó��)
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
		// ���� Ƚ���� 0���� ũ�� ������ ȭ�鿡 �߰� (�̹� �ִٸ� �ƹ� ���� �� ��)
		if (!RoundChoiceInstance->IsInViewport())
		{
			RoundChoiceInstance->AddToViewport();
		}
	}
	else
	{
		// ���� Ƚ���� 0 ���ϸ� ������ ȭ�鿡�� ���� (�̹� ���ٸ� �ƹ� ���� �� ��)
		if (RoundChoiceInstance->IsInViewport())
		{
			RoundChoiceInstance->RemoveFromParent();
		}
	}
}