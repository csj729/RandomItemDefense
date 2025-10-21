// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "RamdomItemDefensePlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;

// Widget 전방 선언
class UMainHUDWidget;
class UStatUpgradeWidget;
class UInventoryWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS()
class ARamdomItemDefensePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARamdomItemDefensePlayerController();

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

public:
	/** (추후 구현) 스탯 강화창 UI를 토글합니다. */
	void ToggleStatUpgradeWidget();

	/** (추후 구현) 인벤토리 UI를 토글합니다. */
	void ToggleInventoryWidget();

protected:
	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	virtual void SetupInputComponent() override;

	// To add mapping context
	virtual void BeginPlay();

	/** Input handlers for SetDestination action. */
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();

	/** WBP_MainHUD (블루프린트) 클래스를 에디터에서 지정할 변수 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;

	/** WBP_StatUpgrade (블루프린트) 클래스를 에디터에서 지정할 변수 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> StatUpgradeWidgetClass; // 부모를 UStatUpgradeWidget으로 해도 됩니다.

	/** WBP_Inventory (블루프린트) 클래스를 에디터에서 지정할 변수 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass; // 부모를 UInventoryWidget으로 해도 됩니다.

	/** 실제 생성된 메인 HUD 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UMainHUDWidget> MainHUDInstance;

	/** 실제 생성된 스탯 강화 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> StatUpgradeInstance;

	/** 실제 생성된 인벤토리 위젯 인스턴스 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> InventoryInstance;

private:
	FVector CachedDestination;

	bool bIsTouch; // Is it a touch device
	float FollowTime; // For how long it has been pressed
};
