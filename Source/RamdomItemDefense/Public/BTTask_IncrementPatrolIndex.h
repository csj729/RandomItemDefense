#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_IncrementPatrolIndex.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UBTTask_IncrementPatrolIndex : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_IncrementPatrolIndex();

	// --- [ Configuration : Blackboard Keys ] ---
	/** 'PathToFollow' 키 (총 포인트 수 확인용) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PathActorKey;

	/** 'CurrentSplinePointIndex' 키 (증가 대상) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PointIndexKey;

protected:
	// --- [ Overrides ] ---
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};