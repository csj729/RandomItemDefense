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


/** @brief 태그별 피격 효과(파티클, 사운드)를 정의하는 구조체 */
USTRUCT(BlueprintType)
struct FHitEffectData
{
	GENERATED_BODY()

	/** 피격 시 스폰할 파티클 이펙트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UParticleSystem> HitEffect;

	/** 피격 시 재생할 사운드 */
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

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	/** 이 몬스터가 보스 몬스터인지 여부를 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsBoss() const { return bIsBoss; }
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

	void SetWaveMaterial(UMaterialInterface* WaveMaterial);
	const TArray<TObjectPtr<UMaterialInterface>>& GetWaveMaterials() const { return WaveMaterials; }
	virtual void PlayHitEffect(const FGameplayTagContainer& EffectTags);


	/** (AIController가 폰에 빙의될 때 호출됨) AI 관련 초기화 수행 */
	virtual void PossessedBy(AController* NewController) override;


protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<class UMonsterAttributeSet> AttributeSet;

	// --- [ ★★★ 코드 추가 ★★★ ] ---
	/** (블루프린트 디폴트에서 설정) 이 몬스터가 보스 몬스터로 취급되는지 여부 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	bool bIsBoss;
	// --- [ ★★★ 코드 추가 끝 ★★★ ] ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 GoldOnDeath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** Health 속성 변경 콜백 (BeginPlay -> PossessedBy로 이동 예정) */
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	/** MoveSpeed 속성 변경 콜백 */
	virtual void HandleMoveSpeedChanged(const FOnAttributeChangeData& Data);

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
	/** State.Stun 태그가 변경될 때 호출될 콜백 함수 */
	void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** State.Slow 태그가 변경될 때 호출될 콜백 함수 */
	void OnSlowTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 이 몬스터를 제어하는 AI 컨트롤러 캐시 */
	UPROPERTY()
	TWeakObjectPtr<AMonsterAIController> MonsterAIController;

	/** (블루프린트 구현용) 스턴 상태가 변경될 때 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void OnStunStateChanged(bool bIsStunned);

	/** (블루프린트 구현용) 슬로우 상태가 변경될 때 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	void OnSlowStateChanged(bool bIsSlowed);

	/**
	 * @brief (블루프린트에서 설정) 몬스터 머리 위에 띄울 데미지 텍스트 위젯
	 * (WBP_DamageText로 설정, 부모는 UDamageTextWidget이어야 함)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Config|UI")
	TSubclassOf<UDamageTextWidget> DamageTextWidgetClass;

	/**
	 * @brief URID_DamageStatics의 치명타 델리게이트에 바인딩될 함수
	 */
	UFUNCTION()
	void OnCritDamageOccurred(AActor* TargetActor, float CritDamageAmount);
};