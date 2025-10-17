// MonsterBaseCharacter.cpp

#include "MonsterBaseCharacter.h"
#include "MonsterAttributeSet.h"

AMonsterBaseCharacter::AMonsterBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = false; // Ư���� ��찡 �ƴϸ� ���� ƽ�� ���δ� ���� ���ɿ� �����ϴ�.

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal); // AI�� Minimal ���� ����մϴ�.

    AttributeSet = CreateDefaultSubobject<UMonsterAttributeSet>(TEXT("AttributeSet"));

    GoldOnDeath = 10; // �⺻ ��尪
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

        if (AttributeSet) // AttributeSet�� ��ȿ���� Ȯ��
        {
            AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AMonsterBaseCharacter::HandleHealthChanged);
        }
    }
}

void AMonsterBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
    // ü���� 0 ���Ϸ� �������� �� ���� ������ �����մϴ�.
    if (Data.NewValue <= 0.f)
    {
        // TODO: ��� �ִϸ��̼�, ����Ʈ ��� ���� �߰�

        // TODO: GameMode�� PlayerState�� ��� ȹ���� ��û�ϴ� ���� �߰�

        Destroy(); // ���͸� �ı��մϴ�.
    }
}