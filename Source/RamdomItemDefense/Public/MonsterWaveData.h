#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterBaseCharacter.h" // 몬스터 베이스 클래스 포함
#include "MonsterWaveData.generated.h"

// 데이터 테이블의 한 행(Row)을 정의하는 구조체
USTRUCT(BlueprintType)
struct FMonsterWaveData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// 이 웨이브의 이름 (예: "1단계 - 고블린 웨이브")
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString WaveName;

	// 이 웨이브에서 스폰될 몬스터 블루프린트 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<AMonsterBaseCharacter>> MonstersToSpawn;
};