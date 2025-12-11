#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "MyAttributeSet.h"
#include "RamdomItemDefenseCharacter.generated.h"

// 전방 선언
class UAttackComponent;
class UInventoryComponent;
class UAnimMontage;
class UMyAttributeSet;
class USoundBase;

UCLASS(Blueprintable)
class RAMDOMITEMDEFENSE_API ARamdomItemDefenseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ARamdomItemDefenseCharacter();

	// --- [ Override Functions ] ---
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- [ IAbilitySystemInterface ] ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	FORCEINLINE const UMyAttributeSet* GetAttributeSet() const { return AttributeSet; }

	// --- [ Public Getters ] ---
	FORCEINLINE UAttackComponent* GetAttackComponent() const { return AttackComponent; }
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	int32 GetMaxUltimateCharge() const { return MaxUltimateCharge; }

	// --- [ Public API : Combat & Stats ] ---
	/** (서버 전용) PlayerState로부터 스탯 강화 적용 요청을 받습니다. */
	void ApplyStatUpgrade(EItemStatType StatType, int32 NewLevel);

	/** 설정된 공격 몽타주 배열에서 랜덤하게 하나를 골라 반환합니다. */
	UAnimMontage* GetRandomAttackMontage() const;

	// --- [ Public API : Visual Effects (NetMulticast) ] ---
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAttack(UAnimMontage* MontageToPlay, FRotator TargetRotation);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleAtLocation(UParticleSystem* EmitterTemplate, FVector Location, FRotator Rotation, FVector Scale);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SpawnParticleAttached(UParticleSystem* EmitterTemplate, FName SocketName, FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator, FVector Scale = FVector(1.0f));

	/** (서버->모든 클라) 특정 태그와 연결된 지속 이펙트를 생성하고 부착합니다. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_AddBuffEffect(FGameplayTag BuffTag, UParticleSystem* EmitterTemplate, FName SocketName, FVector LocationOffset = FVector::ZeroVector, FVector Scale = FVector(1.0f));

	/** (서버->모든 클라) 특정 태그와 연결된 지속 이펙트를 제거합니다. */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RemoveBuffEffect(FGameplayTag BuffTag);

	// --- [ Public Configuration : Sound ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> UltimateReadySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> UltimateActivateSound;

protected:
	virtual void BeginPlay() override;

	// --- [ Components ] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttackComponent> AttackComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// --- [ GAS & Attributes ] ---
	UPROPERTY()
	TObjectPtr<UMyAttributeSet> AttributeSet;

	/** 캐릭터 기본 스탯 적용을 위한 GE */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultStatsEffect;

	/** 캐릭터가 기본적으로 보유할 어빌리티 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;

	/** 기본 스탯 GE 적용 함수 */
	void ApplyDefaultStats();

	// --- [ Animation ] ---
	/** 재생할 기본 공격 몽타주 배열 */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TArray<TObjectPtr<UAnimMontage>> DefaultAttackMontages;

	// --- [ Ultimate Config ] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ultimate")
	int32 MaxUltimateCharge = 100;

	// --- [ Internal State ] ---
	/** 현재 활성화된 버프 파티클 관리 (태그 -> 컴포넌트) */
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UParticleSystemComponent>> ActiveBuffParticles;

	/** 수동 타겟 (Replicated) */
	UPROPERTY(Replicated)
	TObjectPtr<AActor> ManualTarget;

private:
	// --- [ Camera Components ] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
};