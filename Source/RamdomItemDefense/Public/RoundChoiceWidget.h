#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundChoiceWidget.generated.h"

// ���� ����
class UTextBlock;
class UButton;
class AMyPlayerState;

UCLASS()
class RAMDOMITEMDEFENSE_API URoundChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** ���� ���� �� ȣ�� (Event Construct) */
	virtual void NativeConstruct() override;

	/** PlayerState ���� */
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	// --- UMG �����̳� ���� ���ε� ---
protected:
	/** "���� ���� Ƚ��: X" �ؽ�Ʈ (UMG �̸�: ChoiceCountText) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChoiceCountText;

	/** [������ �̱�] ��ư (UMG �̸�: ItemGachaButton) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ItemGachaButton;

	/** [��� ����] ��ư (UMG �̸�: GoldGambleButton) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GoldGambleButton;

	// --- C++ �̺�Ʈ �ڵ鷯 ---
protected:
	/** PlayerState�� ���� Ƚ���� ����� �� ȣ��˴ϴ�. */
	UFUNCTION()
	void HandleChoiceCountChanged(int32 NewCount);

	/** [������ �̱�] ��ư Ŭ�� �� ȣ��˴ϴ�. */
	UFUNCTION()
	void HandleItemGachaClicked();

	/** [��� ����] ��ư Ŭ�� �� ȣ��˴ϴ�. */
	UFUNCTION()
	void HandleGoldGambleClicked();
};