// Source/RamdomItemDefense/Public/DamageTextWidget.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageTextWidget.generated.h"

class UTextBlock;

UCLASS()
class RAMDOMITEMDEFENSE_API UDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;

public:
	UFUNCTION(BlueprintCallable, Category = "Damage Text")
	void SetDamageText(const FText& InText);

	/**
	 * (블루프린트 구현용) 애니메이션 재생 신호
	 * [중요] .cpp 파일에 이 함수의 구현(본체)이 *없어야* 합니다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage Text")
	void PlayRiseAndFade();
};