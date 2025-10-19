#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsterSpawner.generated.h"

class AMonsterBaseCharacter;

UCLASS()
class RAMDOMITEMDEFENSE_API AMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawner();

	// GameMode�� ȣ���� ���� ���� �Լ�
	void BeginSpawning(TSubclassOf<AMonsterBaseCharacter> MonsterClass, int32 Count);

	// ���Ͱ� �׾��� �� ȣ���� �Լ�
	void OnMonsterKilled();

	// ���� ������ ���� ���� ��ȯ�ϴ� �Լ�
	int32 GetCurrentMonsterCount() const { return CurrentMonsterCount; }

	// �� ���������� ���ӿ��� ���·� ����� �Լ�
	void SetGameOver();

	// �� ���������� ���ӿ��� �������� Ȯ���ϴ� �Լ�
	bool IsGameOver() const { return bIsGameOver; }

protected:
	virtual void BeginPlay() override;

private:
	// ���͸� ������ �����ϴ� �Լ�
	void SpawnMonster();

	// ������ ������ Ŭ����
	UPROPERTY()
	TSubclassOf<AMonsterBaseCharacter> MonsterClassToSpawn;

	// �̹� ���̺꿡 ������ �� ���� ��
	int32 TotalToSpawn;

	// ���ݱ��� ������ ���� ��
	int32 SpawnCounter;

	// ���� Ÿ�̸� �ڵ�
	FTimerHandle SpawnTimerHandle;

	// ����� Ȱ��ȭ ���θ� �����Ϳ��� �Ѱ� �� �� �ִ� ����
	UPROPERTY(EditAnywhere, Category = "Debugging")
	bool bEnableDebug;

	// �� �����ʰ� �����ϴ� ���� ���� ��
	UPROPERTY(VisibleAnywhere, Category = "Live Count")
	int32 CurrentMonsterCount;

	// �� ���������� ���ӿ��� ����
	bool bIsGameOver;
};