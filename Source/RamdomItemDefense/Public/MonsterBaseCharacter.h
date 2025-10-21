#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "MonsterBaseCharacter.generated.h"

class UMonsterAttributeSet;
class AMonsterSpawner;

UCLASS()
class RAMDOMITEMDEFENSE_API AMonsterBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMonsterBaseCharacter();
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// Spawner�� �ڽ��� ����ϱ� ���� ȣ���� �Լ�
	void SetSpawner(AMonsterSpawner* InSpawner);

	/** @brief ���Ͱ� �׾��� �� ������ ��差�� ��ȯ�մϴ�. */
	UFUNCTION(BlueprintPure, Category = "Stats")
	FORCEINLINE int32 GetGoldOnDeath() const { return GoldOnDeath; }

	/**
	 * @brief ������ ���� ó���� �����մϴ�. (AI ����, �ִϸ��̼� ���, ��� ���� �˸� ��)
	 * @param Killer ���͸� ���� ���� (�÷��̾� ĳ����)
	 */
	virtual void Die(AActor* Killer);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 GoldOnDeath;

	// ü�� ��ȭ�� �����Ͽ� ������ ó���� �Լ�
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	// �� ���͸� ������ �������� �ּ�
	UPROPERTY()
	TObjectPtr<AMonsterSpawner> MySpawner;
};