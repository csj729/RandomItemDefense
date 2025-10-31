// Source/RamdomItemDefense/Public/DamageTextWidget.h (����)

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
	 * (�������Ʈ ������) �ִϸ��̼� ��� ��ȣ
	 * [�߿�] .cpp ���Ͽ� �� �Լ��� ����(��ü)�� *�����* �մϴ�.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Damage Text")
	void PlayRiseAndFade();
};