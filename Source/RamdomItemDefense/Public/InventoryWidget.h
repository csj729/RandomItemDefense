#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.generated.h"

class UInventoryComponent;

UCLASS()
class RAMDOMITEMDEFENSE_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnInventoryUpdated();

protected:
	// --- [ Lifecycle ] ---
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void BindDataSources();

	// --- [ Data Sources ] ---
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	TObjectPtr<UInventoryComponent> InventoryComp;

	// --- [ Handlers ] ---
	UFUNCTION()
	void HandleInventoryUpdated();
};