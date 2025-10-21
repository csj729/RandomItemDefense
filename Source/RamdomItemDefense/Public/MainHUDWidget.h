#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainHUDWidget.generated.h"

class AMyPlayerState;
class ARamdomItemDefensePlayerController;

UCLASS()
class RAMDOMITEMDEFENSE_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// ������ ó�� ������ �� ȣ��˴ϴ�.
	virtual bool Initialize() override;

	// PlayerState�� ��������Ʈ�� �̺�Ʈ�� ���ε�(����)�մϴ�.
	void BindPlayerStateEvents();

	void BindSpawnerEvents();

	// PlayerState ����
	UPROPERTY(BlueprintReadOnly, Category = "PlayerState")
	TObjectPtr<AMyPlayerState> MyPlayerState;

	// PlayerController ����
	UPROPERTY(BlueprintReadOnly, Category = "Controller")
	TObjectPtr<ARamdomItemDefensePlayerController> MyPlayerController;

	// --- �������Ʈ(UMG)���� ������ �̺�Ʈ ---

		/** (WBP_MainHUD���� ����) ��� ���� ����� �� ȣ��˴ϴ�. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnGoldChanged(int32 NewGold);

	/** (WBP_MainHUD���� ����) ���� ���� ����� �� ȣ��˴ϴ�. */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnMonsterCountChanged(int32 NewCount, int32 MaxCount);

	// --- �������Ʈ(UMG)�� ��ư�� Ŭ���� �Լ� ---

	/** (WBP_MainHUD�� '���� ��ȭ' ��ư�� ����) */
	UFUNCTION(BlueprintCallable, Category = "UI Actions")
	void HandleStatUpgradeClicked();

	/** (WBP_MainHUD�� '�κ��丮' ��ư�� ����) */
	UFUNCTION(BlueprintCallable, Category = "UI Actions")
	void HandleInventoryClicked();

	/** PlayerState�� �����ʰ� �Ҵ�/�����Ǿ��� �� C++���� ���� */
	UFUNCTION()
	void HandleSpawnerAssigned(int32 Dummy); // PlayerState�� OnSpawnerAssignedDelegate�� ����

	/** �������� ���� ���� ����� �� C++���� ���� */
	UFUNCTION()
	void HandleMonsterCountChanged(int32 NewCount);
};