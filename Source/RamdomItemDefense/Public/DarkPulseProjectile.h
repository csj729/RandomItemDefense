// Source/RamdomItemDefense/Public/DarkPulseProjectile.h

#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DarkPulseProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystemComponent;
class UParticleSystem;


UCLASS()
class RAMDOMITEMDEFENSE_API ADarkPulseProjectile : public AActor
{
	GENERATED_BODY()

public:
	ADarkPulseProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> TrailEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ArrivalTolerance;

};