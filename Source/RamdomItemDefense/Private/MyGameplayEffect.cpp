#include "MyGameplayEffect.h"
#include "GameplayTagsManager.h"

UMyGameplayEffect::UMyGameplayEffect()
{
#if WITH_EDITORONLY_DATA
    // �⺻ ���� �±� (���Ѵٸ� �ʱⰪ)
    // EditableAssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Effect.Default")));
#endif
}

void UMyGameplayEffect::PostLoad()
{
    Super::PostLoad();

#if WITH_EDITORONLY_DATA
    // �����Ϳ��� ������ �±׸� ���� AssetTags�� �ݿ�
    InheritableGameplayEffectTags.CombinedTags.Reset();
    InheritableGameplayEffectTags.CombinedTags.AppendTags(EditableAssetTags);
#endif
}

void UMyGameplayEffect::PostInitProperties()
{
    Super::PostInitProperties();

#if WITH_EDITORONLY_DATA
    // C++ ���� �ÿ��� ������ �±� �ݿ�
    InheritableGameplayEffectTags.CombinedTags.Reset();
    InheritableGameplayEffectTags.CombinedTags.AppendTags(EditableAssetTags);
#endif
}
