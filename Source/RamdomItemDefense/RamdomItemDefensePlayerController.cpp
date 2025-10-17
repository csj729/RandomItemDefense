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
	// 짧은 클릭이었는지 확인하는 것은 기존과 동일합니다.
	if (FollowTime <= ShortPressThreshold)
	{
		// 마우스 커서 아래에 무엇이 있는지 확인합니다.
		FHitResult Hit;
		GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

		if (Hit.bBlockingHit)
		{
			// --- 디버그 ---: 어떤 액터가 클릭되었는지 이름 출력
			if (GEngine && Hit.GetActor())
			{
				FString HitActorName = Hit.GetActor()->GetName();
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("Clicked Actor: %s"), *HitActorName));
			}
			// --- 디버그 끝 ---

			// 현재 조종 중인 내 캐릭터를 가져옵니다.
			ARamdomItemDefenseCharacter* MyCharacter = Cast<ARamdomItemDefenseCharacter>(GetPawn());
			if (!MyCharacter)
			{
				FollowTime = 0.f;
				return;
			}

			// 클릭된 액터를 몬스터로 형변환(Cast) 시도합니다.
			AMonsterBaseCharacter* ClickedMonster = Cast<AMonsterBaseCharacter>(Hit.GetActor());

			UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
			if (!NavSys) return;

			if (ClickedMonster) // 형변환 성공 -> 몬스터를 클릭했습니다
			{
				// --- 디버그 ---: 캐스팅 성공 메시지 출력
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("SUCCESS: Cast to MonsterBaseCharacter"));
				}
				// --- 디버그 끝 ---

				const UMyAttributeSet* AttributeSet = MyCharacter->GetAttributeSet();
				if (AttributeSet)
				{
					const float AttackRange = AttributeSet->GetAttackRange();
					const float Distance = MyCharacter->GetDistanceTo(ClickedMonster);

					if (Distance > AttackRange) // 범위 밖이면
					{
						// "저 몬스터를 공격할 예정이다" 라고 캐릭터에게 알리고 이동만 시작합니다.
						MyCharacter->SetPendingManualTarget(ClickedMonster);

						// --- 핵심 로직 ---
						// 1. 이동 완료 시 호출될 함수를 캐릭터의 OnMoveCompleted 함수로 지정합니다.
						FAIMoveRequest MoveRequest(ClickedMonster);

						// 이동 로직 다시 수정해야됨

						NavSys->SimpleMoveToLocation(this, ClickedMonster->GetActorLocation());

						// MoveTo 완료 콜백 바인딩
						if (UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent())
						{
							// OnRequestFinished는 FPathFollowingResult 기반 단일 델리게이트
							PathComp->OnRequestFinished.AddUObject(MyCharacter, &ARamdomItemDefenseCharacter::OnMoveCompleted);
						}
					}
					else // 범위 안이면
					{
						// 즉시 공격 타겟으로 설정하고 그 자리에 멈춥니다.
						MyCharacter->SetManualTarget(ClickedMonster);
						AIController->StopMovement();
					}
				}
			}
			else // 형변환 실패 -> 바닥이나 다른 오브젝트를 클릭했습니다!
			{
				// --- 디버그 ---: 캐스팅 실패 메시지 출력
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("FAILED: Cast to MonsterBaseCharacter"));
				}
				// --- 디버그 끝 ---

				// 모든 타겟(수동, 예정, 자동)을 깨끗하게 비우고 이동만 합니다.
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
