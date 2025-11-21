#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MonsterAIController.generated.h"

UCLASS()
class RAMDOMITEMDEFENSE_API AMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMonsterAIController();

	void SetPatrolPath(AActor* PathActor);

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	// 비헤이비어 트리 에셋을 저장할 변수
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<class UBehaviorTree> BehaviorTree;

	// 블랙보드 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<class UBlackboardComponent> BlackboardComponent;
};