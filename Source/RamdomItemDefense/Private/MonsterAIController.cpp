#include "MonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" // GEngine 대체

// 생성자는 이전과 동일
AMonsterAIController::AMonsterAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

// OnPossess 함수를 아래와 같이 수정합니다.
void AMonsterAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (BehaviorTree)
    {
        BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);

        // 이제 경로는 SetSpawner 시점에 주입되므로 여기서는 BT만 실행합니다.
        RunBehaviorTree(BehaviorTree);
    }
    else
    {
        RID_LOG(FColor::Red, TEXT("AIController: ERROR! BehaviorTree is NOT assigned."));
    }
}

void AMonsterAIController::SetPatrolPath(AActor* PathActor)
{
    if (BlackboardComponent && PathActor)
    {
        // 블랙보드에 경로 액터 입력
        BlackboardComponent->SetValueAsObject(TEXT("PathToFollow"), PathActor);
        // 시작 인덱스 초기화
        BlackboardComponent->SetValueAsInt(TEXT("CurrentSplinePointIndex"), 0);

        // RID_LOG(FColor::Green, TEXT("AIController: Path set to '%s'"), *PathActor->GetName());
    }
}