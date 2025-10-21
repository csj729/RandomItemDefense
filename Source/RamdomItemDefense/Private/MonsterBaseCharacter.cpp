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

    // 이 폰(Pawn)이 월드에 배치되거나 스폰될 때 AI 컨트롤러를 자동으로 갖도록 설정합니다.
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
    // --- [코드 수정] ---
    // 이 함수는 이제 아무것도 하지 않거나, 체력바 UI 업데이트 등 단순한 작업만 수행합니다.
    // Destroy() 및 Spawner 알림 로직을 모두 제거합니다.
    if (Data.NewValue <= 0.f)
    {
        // 죽음 관련 로직은 모두 MonsterAttributeSet의 PostGameplayEffectExecute에서 Die()를 호출하여 처리합니다.
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

    // 나를 생성한 스포너가 있다면, 죽음을 알립니다.
    if (MySpawner)
    {
        MySpawner->OnMonsterKilled();
    }

    // TODO: 사망 애니메이션, 이펙트 재생 로직 추가

    // 콜리전(충돌)을 비활성화합니다.
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 사망 애니메이션 등이 재생될 시간을 기다린 후 액터를 파괴합니다.
    // (지금은 2초로 설정, 애니메이션 길이에 맞게 조절)
    SetLifeSpan(2.0f);
}