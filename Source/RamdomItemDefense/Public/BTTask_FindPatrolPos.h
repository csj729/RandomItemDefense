// Source/RamdomItemDefense/Public/BTTask_FindPatrolPos.h (수정)

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h" // [ ★★★ 수정: UBTTaskNode로 변경 ★★★ ]
#include "BTTask_FindPatrolPos.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API UBTTask_FindPatrolPos : public UBTTaskNode // [ ★★★ 수정: UBTTaskNode로 변경 ★★★ ]
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPos();

protected:
	// --- [ ★★★ 수정: ExecuteTask만 남김 ★★★ ] ---
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// --- [ ★★★ 수정 끝 ★★★ ] ---

public:
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