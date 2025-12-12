// Source/RamdomItemDefense/Public/MonsterBaseCharacter.h

#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MonsterBaseCharacter.generated.h"

// 전방 선언
class UMonsterAttributeSet;
class AMonsterSpawner;
class UAnimMontage;
class UMaterialInterface;
class UParticleSystem;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class AMonsterAIController;
class UDamageTextWidget;
struct FOnAttributeChangeData;

/** 태그별 피격 효과(파티클, 사운드)를 정의하는 구조체 */
USTRUCT(BlueprintType)
struct FHitEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UParticleSystem> HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> HitSound;
};

UCLASS()
class RAMDOMITEMDEFENSE_API AMonsterBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMonsterBaseCharacter();

	// --- [ Override Functions ] ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;

	// --- [ Public API : Spawning & Setup ] ---
	void SetSpawner(AMonsterSpawner* InSpawner);
	AMonsterSpawner* GetSpawner() const { return MySpawner; }
	void SetSpawnWaveIndex(int32 InWaveIndex) { SpawnWaveIndex = InWaveIndex; }
	int32 GetSpawnWaveIndex() const { return SpawnWaveIndex; }

	/** PVP 반격 몬스터 설정 */
	void SetIsCounterAttackMonster(bool bInValue) { bIsCounterAttackMonster = bInValue; }
	bool IsCounterAttackMonster() const { return bIsCounterAttackMonster; }

	// --- [ Public API : State ] ---
	void Die(AActor* Killer);
	bool IsDying() const { return bIsDying; }
	bool IsBoss() const { return bIsBoss; }
	int32 GetGoldOnDeath() const { return GoldOnDeath; }

	// --- [ Public API : Visuals ] ---
	void SetWaveMaterial(UMaterialInterface* WaveMaterial);
	const TArray<TObjectPtr<UMaterialInterface>>& GetWaveMaterials() const { return WaveMaterials; }

	virtual void PlayHitEffect(const FGameplayTagContainer& EffectTags);

	/** 상태 이상(슬로우 등) 이펙트 켜기/끄기 (서버->멀티캐스트) */
	void SetStatusEffectState(FGameplayTag StatusTag, bool bIsActive, UNiagaraSystem* EffectTemplate);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayMontage(UAnimMontage* MontageToPlay);

	/** (BP 설정) 몬스터가 스폰될 때 재생할 이펙트 (Smoke 등) */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals|Effects")
	TObjectPtr<UParticleSystem> SpawnEffect;

	/** (서버->모든 클라) 스폰 이펙트를 재생합니다. */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySpawnEffect();

	// --- [ Public Config ] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 100.0f;

protected:
	virtual void BeginPlay() override;

	// --- [ GAS Components ] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UMonsterAttributeSet> AttributeSet;

	// --- [ Configuration ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	bool bIsBoss;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 GoldOnDeath;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float BaseMoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	// --- [ GAS Callbacks ] ---
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);
	virtual void HandleMoveSpeedChanged(const FOnAttributeChangeData& Data);

	void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnArmorShredTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 치명타 발생 시 호출되는 함수 */
	UFUNCTION()
	void OnCritDamageOccurred(AActor* TargetActor, float CritDamageAmount);

	// --- [ Visuals : Materials ] ---
	UPROPERTY(ReplicatedUsing = OnRep_WaveMaterial)
	TObjectPtr<UMaterialInterface> CurrentWaveMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	TArray<TObjectPtr<UMaterialInterface>> WaveMaterials;

	UFUNCTION()
	void OnRep_WaveMaterial();

	// --- [ Visuals : Hit Effects ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TMap<FGameplayTag, FHitEffectData> HitEffectsMap;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitEffect(const FGameplayTag& HitTag);

	// --- [ Visuals : Status Effects ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Visuals|Status")
	TObjectPtr<UNiagaraSystem> SlowEffectTemplate;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals|Status")
	TObjectPtr<UNiagaraSystem> ArmorShredEffectTemplate;

	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UNiagaraComponent>> ActiveStatusParticles;

	// --- [ UI ] ---
	UPROPERTY(EditDefaultsOnly, Category = "Config|UI")
	TSubclassOf<UDamageTextWidget> DamageTextWidgetClass;

	// --- [ Internal State ] ---
	UPROPERTY()
	TObjectPtr<AMonsterSpawner> MySpawner;

	UPROPERTY(VisibleAnywhere, Category = "Stats")
	bool bIsDying;

	UPROPERTY(VisibleAnywhere, Category = "Stats")
	int32 SpawnWaveIndex;

	UPROPERTY()
	TWeakObjectPtr<AMonsterAIController> MonsterAIController;

	bool bIsCounterAttackMonster;

	// --- [ Blueprint Events ] ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void OnStunStateChanged(bool bIsStunned);

	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void OnSlowStateChanged(bool bIsSlowed);

	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void OnArmorShredStateChanged(bool bIsShredded);

	float CurrentBossBuffArmor = 0.0f;

	UFUNCTION()
	void OnBossStateChanged(bool bIsBossAlive);

private:
	// --- [ Ragdoll Logic ] ---
	FTimerHandle RagdollTimerHandle;
	void GoRagdoll();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_GoRagdoll();
};