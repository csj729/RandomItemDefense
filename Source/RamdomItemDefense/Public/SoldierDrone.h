#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "SoldierDrone.generated.h"

class UStaticMeshComponent;
class UAbilitySystemComponent;
class UMyAttributeSet;
class UAttackComponent;
class ARamdomItemDefenseCharacter;

UCLASS()
class RAMDOMITEMDEFENSE_API ASoldierDrone : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASoldierDrone();

	// IAbilitySystemInterface 구현
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** GA_Soldier_DronePassive가 스폰 직후 호출할 함수 */
	void SetOwnerCharacter(ARamdomItemDefenseCharacter* InOwner);

	// [ ★★★ 코드 추가 ★★★ ]
	/**
	 * @brief 이 드론의 스태틱 메쉬 컴포넌트를 반환합니다. (GA에서 이펙트 부착용)
	 */
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }
	// [ ★★★ 코드 추가 끝 ★★★ ]

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override; // Tick에서 이동을 처리합니다.

	/** 드론의 외형 (Static Mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 드론의 GAS 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** 드론의 스탯 세트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMyAttributeSet> AttributeSet;

	/** 드론의 공격 로직 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttackComponent> AttackComponent;

private:
	/** 따라다닐 대상 (소유자) */
	UPROPERTY()
	TWeakObjectPtr<ARamdomItemDefenseCharacter> OwnerCharacter;

	/** 루트 씬 컴포넌트 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RootScene;

	/** 따라다닐 위치 오프셋 (캐릭터 기준) */
	UPROPERTY(EditDefaultsOnly, Category = "Drone")
	FVector FollowOffset;

	/** 따라다니는 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "Drone")
	float FollowInterpSpeed;
};