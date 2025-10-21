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

// Widget ���� ����
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
	/** (���� ����) ���� ��ȭâ UI�� ����մϴ�. */
	void ToggleStatUpgradeWidget();

	/** (���� ����) �κ��丮 UI�� ����մϴ�. */
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

	/** WBP_MainHUD (�������Ʈ) Ŭ������ �����Ϳ��� ������ ���� */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMainHUDWidget> MainHUDWidgetClass;

	/** WBP_StatUpgrade (�������Ʈ) Ŭ������ �����Ϳ��� ������ ���� */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> StatUpgradeWidgetClass; // �θ� UStatUpgradeWidget���� �ص� �˴ϴ�.

	/** WBP_Inventory (�������Ʈ) Ŭ������ �����Ϳ��� ������ ���� */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass; // �θ� UInventoryWidget���� �ص� �˴ϴ�.

	/** ���� ������ ���� HUD ���� �ν��Ͻ� */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UMainHUDWidget> MainHUDInstance;

	/** ���� ������ ���� ��ȭ ���� �ν��Ͻ� */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> StatUpgradeInstance;

	/** ���� ������ �κ��丮 ���� �ν��Ͻ� */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> InventoryInstance;

private:
	FVector CachedDestination;

	bool bIsTouch; // Is it a touch device
	float FollowTime; // For how long it has been pressed
};
