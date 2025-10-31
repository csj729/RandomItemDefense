#include "MonsterAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MonsterBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "RamdomItemDefense.h" // GEngine 대체

// 생성자는 이전과 동일
AMonsterAIController::AMonsterAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

// OnPossess 함수를 아래와 같이 수정합니다.
void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTree)
	{
		// 블랙보드를 초기화하고 비헤이비어 트리를 실행합니다.
		BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);

		// ================== [AI 디버깅 1-2] ==================
		// 월드에 배치된 BP_MonsterPath 액터를 찾습니다.
		// BP_MonsterPath의 부모인 Actor 클래스로 찾고, 태그를 이용해 더 정확히 찾습니다.
		// (레벨에 있는 BP_MonsterPath 액터의 디테일 패널에서 Actor > Tags에 "MonsterPath" 태그를 추가해주세요.)
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
			// 블랙보드에 경로 액터와 시작 인덱스를 저장합니다.
			BlackboardComponent->SetValueAsObject(TEXT("PathToFollow"), PathActor);
			BlackboardComponent->SetValueAsInt(TEXT("CurrentSplinePointIndex"), 0);
		}

		RunBehaviorTree(BehaviorTree);
	}
	else
	{
		// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
		RID_LOG(FColor::Red, TEXT("AIController: ERROR! BehaviorTree is NOT assigned."));
		// -----------------------------------------
	}
}