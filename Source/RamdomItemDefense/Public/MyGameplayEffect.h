#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MyGameplayEffect.generated.h"

/**
 * UE5.3 ���� ������ Asset Tags�� �����Ϳ� �ٽ� �����Ű�� Ȯ���� GameplayEffect
 */
UCLASS(Blueprintable)
class RAMDOMITEMDEFENSE_API UMyGameplayEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UMyGameplayEffect();

#if WITH_EDITORONLY_DATA
    /** �����Ϳ��� Asset Tags ���� �����ϵ��� ���� �����̳� ���� */
    UPROPERTY(EditAnywhere, Category = "Gameplay Effect Tags")
    FGameplayTagContainer EditableAssetTags;
#endif

    virtual void PostLoad() override;
    virtual void PostInitProperties() override;
};
