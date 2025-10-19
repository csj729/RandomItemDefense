#include "BTTask_FindPatrolPos.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "NavigationSystem.h"

UBTTask_FindPatrolPos::UBTTask_FindPatrolPos()
{
	NodeName = TEXT("Find Patrol Pos (Spline)"); // 에디터에서 알아보기 쉽게 이름 변경
}

EBTNodeResult::Type UBTTask_FindPatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BTTask: ERROR! Blackboard is null."));
		return EBTNodeResult::Failed;
	}

	// ================== [코드 수정] ==================
	// 이제 하드코딩된 이름 대신, 에디터에서 선택한 KeySelector의 이름을 사용해 값을 가져옵니다.

	// 1. 블랙보드에서 경로 액터와 현재 인덱스를 가져옵니다.
	AActor* PathActor = Cast<AActor>(BlackboardComp->GetValueAsObject(PathActorKey.SelectedKeyName));
	int32 CurrentPointIndex = BlackboardComp->GetValueAsInt(PointIndexKey.SelectedKeyName);
	
	if (PathActor == nullptr)
	{
		// 이 에러 메시지가 바로 지금 겪고 계신 문제입니다.
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("BTTask: ERROR! Value for key '%s' is null in Blackboard."), *PathActorKey.SelectedKeyName.ToString()));
		return EBTNodeResult::Failed;
	}

	// 2. 경로 액터에서 스플라인 컴포넌트를 찾습니다.
	USplineComponent* SplineComp = PathActor->FindComponentByClass<USplineComponent>();
	if (SplineComp == nullptr)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BTTask: ERROR! Path Actor has no Spline Component."));
		return EBTNodeResult::Failed;
	}

	// 3. 현재 인덱스에 해당하는 스플라인 포인트의 월드 좌표를 가져옵니다.
	FVector TargetLocation = SplineComp->GetLocationAtSplinePoint(CurrentPointIndex, ESplineCoordinateSpace::World);

	// 4. 찾은 위치를 블랙보드의 PatrolPosKey에 저장합니다.
	BlackboardComp->SetValueAsVector(PatrolPosKey.SelectedKeyName, TargetLocation);

	// 5. 다음 이동을 위해 인덱스를 1 증가시킵니다.
	int32 NextPointIndex = (CurrentPointIndex + 1) % SplineComp->GetNumberOfSplinePoints();
	BlackboardComp->SetValueAsInt(PointIndexKey.SelectedKeyName, NextPointIndex);
	// ===============================================

	return EBTNodeResult::Succeeded;
}