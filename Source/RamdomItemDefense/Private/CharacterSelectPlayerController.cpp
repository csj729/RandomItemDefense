// CharacterSelectPlayerController.cpp

#include "CharacterSelectPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "SelectableCharacter.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

ACharacterSelectPlayerController::ACharacterSelectPlayerController()
{
	// 생성자에서 기본 마우스 설정
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;

	bIsPlayerReady = false;
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

		AActor* FoundCamera = UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass());
		MainCamera = Cast<ACameraActor>(FoundCamera);

		if (!MainCamera)
		{
			// 없으면 현재 뷰 위치에 새로 스폰
			FTransform SpawnTransform = PlayerCameraManager ? PlayerCameraManager->GetTransform() : FTransform::Identity;
			MainCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), SpawnTransform);
		}

		// 2. 플레이어의 뷰 타겟을 이 카메라로 고정
		if (MainCamera)
		{
			SetViewTarget(MainCamera);
			InitialCameraLocation = MainCamera->GetActorLocation();
			InitialCameraRotation = MainCamera->GetActorRotation();
		}

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (CharacterSelectMappingContext)
			{
				Subsystem->AddMappingContext(CharacterSelectMappingContext, 0);
			}
		}
	}
}

void ACharacterSelectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// [Enhanced Input] 액션 바인딩
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (BackToMenuAction)
		{
			// Started: 키를 누르는 순간 발생
			EnhancedInputComponent->BindAction(BackToMenuAction, ETriggerEvent::Started, this, &ACharacterSelectPlayerController::OnBackToMainMenu);
		}
	}
}

void ACharacterSelectPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MainCamera)
	{
		// 1. 목표 위치/회전 설정
		FVector TargetLoc = InitialCameraLocation;
		FRotator TargetRot = InitialCameraRotation;

		// 타겟 캐릭터가 있다면(선택됨), 그 캐릭터의 뷰 포인트로 목표 변경
		if (CurrentTarget && CurrentTarget->GetCameraViewPoint())
		{
			TargetLoc = CurrentTarget->GetCameraViewPoint()->GetComponentLocation();
			TargetRot = CurrentTarget->GetCameraViewPoint()->GetComponentRotation();
		}

		// 2. 부드럽게 이동 (선택 해제 시에는 초기 위치로 복귀함)
		FVector NewLoc = FMath::VInterpTo(MainCamera->GetActorLocation(), TargetLoc, DeltaTime, CameraInterpSpeed);
		FRotator NewRot = FMath::RInterpTo(MainCamera->GetActorRotation(), TargetRot, DeltaTime, CameraInterpSpeed);

		MainCamera->SetActorLocationAndRotation(NewLoc, NewRot);
	}
}

void ACharacterSelectPlayerController::SetPlayerReady(bool bReady)
{
	bIsPlayerReady = bReady;

	if (bIsPlayerReady && CurrentTarget)
	{
		if (CurrentTarget->ReadySound)
		{
			UGameplayStatics::PlaySound2D(this, CurrentTarget->ReadySound);
		}
	}

}

void ACharacterSelectPlayerController::ResetView()
{
	if (bIsPlayerReady)
	{
		return;
	}

	CurrentTarget = nullptr;
	ToggleCharacterInfo(false);
}

void ACharacterSelectPlayerController::SetTargetCharacter(ASelectableCharacter* NewTarget)
{
	if (bIsPlayerReady)
	{
		return;
	}

	CurrentTarget = NewTarget;
	ToggleCharacterInfo(true);
}

void ACharacterSelectPlayerController::OnBackToMainMenu(const FInputActionValue& Value)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			if (SessionInterface->GetNamedSession(NAME_GameSession))
			{
				SessionInterface->DestroySession(NAME_GameSession);
			}
		}
	}

	UGameplayStatics::OpenLevel(this, FName("Map_MainMenu"));
}