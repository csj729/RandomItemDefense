#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterBaseCharacter.h"
#include "MonsterWaveData.generated.h"

USTRUCT(BlueprintType)
struct FMonsterWaveData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** 웨이브 이름 (예: "1단계 - 고블린") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString WaveName;

	/** 이 웨이브에서 스폰될 몬스터 종류 목록 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<AMonsterBaseCharacter>> MonstersToSpawn;
};