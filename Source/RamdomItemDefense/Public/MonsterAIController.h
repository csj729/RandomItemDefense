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

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	// �����̺�� Ʈ�� ������ ������ ����
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<class UBehaviorTree> BehaviorTree;

	// ������ ������Ʈ
	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<class UBlackboardComponent> BlackboardComponent;
};