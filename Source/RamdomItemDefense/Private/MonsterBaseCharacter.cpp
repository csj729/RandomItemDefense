#include "MonsterBaseCharacter.h"
#include "MonsterAttributeSet.h"
#include "MonsterSpawner.h"

AMonsterBaseCharacter::AMonsterBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    AttributeSet = CreateDefaultSubobject<UMonsterAttributeSet>(TEXT("AttributeSet"));

    GoldOnDeath = 10;

    // ================== [�ڵ� �߰�] ==================
    // �� ��(Pawn)�� ���忡 ��ġ�ǰų� ������ �� AI ��Ʈ�ѷ��� �ڵ����� ������ �����մϴ�.
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    // ===============================================
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

        if (AttributeSet)
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
        // ���� ������ �����ʰ� �ִٸ�, ������ �˸��ϴ�.
        if (MySpawner)
        {
            MySpawner->OnMonsterKilled();
        }

        // TODO: ��� �ִϸ��̼�, ����Ʈ ��� ���� �߰�

        // TODO: GameMode�� PlayerState�� ��� ȹ���� ��û�ϴ� ���� �߰�

        Destroy();
    }
}

void AMonsterBaseCharacter::SetSpawner(AMonsterSpawner* InSpawner)
{
    MySpawner = InSpawner;
}