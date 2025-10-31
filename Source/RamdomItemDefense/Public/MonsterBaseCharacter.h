#pragma once

#include "RamdomItemDefense.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MonsterBaseCharacter.generated.h"

class UMonsterAttributeSet;
class AMonsterSpawner;
class UAnimMontage;
class UMaterialInterface;
class UParticleSystem;
class USoundBase;
class AMonsterAIController;
struct FOnAttributeChangeData;
class UDamageTextWidget;


/** @brief �±׺� �ǰ� ȿ��(��ƼŬ, ����)�� �����ϴ� ����ü */
USTRUCT(BlueprintType)
struct FHitEffectData
{
	GENERATED_BODY()

	/** �ǰ� �� ������ ��ƼŬ ����Ʈ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UParticleSystem> HitEffect;

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


	/** (AIController�� ���� ���ǵ� �� ȣ���) AI ���� �ʱ�ȭ ���� */
	virtual void PossessedBy(AController* NewController) override;


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

	// --- [ �ڡڡ� �ڵ� ���� �ڡڡ� ] ---
	/** Health �Ӽ� ���� �ݹ� (BeginPlay -> PossessedBy�� �̵� ����) */
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	/** MoveSpeed �Ӽ� ���� �ݹ� */
	virtual void HandleMoveSpeedChanged(const FOnAttributeChangeData& Data);
	// --- [ �ڵ� ���� �� ] ---

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float BaseMoveSpeed;

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

protected:
	/** State.Stun �±װ� ����� �� ȣ��� �ݹ� �Լ� */
	void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** State.Slow �±װ� ����� �� ȣ��� �ݹ� �Լ� */
	void OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** �� ���͸� �����ϴ� AI ��Ʈ�ѷ� ĳ�� */
	UPROPERTY()
	TWeakObjectPtr<AMonsterAIController> MonsterAIController;

	/** (�������Ʈ ������) ���� ���°� ����� �� ȣ��˴ϴ�. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void OnStunStateChanged(bool bIsStunned);

	/** (�������Ʈ ������) ���ο� ���°� ����� �� ȣ��˴ϴ�. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void OnSlowStateChanged(bool bIsSlowed);

	/**
	 * @brief (�������Ʈ���� ����) ���� �Ӹ� ���� ��� ������ �ؽ�Ʈ ����
	 * (WBP_DamageText�� ����, �θ�� UDamageTextWidget�̾�� ��)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|UI")
	TSubclassOf<UDamageTextWidget> DamageTextWidgetClass;

	/**
	 * @brief URID_DamageStatics�� ġ��Ÿ ��������Ʈ�� ���ε��� �Լ�
	 */
	UFUNCTION()
	void OnCritDamageOccurred(AActor* TargetActor, float CritDamageAmount);
};