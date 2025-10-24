#include "BTTask_FindPatrolPos.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SplineComponent.h"
#include "RamdomItemDefense.h" // GEngine ��ü
#include "NavigationSystem.h"

UBTTask_FindPatrolPos::UBTTask_FindPatrolPos()
{
	NodeName = TEXT("Find Patrol Pos (Spline)"); // �����Ϳ��� �˾ƺ��� ���� �̸� ����
}

EBTNodeResult::Type UBTTask_FindPatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("BTTask: ERROR! Blackboard is null."));
		// -----------------------------------------
		return EBTNodeResult::Failed;
	}

	// ================== [�ڵ� ����] ==================
	// ���� �ϵ��ڵ��� �̸� ���, �����Ϳ��� ������ KeySelector�� �̸��� ����� ���� �����ɴϴ�.

	// 1. �����忡�� ��� ���Ϳ� ���� �ε����� �����ɴϴ�.
	AActor* PathActor = Cast<AActor>(BlackboardComp->GetValueAsObject(PathActorKey.SelectedKeyName));
	int32 CurrentPointIndex = BlackboardComp->GetValueAsInt(PointIndexKey.SelectedKeyName);

	if (PathActor == nullptr)
	{
		// �� ���� �޽����� �ٷ� ���� �ް� ��� �����Դϴ�.
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("BTTask: ERROR! Value for key '%s' is null in Blackboard."), *PathActorKey.SelectedKeyName.ToString());
		// -----------------------------------------
		return EBTNodeResult::Failed;
	}

	// 2. ��� ���Ϳ��� ���ö��� ������Ʈ�� ã���ϴ�.
	USplineComponent* SplineComp = PathActor->FindComponentByClass<USplineComponent>();
	if (SplineComp == nullptr)
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("BTTask: ERROR! Path Actor has no Spline Component."));
		// -----------------------------------------
		return EBTNodeResult::Failed;
	}

	// 3. ���� �ε����� �ش��ϴ� ���ö��� ����Ʈ�� ���� ��ǥ�� �����ɴϴ�.
	FVector TargetLocation = SplineComp->GetLocationAtSplinePoint(CurrentPointIndex, ESplineCoordinateSpace::World);

	// 4. ã�� ��ġ�� �������� PatrolPosKey�� �����մϴ�.
	BlackboardComp->SetValueAsVector(PatrolPosKey.SelectedKeyName, TargetLocation);

	// 5. ���� �̵��� ���� �ε����� 1 ������ŵ�ϴ�.
	int32 NextPointIndex = (CurrentPointIndex + 1) % SplineComp->GetNumberOfSplinePoints();
	BlackboardComp->SetValueAsInt(PointIndexKey.SelectedKeyName, NextPointIndex);
	// ===============================================

	return EBTNodeResult::Succeeded;
}