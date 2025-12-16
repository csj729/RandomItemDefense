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
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PathActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PointIndexKey;

protected:
	// --- [ Overrides ] ---
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};