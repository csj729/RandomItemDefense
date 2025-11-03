// Source/RamdomItemDefense/Private/RamdomItemDefensePlayerController.cpp (수정)

#include "RamdomItemDefensePlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "InventoryComponent.h"
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
#include "CommonItemChoiceWidget.h" // [코드 추가] 새 위젯 헤더 포함
#include "AbilitySystemBlueprintLibrary.h" // [ ★★★ 코드 추가 ★★★ ]
#include "GameplayTagContainer.h"


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

	// 로컬 플레이어 컨트롤러에서만 Input 및 UI 설정
	if (IsLocalPlayerController())
	{
		// --- Enhanced Input Subsystem 설정 ---
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		// ------------------------------------

		// --- UI 생성 (기존과 동일) ---
		UWorld* World = GetWorld();
		if (World)
		{
			if (MainHUDWidgetClass) MainHUDInstance = CreateWidget<UMainHUDWidget>(this, MainHUDWidgetClass);
			if (StatUpgradeWidgetClass) StatUpgradeInstance = CreateWidget<UUserWidget>(this, StatUpgradeWidgetClass);
			if (InventoryWidgetClass) InventoryInstance = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
			if (RoundChoiceWidgetClass) RoundChoiceInstance = CreateWidget<URoundChoiceWidget>(this, RoundChoiceWidgetClass);
			if (GameOverWidgetClass) GameOverInstance = CreateWidget<UUserWidget>(this, GameOverWidgetClass);

			// --- [ ★★★ 코드 추가 ★★★ ] ---
			// 새 위젯 생성
			if (CommonItemChoiceWidgetClass) CommonItemChoiceInstance = CreateWidget<UCommonItemChoiceWidget>(this, CommonItemChoiceWidgetClass);
			// --- [ ★★★ 코드 추가 끝 ★★★ ] ---


			// --- [핵심 수정] ---
			// MainHUD를 뷰포트에 추가하기 전에
			// 위젯의 '가시성(Visibility)' 설정을 변경합니다.
			if (MainHUDInstance)
			{
				// SelfHitTestInvisible:
				// 위젯 '자신'(배경)은 마우스 클릭에 반응하지(Hit-Test) 않고
				// '자식'(버튼 등)들만 반응하도록 설정합니다.
				MainHUDInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			// --- [수정 끝] ---


			// 초기에는 메인 HUD만 보이도록
			if (MainHUDInstance) MainHUDInstance->AddToViewport();
		}
		// -----------------------------
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
			// (일반) 라운드 선택 델리게이트 바인딩
			MyPlayerStateRef->OnChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged);
			OnPlayerChoiceCountChanged(MyPlayerStateRef->GetChoiceCount());

			// --- [ ★★★ 코드 추가 ★★★ ] ---
			// (신규) 흔함 아이템 선택 델리게이트 바인딩
			MyPlayerStateRef->OnCommonItemChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerCommonChoiceCountChanged);
			OnPlayerCommonChoiceCountChanged(MyPlayerStateRef->GetCommonItemChoiceCount()); // 초기 상태 반영
			// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
		}
		else
		{
			// 아직 PlayerState가 준비되지 않았다면 잠시 후 재시도
			GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
				MyPlayerStateRef = GetPlayerState<AMyPlayerState>();
				if (MyPlayerStateRef)
				{
					MyPlayerStateRef->OnChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged);
					OnPlayerChoiceCountChanged(MyPlayerStateRef->GetChoiceCount());

					// --- [ ★★★ 코드 추가 ★★★ ] ---
					MyPlayerStateRef->OnCommonItemChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerCommonChoiceCountChanged);
					OnPlayerCommonChoiceCountChanged(MyPlayerStateRef->GetCommonItemChoiceCount());
					// --- [ ★★★ 코드 추가 끝 ★★★ ] ---
				}
				});
		}
	}
}

