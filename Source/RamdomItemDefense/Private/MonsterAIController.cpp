#include "MonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" // GEngine ��ü

// �����ڴ� ������ ����
AMonsterAIController::AMonsterAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

// OnPossess �Լ��� �Ʒ��� ���� �����մϴ�.
void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTree)
	{
		// �����带 �ʱ�ȭ�ϰ� �����̺�� Ʈ���� �����մϴ�.
		BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);

		// ================== [AI ����� 1-2] ==================
		// ���忡 ��ġ�� BP_MonsterPath ���͸� ã���ϴ�.
		// BP_MonsterPath�� �θ��� Actor Ŭ������ ã��, �±׸� �̿��� �� ��Ȯ�� ã���ϴ�.
		// (������ �ִ� BP_MonsterPath ������ ������ �гο��� Actor > Tags�� "MonsterPath" �±׸� �߰����ּ���.)
		TArray<AActor*> FoundPaths;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("MonsterPath"), FoundPaths);

		AActor* PathActor = nullptr;
		if (FoundPaths.Num() > 0)
		{
			PathActor = FoundPaths[0];
			FString PathName = GetNameSafe(PathActor);
		}
		else
		{
			RID_LOG(FColor::Red, TEXT("AIController: ERROR! Could not find Actor with tag 'MonsterPath'."));
		}
		// =======================================================

		if (PathActor)
		{
			// �����忡 ��� ���Ϳ� ���� �ε����� �����մϴ�.
			BlackboardComponent->SetValueAsObject(TEXT("PathToFollow"), PathActor);
			BlackboardComponent->SetValueAsInt(TEXT("CurrentSplinePointIndex"), 0);
		}

		RunBehaviorTree(BehaviorTree);
	}
	else
	{
		// --- [�ڵ� ����] GEngine�� RID_LOG�� ��ü ---
		RID_LOG(FColor::Red, TEXT("AIController: ERROR! BehaviorTree is NOT assigned."));
		// -----------------------------------------
	}
}