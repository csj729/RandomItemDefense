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

	// Spawner가 자신을 등록하기 위해 호출할 함수
	void SetSpawner(AMonsterSpawner* InSpawner);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 GoldOnDeath;

	// 체력 변화를 감지하여 죽음을 처리할 함수
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	// 이 몬스터를 스폰한 스포너의 주소
	UPROPERTY()
	TObjectPtr<AMonsterSpawner> MySpawner;
};