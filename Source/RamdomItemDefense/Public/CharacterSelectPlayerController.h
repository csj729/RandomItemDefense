// CharacterSelectPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CharacterSelectPlayerController.generated.h"

class ASelectableCharacter;
class ACameraActor;

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
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// [추가] 씬에 배치하거나 스폰할 실제 카메라 액터
	UPROPERTY()
	TObjectPtr<ACameraActor> MainCamera;

	// [추가] 현재 바라보고 있는(이동 중인) 타겟 캐릭터
	UPROPERTY()
	TObjectPtr<ASelectableCharacter> CurrentTarget;

	// [추가] 카메라 이동 속도 (높을수록 빠름)
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraInterpSpeed = 5.0f;

	FVector InitialCameraLocation;
	FRotator InitialCameraRotation;

public:
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

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void ToggleCharacterInfo(bool bVisible);

	void SetTargetCharacter(ASelectableCharacter* NewTarget);

	void ResetView();
};