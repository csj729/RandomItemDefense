#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RID_DamageStatics.generated.h"

class UAbilitySystemComponent;
class UMyAttributeSet;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCritDamageOccurredDelegate, AActor*, TargetActor, float, CritDamageAmount);

UCLASS()
class RAMDOMITEMDEFENSE_API URID_DamageStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// --- [ Public Static API ] ---
	/** (단일 대상) 치명타 계산/적용/텍스트 표시 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static float ApplyCritDamage(float BaseDamage, UAbilitySystemComponent* SourceASC, AActor* TargetActor, bool bIsSkillAttack);

	/** (범위 스킬용) 치명타 발생 여부 확인 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static bool CheckForCrit(UAbilitySystemComponent* SourceASC, bool bIsSkillAttack);

	/** (범위 스킬용) 치명타 배율 계산 */
	UFUNCTION(BlueprintPure, Category = "Damage")
	static float GetCritMultiplier(UAbilitySystemComponent* SourceASC);

	/** 치명타 델리게이트 (UI 표시용) */
	static FOnCritDamageOccurredDelegate OnCritDamageOccurred;

private:
	static const UMyAttributeSet* GetAttributeSetFromASC(UAbilitySystemComponent* SourceASC);
};