void ARamdomItemDefensePlayerController::Server_RequestCombineItem_Implementation(FName ResultItemID)
{
	// 서버인지 확인
	if (!HasAuthority()) return;

	// 이 컨트롤러가 조종하는 폰(캐릭터)을 가져옵니다.
	ARamdomItemDefenseCharacter* MyCharacter = GetPawn<ARamdomItemDefenseCharacter>();
	if (!MyCharacter) return; // 캐릭터가 없으면 종료

	// 캐릭터에서 인벤토리 컴포넌트를 가져옵니다.
	UInventoryComponent* InventoryComp = MyCharacter->GetInventoryComponent();
	if (!InventoryComp) return; // 인벤토리 컴포넌트가 없으면 종료

	// 인벤토리 컴포넌트의 조합 함수를 호출합니다.
	InventoryComp->CombineItemByResultID(ResultItemID);
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
		EnhancedInputComponent->BindAction(UltimateSkillAction, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnUltimateSkillPressed);
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

/** (일반) 라운드 선택권 횟수 변경 시 */
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

// --- [ ★★★ 코드 추가 ★★★ ] ---
/** (신규) '흔함 아이템 선택권' 횟수 변경 시 */
void ARamdomItemDefensePlayerController::OnPlayerCommonChoiceCountChanged(int32 NewCount)
{
	if (!IsLocalPlayerController() || !CommonItemChoiceInstance) return;

	if (NewCount > 0)
	{
		// 선택 횟수가 0보다 크면 위젯을 화면에 추가
		if (!CommonItemChoiceInstance->IsInViewport())
		{
			CommonItemChoiceInstance->AddToViewport();
		}
	}
	else
	{
		// 선택 횟수가 0 이하면 위젯을 화면에서 제거
		if (CommonItemChoiceInstance->IsInViewport())
		{
			CommonItemChoiceInstance->RemoveFromParent();
		}
	}
}
// --- [ ★★★ 코드 추가 끝 ★★★ ] ---


/** 게임오버 UI 표시 */
void ARamdomItemDefensePlayerController::ShowGameOverUI()
{
	if (!IsLocalPlayerController()) return;

	// 1. 기존 게임 UI 숨기기
	if (MainHUDInstance && MainHUDInstance->IsInViewport()) MainHUDInstance->RemoveFromParent();
	if (StatUpgradeInstance && StatUpgradeInstance->IsInViewport()) StatUpgradeInstance->RemoveFromParent();
	if (InventoryInstance && InventoryInstance->IsInViewport()) InventoryInstance->RemoveFromParent();
	if (RoundChoiceInstance && RoundChoiceInstance->IsInViewport()) RoundChoiceInstance->RemoveFromParent();
	// --- [ ★★★ 코드 추가 ★★★ ] ---
	if (CommonItemChoiceInstance && CommonItemChoiceInstance->IsInViewport()) CommonItemChoiceInstance->RemoveFromParent(); // 게임오버 시 선택권 UI도 숨김
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

	// 2. 게임오버 UI 표시
	if (GameOverInstance && !GameOverInstance->IsInViewport())
	{
		GameOverInstance->AddToViewport();
	}
}

/** 게임오버 UI 숨기기 */
void ARamdomItemDefensePlayerController::HideGameOverUI()
{
	if (!IsLocalPlayerController()) return;

	if (GameOverInstance && GameOverInstance->IsInViewport())
	{
		GameOverInstance->RemoveFromParent();
	}

	// 메인 HUD 다시 표시
	if (MainHUDInstance && !MainHUDInstance->IsInViewport())
	{
		MainHUDInstance->AddToViewport();
	}
}

/** 'G' 키 (UltimateSkillAction)가 눌렸을 때 호출됩니다. */
void ARamdomItemDefensePlayerController::OnUltimateSkillPressed()
{
	ARamdomItemDefenseCharacter* MyCharacter = GetPawn<ARamdomItemDefenseCharacter>();
	if (MyCharacter)
	{
		UAbilitySystemComponent* ASC = MyCharacter->GetAbilitySystemComponent();
		if (ASC)
		{
			// "Event.Attack.Execute.Ultimate" 태그를 가진 어빌리티를 실행 시도합니다.
			// (이 태그는 DefaultGameplayTags.ini에 추가해야 합니다)
			FGameplayTag UltimateTag = FGameplayTag::RequestGameplayTag(FName("Event.Attack.Execute.Ultimate"));
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(UltimateTag), true);
		}
	}
}	