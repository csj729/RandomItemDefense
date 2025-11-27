#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RIDGameInstance.generated.h"

/** 캐릭터 클래스 타입을 정의하거나, 실제 스폰할 클래스 정보를 저장 */
UCLASS()
class RAMDOMITEMDEFENSE_API URIDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 선택된 캐릭터 클래스 (BP_Warrior, BP_Archer 등)
	UPROPERTY(BlueprintReadWrite, Category = "Game Data")
	TSubclassOf<class APawn> SelectedCharacterClass;

	// 플레이어가 입력한 이름
	UPROPERTY(BlueprintReadWrite, Category = "Game Data")
	FString PlayerName;
};