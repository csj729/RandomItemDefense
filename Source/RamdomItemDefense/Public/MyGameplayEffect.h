#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MyGameplayEffect.generated.h"

/**
 * UE5.3 이후 숨겨진 Asset Tags를 에디터에 다시 노출시키는 확장형 GameplayEffect
 */
UCLASS(Blueprintable)
class RAMDOMITEMDEFENSE_API UMyGameplayEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UMyGameplayEffect();

#if WITH_EDITORONLY_DATA
    /** 에디터에서 Asset Tags 편집 가능하도록 별도 컨테이너 제공 */
    UPROPERTY(EditAnywhere, Category = "Gameplay Effect Tags")
    FGameplayTagContainer EditableAssetTags;
#endif

    virtual void PostLoad() override;
    virtual void PostInitProperties() override;
};
