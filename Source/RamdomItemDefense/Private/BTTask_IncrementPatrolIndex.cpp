// Source/RamdomItemDefense/Private/BTTask_IncrementPatrolIndex.cpp (새 파일)

#include "BTTask_IncrementPatrolIndex.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SplineComponent.h"
#include "RamdomItemDefense.h"
#include "GameFramework/Pawn.h"

UBTTask_IncrementPatrolIndex::UBTTask_IncrementPatrolIndex()
{
	NodeName = TEXT("Increment Patrol Index");
}

EBTNodeResult::Type UBTTask_IncrementPatrolIndex::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[BTTask_Increment] ERROR! Blackboard is null."));
		return EBTNodeResult::Failed;
	}

	// 1. 블랙보드에서 경로 액터와 현재 인덱스를 가져옵니다.
	AActor* PathActor = Cast<AActor>(BlackboardComp->GetValueAsObject(PathActorKey.SelectedKeyName));
	int32 CurrentPointIndex = BlackboardComp->GetValueAsInt(PointIndexKey.SelectedKeyName);

	if (PathActor == nullptr)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[BTTask_Increment] ERROR! Value for key '%s' is null."), *PathActorKey.SelectedKeyName.ToString());
		return EBTNodeResult::Failed;
	}

	// 2. 경로 액터에서 스플라인 컴포넌트를 찾습니다.
	USplineComponent* SplineComp = PathActor->FindComponentByClass<USplineComponent>();
	if (SplineComp == nullptr)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[BTTask_Increment] ERROR! Path Actor has no Spline Component."));
		return EBTNodeResult::Failed;
	}

	// 3. 다음 이동을 위해 인덱스를 1 증가시킵니다.
	const int32 TotalPoints = SplineComp->GetNumberOfSplinePoints();
	if (TotalPoints == 0) return EBTNodeResult::Failed;

	int32 NextPointIndex = (CurrentPointIndex + 1) % TotalPoints;
	BlackboardComp->SetValueAsInt(PointIndexKey.SelectedKeyName, NextPointIndex);

	UE_LOG(LogRamdomItemDefense, Log, TEXT("[BTTask_Increment] (Pawn: %s) Arrived at Point %d. Set Index to %d."),
		*GetNameSafe(OwnerComp.GetAIOwner()->GetPawn()), CurrentPointIndex, NextPointIndex);

	return EBTNodeResult::Succeeded;
}