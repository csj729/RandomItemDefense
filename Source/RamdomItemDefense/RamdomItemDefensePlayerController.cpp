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
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
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
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void ARamdomItemDefensePlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &ARamdomItemDefensePlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &ARamdomItemDefensePlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &ARamdomItemDefensePlayerController::OnSetDestinationReleased);

		// Setup touch input events
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &ARamdomItemDefensePlayerController::OnTouchTriggered);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &ARamdomItemDefensePlayerController::OnTouchReleased);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &ARamdomItemDefensePlayerController::OnTouchReleased);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ARamdomItemDefensePlayerController::OnInputStarted()
{
	StopMovement();
}

// Triggered every frame when the input is held down
void ARamdomItemDefensePlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
	
	// Move towards mouse pointer or touch
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void ARamdomItemDefensePlayerController::OnSetDestinationReleased()
{
	// ª�� Ŭ���̾����� Ȯ���ϴ� ���� ������ �����մϴ�.
	if (FollowTime <= ShortPressThreshold)
	{
		// ���콺 Ŀ�� �Ʒ��� ������ �ִ��� Ȯ���մϴ�.
		FHitResult Hit;
		GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

		if (Hit.bBlockingHit)
		{
			// --- ����� ---: � ���Ͱ� Ŭ���Ǿ����� �̸� ���
			if (GEngine && Hit.GetActor())
			{
				FString HitActorName = Hit.GetActor()->GetName();
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("Clicked Actor: %s"), *HitActorName));
			}
			// --- ����� �� ---

			// ���� ���� ���� �� ĳ���͸� �����ɴϴ�.
			ARamdomItemDefenseCharacter* MyCharacter = Cast<ARamdomItemDefenseCharacter>(GetPawn());
			if (!MyCharacter)
			{
				FollowTime = 0.f;
				return;
			}

			// Ŭ���� ���͸� ���ͷ� ����ȯ(Cast) �õ��մϴ�.
			AMonsterBaseCharacter* ClickedMonster = Cast<AMonsterBaseCharacter>(Hit.GetActor());

			UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
			if (!NavSys) return;

			if (ClickedMonster) // ����ȯ ���� -> ���͸� Ŭ���߽��ϴ�
			{
				// --- ����� ---: ĳ���� ���� �޽��� ���
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("SUCCESS: Cast to MonsterBaseCharacter"));
				}
				// --- ����� �� ---

				const UMyAttributeSet* AttributeSet = MyCharacter->GetAttributeSet();
				if (AttributeSet)
				{
					const float AttackRange = AttributeSet->GetAttackRange();
					const float Distance = MyCharacter->GetDistanceTo(ClickedMonster);

					if (Distance > AttackRange) // ���� ���̸�
					{
						// "�� ���͸� ������ �����̴�" ��� ĳ���Ϳ��� �˸��� �̵��� �����մϴ�.
						MyCharacter->SetPendingManualTarget(ClickedMonster);

						// --- �ٽ� ���� ---
						// 1. �̵� �Ϸ� �� ȣ��� �Լ��� ĳ������ OnMoveCompleted �Լ��� �����մϴ�.
						FAIMoveRequest MoveRequest(ClickedMonster);

						// �̵� ���� �ٽ� �����ؾߵ�

						NavSys->SimpleMoveToLocation(this, ClickedMonster->GetActorLocation());

						// MoveTo �Ϸ� �ݹ� ���ε�
						if (UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent())
						{
							// OnRequestFinished�� FPathFollowingResult ��� ���� ��������Ʈ
							PathComp->OnRequestFinished.AddUObject(MyCharacter, &ARamdomItemDefenseCharacter::OnMoveCompleted);
						}
					}
					else // ���� ���̸�
					{
						// ��� ���� Ÿ������ �����ϰ� �� �ڸ��� ����ϴ�.
						MyCharacter->SetManualTarget(ClickedMonster);
						AIController->StopMovement();
					}
				}
			}
			else // ����ȯ ���� -> �ٴ��̳� �ٸ� ������Ʈ�� Ŭ���߽��ϴ�!
			{
				// --- ����� ---: ĳ���� ���� �޽��� ���
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("FAILED: Cast to MonsterBaseCharacter"));
				}
				// --- ����� �� ---

				// ��� Ÿ��(����, ����, �ڵ�)�� �����ϰ� ���� �̵��� �մϴ�.
				MyCharacter->ClearAllTargets();
				AIController->MoveToLocation(Hit.Location);
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, Hit.Location, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
			}
		}
	}

	FollowTime = 0.f;
}

// Triggered every frame when the input is held down
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
