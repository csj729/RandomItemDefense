#include "MonsterBaseCharacter.h"
#include "MonsterAttributeSet.h"
#include "MonsterSpawner.h"
#include "MonsterAIController.h"
#include "BrainComponent.h" 
#include "Components/CapsuleComponent.h"

AMonsterBaseCharacter::AMonsterBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    AttributeSet = CreateDefaultSubobject<UMonsterAttributeSet>(TEXT("AttributeSet"));

    GoldOnDeath = 10;
    bIsDying = false;

    // �� ��(Pawn)�� ���忡 ��ġ�ǰų� ������ �� AI ��Ʈ�ѷ��� �ڵ����� ������ �����մϴ�.
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
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
    // --- [�ڵ� ����] ---
    // �� �Լ��� ���� �ƹ��͵� ���� �ʰų�, ü�¹� UI ������Ʈ �� �ܼ��� �۾��� �����մϴ�.
    // Destroy() �� Spawner �˸� ������ ��� �����մϴ�.
    if (Data.NewValue <= 0.f)
    {
        // ���� ���� ������ ��� MonsterAttributeSet�� PostGameplayEffectExecute���� Die()�� ȣ���Ͽ� ó���մϴ�.
    }
    // ------------------
}

void AMonsterBaseCharacter::SetSpawner(AMonsterSpawner* InSpawner)
{
    MySpawner = InSpawner;
}

void AMonsterBaseCharacter::Die(AActor* Killer)
{
    if (bIsDying)
    {
        return;
    }
    bIsDying = true;

    // ���� ������ �����ʰ� �ִٸ�, ������ �˸��ϴ�.
    if (MySpawner)
    {
        MySpawner->OnMonsterKilled();
    }

    // 1. AI ����
    AMonsterAIController* AIController = Cast<AMonsterAIController>(GetController());
    if (AIController && AIController->GetBrainComponent())
    {
        AIController->GetBrainComponent()->StopLogic(TEXT("Died"));
    }

    // 2. �ݸ���(�浹)�� ��Ȱ��ȭ�մϴ�.
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    float DeathAnimLength = 0.1f; // �ִϸ��̼��� ���� ��� �⺻ �ı� �ð� (�ſ� ª��)

    // 3. ��� �ִϸ��̼� ���
    if (DeathMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("Death"));
        // ��Ÿ�ָ� ����ϰ� ���� ���̸� �޾ƿɴϴ�.
        DeathAnimLength = PlayAnimMontage(DeathMontage);
    }

    // 4. �ִϸ��̼� ���̸�ŭ LifeSpan ����
    // (���̰� 0�̰ų� ��Ÿ�ְ� ������ �⺻��(0.1f)�� ����� ��� �ı�)
    if (DeathAnimLength > 0.f)
    {
        SetLifeSpan(DeathAnimLength);
    }
    else
    {
        // ��Ÿ�ְ� ���ų� ���̰� 0�̸� ��� �ı��ǵ��� �ſ� ª�� �ð� ����
        SetLifeSpan(0.1f);
    }
}