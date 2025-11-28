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
	bIsButtonActionWindowActive = false;

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

			TryBindPlayerState();
		}
		// -----------------------------
	}
}

void ARamdomItemDefensePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

// [추가] 재귀적 바인딩 함수 구현
void ARamdomItemDefensePlayerController::TryBindPlayerState()
{
	MyPlayerStateRef = GetPlayerState<AMyPlayerState>();

	if (MyPlayerStateRef)
	{
		// 1. (일반) 라운드 선택 델리게이트 바인딩
		if (!MyPlayerStateRef->OnChoiceCountChangedDelegate.IsAlreadyBound(this, &ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged))
		{
			MyPlayerStateRef->OnChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerChoiceCountChanged);
			OnPlayerChoiceCountChanged(MyPlayerStateRef->GetChoiceCount());
		}

		// 2. (신규) 흔함 아이템 선택 델리게이트 바인딩
		if (!MyPlayerStateRef->OnCommonItemChoiceCountChangedDelegate.IsAlreadyBound(this, &ARamdomItemDefensePlayerController::OnPlayerCommonChoiceCountChanged))
		{
			MyPlayerStateRef->OnCommonItemChoiceCountChangedDelegate.AddDynamic(this, &ARamdomItemDefensePlayerController::OnPlayerCommonChoiceCountChanged);
			OnPlayerCommonChoiceCountChanged(MyPlayerStateRef->GetCommonItemChoiceCount());
		}

		UE_LOG(LogRamdomItemDefense, Log, TEXT("PlayerController: PlayerState Bound Successfully!"));
	}
	else
	{
		// [★★★ 핵심 수정 ★★★] 실패 시 다음 틱에 자기 자신(TryBindPlayerState)을 다시 호출
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ARamdomItemDefensePlayerController::TryBindPlayerState);
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
	
		if (ButtonAction_Q_Action) EnhancedInputComponent->BindAction(ButtonAction_Q_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_Q);
		if (ButtonAction_W_Action) EnhancedInputComponent->BindAction(ButtonAction_W_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_W);
		if (ButtonAction_E_Action) EnhancedInputComponent->BindAction(ButtonAction_E_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_E);
		if (ButtonAction_R_Action) EnhancedInputComponent->BindAction(ButtonAction_R_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_R);
		if (ButtonAction_A_Action) EnhancedInputComponent->BindAction(ButtonAction_A_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_A);
		if (ButtonAction_S_Action) EnhancedInputComponent->BindAction(ButtonAction_S_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_S);
		if (ButtonAction_D_Action) EnhancedInputComponent->BindAction(ButtonAction_D_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_D);
		if (ButtonAction_F_Action) EnhancedInputComponent->BindAction(ButtonAction_F_Action, ETriggerEvent::Started, this, &ARamdomItemDefensePlayerController::OnButtonAction_F);
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
	else
	{
		// 이 로그가 계속 뜬다면 UI가 가로막고 있는 것입니다.
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Move Trace Failed! Cursor is blocked by UI or Sky."));
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

/** (PlayerState로부터 받은) 클라이언트 RPC 구현 */
void ARamdomItemDefensePlayerController::Client_OnShowButtonActionUI_Implementation(float TimingWindow, EButtonActionKey KeyToPress)
{
	// (클라이언트에서 실행됨)
	bIsButtonActionWindowActive = true;
	RequiredButtonActionKey = KeyToPress;
	ButtonActionWindowEndTime = GetWorld()->GetTimeSeconds() + TimingWindow;

	if (MainHUDInstance)
	{
		// [ ★★★ 수정 ★★★ ]
		// WBP_MainHUD의 "ShowButtonActionPrompt" 이벤트 호출
		MainHUDInstance->ShowButtonActionPrompt(KeyToPress, TimingWindow);
	}
}

/** (추가) 서버로부터 최종 결과를 통지받았을 때 */
void ARamdomItemDefensePlayerController::OnButtonActionResult(bool bWasSuccess, int32 RewardIndex)
{
	bIsButtonActionWindowActive = false;

	if (MainHUDInstance)
	{
		MainHUDInstance->HideButtonActionPrompt();

		// [ ★★★ 수정: 인자 추가 전달 ★★★ ]
		// WBP_MainHUD의 함수도 인자를 받도록 수정해야 함
		MainHUDInstance->ShowButtonActionResult(bWasSuccess, RewardIndex);
	}
}

// 8개의 입력 핸들러 구현 (모두 HandleButtonActionInput 호출)
void ARamdomItemDefensePlayerController::OnButtonAction_Q() { HandleButtonActionInput(EButtonActionKey::Key_Q); }
void ARamdomItemDefensePlayerController::OnButtonAction_W() { HandleButtonActionInput(EButtonActionKey::Key_W); }
void ARamdomItemDefensePlayerController::OnButtonAction_E() { HandleButtonActionInput(EButtonActionKey::Key_E); }
void ARamdomItemDefensePlayerController::OnButtonAction_R() { HandleButtonActionInput(EButtonActionKey::Key_R); }
void ARamdomItemDefensePlayerController::OnButtonAction_A() { HandleButtonActionInput(EButtonActionKey::Key_A); }
void ARamdomItemDefensePlayerController::OnButtonAction_S() { HandleButtonActionInput(EButtonActionKey::Key_S); }
void ARamdomItemDefensePlayerController::OnButtonAction_D() { HandleButtonActionInput(EButtonActionKey::Key_D); }
void ARamdomItemDefensePlayerController::OnButtonAction_F() { HandleButtonActionInput(EButtonActionKey::Key_F); }

/** 모든 버튼 액션 입력을 처리하는 공통 헬퍼 함수 */
void ARamdomItemDefensePlayerController::HandleButtonActionInput(EButtonActionKey KeyPressed)
{
	// (클라이언트에서 실행됨)

	// 1. 활성화된 창이 아니면 무시
	if (!bIsButtonActionWindowActive) return;

	// 2. 클라이언트측 시간 초과 예비 체크 (서버 타임아웃이 메인이지만, 빠른 피드백용)
	if (GetWorld()->GetTimeSeconds() > ButtonActionWindowEndTime)
	{
		bIsButtonActionWindowActive = false;
		return;
	}

	// 3. 입력이 유효함 -> 창을 즉시 닫음 (중복 입력 방지)
	bIsButtonActionWindowActive = false;

	// 4. UMG의 키 프롬프트 UI 즉시 숨김
	if (MainHUDInstance)
	{
		MainHUDInstance->HideButtonActionPrompt();
	}

	// 5. 서버에 결과 보고
	if (MyPlayerStateRef) // OnPossess에서 캐시된 PlayerState
	{
		if (KeyPressed == RequiredButtonActionKey)
		{
			// [성공]
			MyPlayerStateRef->Server_ReportButtonActionSuccess();
		}
		else
		{
			// [실패 - 틀린 키]
			MyPlayerStateRef->Server_ReportButtonActionFailure();
		}
	}
}

void ARamdomItemDefensePlayerController::HideMainHUD()
{
	if (MainHUDInstance && MainHUDInstance->IsInViewport())
	{
		MainHUDInstance->SetVisibility(ESlateVisibility::Hidden);
	}
	// 다른 UI들도 필요하면 숨김 처리
}

void ARamdomItemDefensePlayerController::Client_ShowWaitingUI_Implementation()
{
	// 1. 위젯 생성 및 표시 (기존 코드)
	if (!WaitingWidgetInstance && WaitingWidgetClass)
	{
		WaitingWidgetInstance = CreateWidget<UUserWidget>(this, WaitingWidgetClass);
	}

	if (WaitingWidgetInstance)
	{
		HideMainHUD();
		if (!WaitingWidgetInstance->IsInViewport())
		{
			WaitingWidgetInstance->AddToViewport(10);
		}
		WaitingWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	}
}

void ARamdomItemDefensePlayerController::Client_HideWaitingUI_Implementation()
{
	// 1. 위젯 제거 (기존 코드)
	if (WaitingWidgetInstance)
	{
		WaitingWidgetInstance->RemoveFromParent();
		WaitingWidgetInstance = nullptr;
	}

	if (MainHUDInstance)
	{
		MainHUDInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 2. [수정] 게임 플레이 모드 복구 (확실하게 설정)

	// 마우스 커서 보이기
	bShowMouseCursor = true;

	// ★ 핵심: 마우스 이벤트 다시 켜기 (이게 꺼져있으면 드래그가 꼬일 수 있음)
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	// 입력 모드 재설정
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	// 위젯 포커스를 해제하고 뷰포트로 돌리기 위해 nullptr 지정 가능 (기본값이긴 함)
	InputMode.SetWidgetToFocus(nullptr);

	SetInputMode(InputMode);
}

void ARamdomItemDefensePlayerController::Client_ShowVictoryUI_Implementation()
{
	if (!VictoryWidgetInstance && VictoryWidgetClass)
	{
		VictoryWidgetInstance = CreateWidget<UUserWidget>(this, VictoryWidgetClass);
	}

	if (VictoryWidgetInstance)
	{
		HideMainHUD();
		VictoryWidgetInstance->AddToViewport(20);

	}
}

void ARamdomItemDefensePlayerController::Client_ShowDefeatUI_Implementation()
{
	if (!DefeatWidgetInstance && DefeatWidgetClass)
	{
		DefeatWidgetInstance = CreateWidget<UUserWidget>(this, DefeatWidgetClass);
	}

	if (DefeatWidgetInstance)
	{
		HideMainHUD();
		DefeatWidgetInstance->AddToViewport(20);

	}
}