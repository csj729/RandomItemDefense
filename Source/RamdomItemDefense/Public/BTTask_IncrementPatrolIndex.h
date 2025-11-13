// Source/RamdomItemDefense/Public/BTTask_IncrementPatrolIndex.h (새 파일)

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

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	// 'PathToFollow' 키 (스플라인 총 개수 확인용)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PathActorKey;

	// 'CurrentSplinePointIndex' 키 (증가시킬 키)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PointIndexKey;
};