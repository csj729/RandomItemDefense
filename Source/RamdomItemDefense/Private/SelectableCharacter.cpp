#include "SelectableCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "CharacterSelectPlayerController.h"
#include "MyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "RIDGameInstance.h" 

ASelectableCharacter::ASelectableCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// 마우스 오버 이벤트 활성화
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic")); // 클릭 감지를 위해 콜리전 필요
	CameraViewPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CameraViewPoint"));
	CameraViewPoint->SetupAttachment(RootComponent);
}

void ASelectableCharacter::BeginPlay()
{
	Super::BeginPlay();
	bIsSelected = false;
}

void ASelectableCharacter::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();

	// [하이라이트 기능] CustomDepth를 켜서 외곽선(PostProcess) 효과를 받게 함
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilValue(1); // 1번 스텐실 (예: 흰색 외곽선)
	}
}

void ASelectableCharacter::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();

	// 선택된 상태가 아니라면 하이라이트 끄기
	if (!bIsSelected && Mesh)
	{
		Mesh->SetRenderCustomDepth(false);
	}
}

void ASelectableCharacter::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ACharacterSelectPlayerController* SelectPC = Cast<ACharacterSelectPlayerController>(PC);

	// --- [Case 1] 이미 선택된 나를 '다시' 클릭함 -> 선택 해제 (초기화) ---
	if (bIsSelected)
	{
		// 1. 카메라 줌 아웃 (원래 위치로)
		if (SelectPC) SelectPC->ResetView();

		// 2. 모든 캐릭터 다시 보이게 처리
		TArray<AActor*> AllChars;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASelectableCharacter::StaticClass(), AllChars);
		for (AActor* Actor : AllChars)
		{
			if (ASelectableCharacter* Char = Cast<ASelectableCharacter>(Actor))
			{
				Char->SetActorHiddenInGame(false); // 다시 보이기
				Char->ResetSelection(); // 하이라이트 끄기
			}
		}
		return; // 여기서 종료
	}

	// --- [Case 2] 새로운 캐릭터 선택 -> 집중 모드 (나머지 숨김) ---

	// 1. 카메라 줌 인 (나에게로)
	if (SelectPC) SelectPC->SetTargetCharacter(this);

	// 2. 캐릭터들 순회
	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASelectableCharacter::StaticClass(), AllChars);

	for (AActor* Actor : AllChars)
	{
		ASelectableCharacter* Char = Cast<ASelectableCharacter>(Actor);
		if (!Char) continue;

		if (Char == this)
		{
			Char->SetActorHiddenInGame(false);
			Char->bIsSelected = true;

			if (Char->Mesh)
			{
				Char->Mesh->SetRenderCustomDepth(true);
				Char->Mesh->SetCustomDepthStencilValue(2);
			}
			Char->PlaySelectionAnimation();
		}
		else
		{
			Char->ResetSelection();
			Char->SetActorHiddenInGame(true);
		}
	}
}

void ASelectableCharacter::PlaySelectionAnimation()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		AMyPlayerState* MyPS = PC->GetPlayerState<AMyPlayerState>();

		// 2. 이미 준비 완료 상태라면 애니메이션 재생 안 함 (함수 종료)
		if (MyPS && MyPS->IsReady())
		{
			return;
		}
	}

	// 3. 준비 상태가 아닐 때만 정상 재생
	if (Mesh && SelectAnimation)
	{
		Mesh->PlayAnimation(SelectAnimation, false);
	}
}

void ASelectableCharacter::ResetSelection()
{
	bIsSelected = false;
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(false);
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint); // 다시 Idle 상태(AnimBP)로 복귀
	}
}