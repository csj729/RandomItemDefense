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

    // ================== [코드 추가] ==================
    // 이 폰(Pawn)이 월드에 배치되거나 스폰될 때 AI 컨트롤러를 자동으로 갖도록 설정합니다.
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
    // 체력이 0 이하로 떨어졌을 때 죽음 로직을 실행합니다.
    if (Data.NewValue <= 0.f)
    {
        // 나를 생성한 스포너가 있다면, 죽음을 알립니다.
        if (MySpawner)
        {
            MySpawner->OnMonsterKilled();
        }

        // TODO: 사망 애니메이션, 이펙트 재생 로직 추가

        // TODO: GameMode나 PlayerState에 골드 획득을 요청하는 로직 추가

        Destroy();
    }
}

void AMonsterBaseCharacter::SetSpawner(AMonsterSpawner* InSpawner)
{
    MySpawner = InSpawner;
}