// Source/RamdomItemDefense/Public/RamdomItemDefensePlayerController.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "ItemTypes.h"
#include "RamdomItemDefensePlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;

// Widget 전방 선언
class UMainHUDWidget;
class UStatUpgradeWidget;
class UInventoryWidget;
class URoundChoiceWidget;
class AMyPlayerState;
class UUserWidget;
class UCommonItemChoiceWidget; // [코드 추가] 새 위젯 전방 선언

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS()
class ARamdomItemDefensePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARamdomItemDefensePlayerController();

	/** (수정) 버튼 액션 Input Actions (Q, W, E, R) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_Q_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_W_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_E_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_R_Action;

	/** (추가) 버튼 액션 Input Actions (A, S, D, F) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_A_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_S_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_D_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ButtonAction_F_Action;

	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float ShortPressThreshold;

	/** FX Class that we will spawn when clicking */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UNiagaraSystem* FXCursor;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SetDestinationClickAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SetDestinationTouchAction;

	/** Ultimate Skill Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UltimateSkillAction;

public:
	/** (추후 구현) 스탯 강화창 UI를 토글합니다. */
	void ToggleStatUpgradeWidget();

	/** (추후 구현) 인벤토리 UI를 토글합니다. */
	void ToggleInventoryWidget();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_RequestCombineItem(FName ResultItemID);

	/** (GameMode에서 호출) 게임오버 UI를 표시합니다. */
	void ShowGameOverUI();
	/** 게임오버 UI를 숨깁니다. (재시작 시 호출될 수 있음) */
	void HideGameOverUI();

	// 대기 UI 켜기
	UFUNCTION(Client, Reliable)
	void Client_ShowWaitingUI();

	// 대기 UI 끄기 (게임 시작)
	UFUNCTION(Client, Reliable)
	void Client_HideWaitingUI();

	// 승리 UI 켜기
	UFUNCTION(Client, Reliable)
	void Client_ShowVictoryUI();

	// 패배 UI 켜기
	UFUNCTION(Client, Reliable)
	void Client_ShowDefeatUI();

	/** (추가) (Called by PlayerState) Tells this client to show the boost UI */
	UFUNCTION(Client, Reliable)
	void Client_OnShowButtonActionUI(float TimingWindow, EButtonActionKey KeyToPress);

	// [ ★★★ 추가 ★★★ ]
	/** (Called by PlayerState) 서버로부터 받은 최종 결과를 HUD에 표시합니다. */
	void OnButtonActionResult(bool bWasSuccess, int32 RewardIndex);

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	virtual void SetupInputComponent() override;

	// To add mapping context
	virtual void BeginPlay();
	virtual void OnPossess(APawn* InPawn) override;

	/** PlayerState의 (일반) ChoiceCount 변경 시 호출될 함수 */
	UFUNCTION()
	void OnPlayerChoiceCountChanged(int32 NewCount);

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	/** PlayerState의 '흔함 아이템 선택권' 변경 시 호출될 함수 */
	UFUNCTION()
	void OnPlayerCommonChoiceCountChanged(int32 NewCount);
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

	/** Input handlers for SetDestination action. */
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();

	/** Input handler for UltimateSkill action. */
	void OnUltimateSkillPressed();

	/** WBP_MainHUD (블루프린트) 클래스를 에디터에서 지정할 변수 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;

	/** WBP_StatUpgrade (블루프린트) 클래스를 에디터에서 지정할 변수 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> StatUpgradeWidgetClass; // 부모를 UStatUpgradeWidget으로 해도 됩니다.

	/** WBP_Inventory (블루프린트) 클래스를 에디터에서 지정할 변수 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass; // 부모를 UInventoryWidget으로 해도 됩니다.

	/** WBP_RoundChoice 블루프린트 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<URoundChoiceWidget> RoundChoiceWidgetClass;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	/** WBP_CommonItemChoice 블루프린트 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UCommonItemChoiceWidget> CommonItemChoiceWidgetClass;

	// --- UI 위젯 클래스 (에디터 할당용) ---
	UPROPERTY(EditDefaultsOnly, Category = "UI|PVP")
	TSubclassOf<UUserWidget> WaitingWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|PVP")
	TSubclassOf<UUserWidget> VictoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|PVP")
	TSubclassOf<UUserWidget> DefeatWidgetClass;

	/** 실제 생성된 메인 HUD 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UMainHUDWidget> MainHUDInstance;

	/** 실제 생성된 스탯 강화 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> StatUpgradeInstance;

	/** 실제 생성된 인벤토리 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> InventoryInstance;

	/** 생성된 라운드 선택 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<URoundChoiceWidget> RoundChoiceInstance;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	/** 생성된 흔함 아이템 선택 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UCommonItemChoiceWidget> CommonItemChoiceInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> WaitingWidgetInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> VictoryWidgetInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> DefeatWidgetInstance;

	UPROPERTY()
	TObjectPtr<AMyPlayerState> MyPlayerStateRef;

	/** WBP_GameOver 블루프린트 클래스 (에디터에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> GameOverWidgetClass;

	/** 생성된 게임오버 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> GameOverInstance;

	/** (수정) Input handlers for ButtonAction actions. */
	void OnButtonAction_Q();
	void OnButtonAction_W();
	void OnButtonAction_E();
	void OnButtonAction_R();

	/** (추가) Input handlers for ButtonAction actions. */
	void OnButtonAction_A();
	void OnButtonAction_S();
	void OnButtonAction_D();
	void OnButtonAction_F();

	/** (추가) 모든 버튼 액션 입력을 처리하는 공통 헬퍼 함수 */
	void HandleButtonActionInput(EButtonActionKey KeyPressed);

	void TryBindPlayerState();

	void HideMainHUD();

private:
	FVector CachedDestination;

	bool bIsTouch; // Is it a touch device
	float FollowTime; // For how long it has been pressed

	/** (추가) 클라이언트측 버튼 액션 상태 변수 */
	bool bIsButtonActionWindowActive;
	EButtonActionKey RequiredButtonActionKey;
	float ButtonActionWindowEndTime; // 타임아웃 클라이언트측 예비 체크
};