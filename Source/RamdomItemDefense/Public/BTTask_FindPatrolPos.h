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

	// --- [ Configuration : Blackboard Keys ] ---
	/** 'PathToFollow' 키 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PathActorKey;

	/** 'CurrentSplinePointIndex' 키 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PointIndexKey;

	/** 목표 지점 저장 키 ('NextMoveLocation' 등) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PatrolPosKey;

protected:
	// --- [ Overrides ] ---
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};