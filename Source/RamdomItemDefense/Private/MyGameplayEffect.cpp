#include "MyGameplayEffect.h"
#include "GameplayTagsManager.h"

UMyGameplayEffect::UMyGameplayEffect()
{
#if WITH_EDITORONLY_DATA
    // 기본 예시 태그 (원한다면 초기값)
    // EditableAssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Effect.Default")));
#endif
}

void UMyGameplayEffect::PostLoad()
{
    Super::PostLoad();

#if WITH_EDITORONLY_DATA
    // 에디터에서 편집한 태그를 실제 AssetTags에 반영
    InheritableGameplayEffectTags.CombinedTags.Reset();
    InheritableGameplayEffectTags.CombinedTags.AppendTags(EditableAssetTags);
#endif
}

void UMyGameplayEffect::PostInitProperties()
{
    Super::PostInitProperties();

#if WITH_EDITORONLY_DATA
    // C++ 생성 시에도 에디터 태그 반영
    InheritableGameplayEffectTags.CombinedTags.Reset();
    InheritableGameplayEffectTags.CombinedTags.AppendTags(EditableAssetTags);
#endif
}
