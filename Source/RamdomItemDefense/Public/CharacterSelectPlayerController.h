// CharacterSelectPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CharacterSelectPlayerController.generated.h"

/**
 * 캐릭터 선택 레벨(로비) 전용 플레이어 컨트롤러입니다.
 * 마우스 입력을 활성화하고, 선택 UI 위젯을 관리합니다.
 */
UCLASS()
class RAMDOMITEMDEFENSE_API ACharacterSelectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACharacterSelectPlayerController();

protected:
	virtual void BeginPlay() override;

public:
	// --- UI 설정 ---

	/** * (에디터 설정용) 생성할 캐릭터 선택 위젯 클래스 (WBP_CharacterSelect)
	 * 블루프린트 디폴트 세팅에서 할당해야 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> CharacterSelectWidgetClass;

	/** * 실제 생성된 위젯 인스턴스.
	 * BP_SelectableCharacter가 이 변수에 접근하여 위젯 함수를 호출합니다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UUserWidget> CharacterSelectWidgetInstance;
};