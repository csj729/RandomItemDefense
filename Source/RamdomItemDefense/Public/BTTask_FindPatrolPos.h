#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolPos.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UBTTask_FindPatrolPos : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPos();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	// �����̺�� Ʈ�� �����Ϳ��� ������ Ű�� ���� ������ �� �ֵ��� ������ �߰�/�����մϴ�.
	
	// 'PathToFollow' Ű�� ������ ����
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PathActorKey;

	// 'CurrentSplinePointIndex' Ű�� ������ ����
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PointIndexKey;

	// ��ǥ ����('NextMoveLocation' ��)�� ������ Ű�� ������ ���� (���� PatrolPosKey�� �״�� ���)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PatrolPosKey;
};