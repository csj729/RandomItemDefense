#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageTextWidget.generated.h"

class UTextBlock;

UCLASS()
class RAMDOMITEMDEFENSE_API UDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- [ Public API ] ---
	UFUNCTION(BlueprintCallable, Category = "Damage Text")
	void SetDamageText(const FText& InText);

	UFUNCTION(BlueprintImplementableEvent, Category = "Damage Text")
	void PlayRiseAndFade();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;
};