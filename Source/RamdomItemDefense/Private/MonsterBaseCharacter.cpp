// MonsterBaseCharacter.cpp

#include "MonsterBaseCharacter.h"
#include "MonsterAttributeSet.h"

AMonsterBaseCharacter::AMonsterBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = false; // 특별한 경우가 아니면 몬스터 틱은 꺼두는 것이 성능에 좋습니다.

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal); // AI는 Minimal 모드로 충분합니다.

    AttributeSet = CreateDefaultSubobject<UMonsterAttributeSet>(TEXT("AttributeSet"));

    GoldOnDeath = 10; // 기본 골드값
}

UAbilitySystemComponent* AMonsterBaseCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AMonsterBaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);

        if (AttributeSet) // AttributeSet이 유효한지 확인
        {
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
        }
    }
}

void AMonsterBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    // 체력이 0 이하로 떨어졌을 때 죽음 로직을 실행합니다.
    if (Data.NewValue <= 0.f)
    {
        // TODO: 사망 애니메이션, 이펙트 재생 로직 추가

        // TODO: GameMode나 PlayerState에 골드 획득을 요청하는 로직 추가

        Destroy(); // 액터를 파괴합니다.
    }
}