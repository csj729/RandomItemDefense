#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterBaseCharacter.h" // ���� ���̽� Ŭ���� ����
#include "MonsterWaveData.generated.h"

// ������ ���̺��� �� ��(Row)�� �����ϴ� ����ü
USTRUCT(BlueprintType)
struct FMonsterWaveData : public FTableRowBase
{
	GENERATED_BODY()

public:
	// �� ���̺��� �̸� (��: "1�ܰ� - ��� ���̺�")
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString WaveName;

	// �� ���̺꿡�� ������ ���� �������Ʈ ���
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<AMonsterBaseCharacter>> MonstersToSpawn;
};