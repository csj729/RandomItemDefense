// Source/RamdomItemDefense/Private/BTTask_FindPatrolPos.cpp (수정)

#include "BTTask_FindPatrolPos.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SplineComponent.h"
#include "RamdomItemDefense.h" 
#include "NavigationSystem.h"

UBTTask_FindPatrolPos::UBTTask_FindPatrolPos()
{
	NodeName = TEXT("Find Patrol Pos (Simple)"); // [ ★★★ 수정 ★★★ ]

}

EBTNodeResult::Type UBTTask_FindPatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// [ ★★★ 수정 ★★★ ]
	Super::ExecuteTask(OwnerComp, NodeMemory); // UBTTaskNode는 Super 호출 필요

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[BTTask_FindPatrolPos] ERROR! Blackboard is null."));
		return EBTNodeResult::Failed;
	}

	// 1. 블랙보드에서 경로 액터와 현재 인덱스를 가져옵니다.
	AActor* PathActor = Cast<AActor>(BlackboardComp->GetValueAsObject(PathActorKey.SelectedKeyName));
	int32 CurrentPointIndex = BlackboardComp->GetValueAsInt(PointIndexKey.SelectedKeyName);

	if (PathActor == nullptr)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[BTTask_FindPatrolPos] ERROR! Value for key '%s' is null in Blackboard."), *PathActorKey.SelectedKeyName.ToString());
		return EBTNodeResult::Failed;
	}

	// 2. 경로 액터에서 스플라인 컴포넌트를 찾습니다.
	USplineComponent* SplineComp = PathActor->FindComponentByClass<USplineComponent>();
	if (SplineComp == nullptr)
	{
		UE_LOG(LogRamdomItemDefense, Error, TEXT("[BTTask_FindPatrolPos] ERROR! Path Actor has no Spline Component."));
		return EBTNodeResult::Failed;
	}

	// 3. 현재 인덱스에 해당하는 스플라인 포인트의 월드 좌표를 가져옵니다.
	FVector TargetLocation = SplineComp->GetLocationAtSplinePoint(CurrentPointIndex, ESplineCoordinateSpace::World);

	// 4. 찾은 위치를 블랙보드의 PatrolPosKey에 저장합니다.
	BlackboardComp->SetValueAsVector(PatrolPosKey.SelectedKeyName, TargetLocation);

	UE_LOG(LogRamdomItemDefense, Log, TEXT("[BTTask_FindPatrolPos] (Pawn: %s) Set TargetLocation to Point %d"), *GetNameSafe(OwnerComp.GetAIOwner()->GetPawn()), CurrentPointIndex);

	return EBTNodeResult::Succeeded; // 위치 찾기 성공
}