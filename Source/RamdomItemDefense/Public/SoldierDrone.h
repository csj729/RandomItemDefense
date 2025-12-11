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

	// --- [ IAbilitySystemInterface ] ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// --- [ Public API ] ---
	void SetOwnerCharacter(ARamdomItemDefenseCharacter* InOwner);
	FORCEINLINE UStaticMeshComponent* GetMesh() const { return Mesh; }

	// --- [ Public API : Visual Effects ] ---
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleAtLocation(UParticleSystem* EmitterTemplate, FVector Location, FRotator Rotation, FVector Scale);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleAttached(UParticleSystem* EmitterTemplate, FName SocketName, FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator, FVector Scale = FVector(1.0f));

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// --- [ Components ] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMyAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttackComponent> AttackComponent;

private:
	// --- [ Internal State ] ---
	UPROPERTY()
	TWeakObjectPtr<ARamdomItemDefenseCharacter> OwnerCharacter;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(EditDefaultsOnly, Category = "Drone|Movement")
	FVector FollowOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Drone|Movement")
	float FollowInterpSpeed;
};