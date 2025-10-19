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
	// 비헤이비어 트리 에디터에서 블랙보드 키를 직접 선택할 수 있도록 변수를 추가/통일합니다.
	
	// 'PathToFollow' 키를 연결할 변수
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PathActorKey;

	// 'CurrentSplinePointIndex' 키를 연결할 변수
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PointIndexKey;

	// 목표 지점('NextMoveLocation' 등)을 저장할 키를 연결할 변수 (기존 PatrolPosKey를 그대로 사용)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PatrolPosKey;
};