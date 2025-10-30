#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "MonsterBaseCharacter.generated.h"

class UMonsterAttributeSet;
class AMonsterSpawner;
class UAnimMontage;
class UMaterialInterface;
// --- [�ڵ� ����] ---
class UParticleSystem; // Niagara -> Particle System
// --- [�ڵ� ���� ��] ---
class USoundBase;

/** @brief �±׺� �ǰ� ȿ��(��ƼŬ, ����)�� �����ϴ� ����ü */
USTRUCT(BlueprintType)
struct FHitEffectData
{
	GENERATED_BODY()

	// --- [�ڵ� ����] ---
	/** �ǰ� �� ������ ��ƼŬ ����Ʈ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UParticleSystem> HitEffect; // Niagara -> Particle System
	// --- [�ڵ� ���� ��] ---

	/** �ǰ� �� ����� ���� */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> HitSound;
};


UCLASS()
class RAMDOMITEMDEFENSE_API AMonsterBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMonsterBaseCharacter();
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetSpawner(AMonsterSpawner* InSpawner);

	UFUNCTION(BlueprintPure, Category = "Stats")
	FORCEINLINE int32 GetGoldOnDeath() const { return GoldOnDeath; }

	virtual void Die(AActor* Killer);

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsDying() const { return bIsDying; }

	void SetWaveMaterial(UMaterialInterface* WaveMaterial);
	const TArray<TObjectPtr<UMaterialInterface>>& GetWaveMaterials() const { return WaveMaterials; }
	virtual void PlayHitEffect(const FGameplayTagContainer& EffectTags);


protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 GoldOnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	TObjectPtr<AMonsterSpawner> MySpawner;

	UPROPERTY(VisibleAnywhere, Category = "Stats")
	bool bIsDying;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_WaveMaterial)
	TObjectPtr<UMaterialInterface> CurrentWaveMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	TArray<TObjectPtr<UMaterialInterface>> WaveMaterials;

	UFUNCTION()
	void OnRep_WaveMaterial();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TMap<FGameplayTag, FHitEffectData> HitEffectsMap;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitEffect(const FGameplayTag& HitTag);
};