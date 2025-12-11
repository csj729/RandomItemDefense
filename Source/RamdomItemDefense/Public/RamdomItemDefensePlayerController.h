#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "ItemTypes.h"
#include "RamdomItemDefensePlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
class UMainHUDWidget;
class URoundChoiceWidget;
class UCommonItemChoiceWidget;
class AMyPlayerState;
class UUserWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS()
class ARamdomItemDefensePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARamdomItemDefensePlayerController();

	// --- [ Input Actions Configuration ] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SetDestinationClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SetDestinationTouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UltimateSkillAction;

	// [Button Action Keys]
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_Q_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_W_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_E_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_R_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_A_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_S_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_D_Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input) UInputAction* ButtonAction_F_Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	float ShortPressThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UNiagaraSystem> FXCursor;

	// --- [ Public API : UI Control ] ---
	void ToggleStatUpgradeWidget();
	void ToggleInventoryWidget();

	/** °ÔÀÓ¿À¹ö UI Ç¥½Ã/¼û±è */
	void ShowGameOverUI();
	void HideGameOverUI();

	// --- [ Network API : Server RPC ] ---
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_RequestCombineItem(FName ResultItemID);

	// --- [ Network API : Client RPC ] ---
	UFUNCTION(Client, Reliable) void Client_ShowWaitingUI();
	UFUNCTION(Client, Reliable) void Client_HideWaitingUI();
	UFUNCTION(Client, Reliable) void Client_ShowVictoryUI();
	UFUNCTION(Client, Reliable) void Client_ShowDefeatUI();

	UFUNCTION(Client, Reliable)
	void Client_OnShowButtonActionUI(float TimingWindow, EButtonActionKey KeyToPress);

	UFUNCTION(Client, Unreliable)
	void Client_ShowDamageText(float DamageAmount, const FVector& Location, bool bIsCritical);

	// --- [ Public Callbacks ] ---
	void OnButtonActionResult(bool bWasSuccess, int32 RewardIndex);

protected:
	// --- [ Lifecycle & Setup ] ---
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	// --- [ Input Handlers ] ---
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();
	void OnUltimateSkillPressed();

	// [Button Action Handlers]
	void OnButtonAction_Q();
	void OnButtonAction_W();
	void OnButtonAction_E();
	void OnButtonAction_R();
	void OnButtonAction_A();
	void OnButtonAction_S();
	void OnButtonAction_D();
	void OnButtonAction_F();
	void HandleButtonActionInput(EButtonActionKey KeyPressed);

	// --- [ UI State Handlers ] ---
	UFUNCTION() void OnPlayerChoiceCountChanged(int32 NewCount);
	UFUNCTION() void OnPlayerCommonChoiceCountChanged(int32 NewCount);

	void TryBindPlayerState();
	void HideMainHUD();

	// --- [ UI Configuration (Classes) ] ---
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UUserWidget> StatUpgradeWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UUserWidget> InventoryWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<URoundChoiceWidget> RoundChoiceWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UCommonItemChoiceWidget> CommonItemChoiceWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UUserWidget> GameOverWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI") TSubclassOf<UUserWidget> DamageTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|PVP") TSubclassOf<UUserWidget> WaitingWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI|PVP") TSubclassOf<UUserWidget> VictoryWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI|PVP") TSubclassOf<UUserWidget> DefeatWidgetClass;

	// --- [ UI Instances ] ---
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI") TObjectPtr<UMainHUDWidget> MainHUDInstance;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI") TObjectPtr<UUserWidget> StatUpgradeInstance;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI") TObjectPtr<UUserWidget> InventoryInstance;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI") TObjectPtr<URoundChoiceWidget> RoundChoiceInstance;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI") TObjectPtr<UCommonItemChoiceWidget> CommonItemChoiceInstance;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI") TObjectPtr<UUserWidget> GameOverInstance;

	UPROPERTY() TObjectPtr<UUserWidget> WaitingWidgetInstance;
	UPROPERTY() TObjectPtr<UUserWidget> VictoryWidgetInstance;
	UPROPERTY() TObjectPtr<UUserWidget> DefeatWidgetInstance;

	UPROPERTY()
	TObjectPtr<AMyPlayerState> MyPlayerStateRef;

	uint32 bMoveToMouseCursor : 1;

private:
	// --- [ Internal State ] ---
	FVector CachedDestination;
	bool bIsTouch;
	float FollowTime;

	// [Button Action State]
	bool bIsButtonActionWindowActive;
	EButtonActionKey RequiredButtonActionKey;
	float ButtonActionWindowEndTime;
};