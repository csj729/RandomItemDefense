// CharacterSelectPlayerController.cpp

#include "CharacterSelectPlayerController.h"
#include "Blueprint/UserWidget.h"

ACharacterSelectPlayerController::ACharacterSelectPlayerController()
{
	// 생성자에서 기본 마우스 설정
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ACharacterSelectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// [핵심 수정] 이 컨트롤러가 '로컬 플레이어(내 화면의 주인)'일 때만 UI를 생성합니다.
	// 서버가 남의 컨트롤러를 가지고 이 코드를 실행할 때는 건너뛰게 됩니다.
	if (IsLocalPlayerController())
	{
		// 1. 마우스 커서 및 입력 설정
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;

		// 2. UI 위젯 생성 및 화면 표시
		if (CharacterSelectWidgetClass)
		{
			CharacterSelectWidgetInstance = CreateWidget<UUserWidget>(this, CharacterSelectWidgetClass);

			if (CharacterSelectWidgetInstance)
			{
				CharacterSelectWidgetInstance->AddToViewport();

				// (선택 사항) 입력 모드 설정
				FInputModeGameAndUI InputMode;
				InputMode.SetWidgetToFocus(CharacterSelectWidgetInstance->TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				SetInputMode(InputMode);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("CharacterSelectPlayerController: Widget Class NOT assigned!"));
		}
	}
}