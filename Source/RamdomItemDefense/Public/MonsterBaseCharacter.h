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

	/** @brief 몬스터가 죽었을 때 지급할 골드량을 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Stats")
	FORCEINLINE int32 GetGoldOnDeath() const { return GoldOnDeath; }

	/**
	 * @brief 몬스터의 죽음 처리를 시작합니다. (AI 정지, 애니메이션 재생, 골드 지급 알림 등)
	 * @param Killer 몬스터를 죽인 액터 (플레이어 캐릭터)
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

	// 체력 변화를 감지하여 죽음을 처리할 함수
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	// 이 몬스터를 스폰한 스포너의 주소
	UPROPERTY()
	TObjectPtr<AMonsterSpawner> MySpawner;
};