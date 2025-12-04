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

	void SetOwnerCharacter(ARamdomItemDefenseCharacter* InOwner);

	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

	/** (서버->모든 클라) 위치에 파티클 스폰 */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleAtLocation(UParticleSystem* EmitterTemplate, FVector Location, FRotator Rotation, FVector Scale);

	/** (서버->모든 클라) 드론 메쉬에 파티클 부착 스폰 */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleAttached(UParticleSystem* EmitterTemplate, FName SocketName, FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator, FVector Scale = FVector(1.0f));

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