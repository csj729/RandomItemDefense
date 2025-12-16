#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "CharacterSelectPlayerController.generated.h"

class ASelectableCharacter;
class ACameraActor;
class UInputMappingContext;
class UInputAction;
class UUserWidget;

/**
 * 캐릭터 선택 레벨(로비) 전용 PC.
 * 마우스 입력 및 선택 UI 위젯 관리.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API ACharacterSelectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACharacterSelectPlayerController();

	// --- [ Public API : Ready State ] ---
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SetPlayerReady(bool bReady);

	UFUNCTION(BlueprintPure, Category = "Game")
	bool GetIsPlayerReady() const { return bIsPlayerReady; }

	// --- [ Public API : View Control ] ---
	void SetTargetCharacter(ASelectableCharacter* NewTarget);
	void ResetView();

	// --- [ UI Config ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> CharacterSelectWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> CharacterSelectWidgetInstance;

	// --- [ Input Config ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> CharacterSelectMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> BackToMenuAction;

protected:
	// --- [ Lifecycle ] ---
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// --- [ Blueprint Implementable Events ] ---
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void ToggleCharacterInfo(bool bVisible);

	// --- [ Camera & Target ] ---
	UPROPERTY()
	TObjectPtr<ACameraActor> MainCamera;

	UPROPERTY()
	TObjectPtr<ASelectableCharacter> CurrentTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraInterpSpeed = 5.0f;

	FVector InitialCameraLocation;
	FRotator InitialCameraRotation;

	// --- [ Internal Logic ] ---
	void OnBackToMainMenu(const FInputActionValue& Value);

	bool bIsPlayerReady = false;
};