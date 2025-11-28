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

	// 1. 다른 모든 캐릭터 선택 해제
	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASelectableCharacter::StaticClass(), AllChars);
	for (AActor* Actor : AllChars)
	{
		if (ASelectableCharacter* Char = Cast<ASelectableCharacter>(Actor))
		{
			Char->ResetSelection(); // 기존 선택 해제 (하이라이트 끄기 등)
		}
	}

	// 2. 나 선택 (하이라이트 색상 변경 - 예: 주황색 스텐실 2)
	bIsSelected = true;
	if (Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Highlight"));
		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilValue(2);
	}

	// 3. 애니메이션 재생
	PlaySelectionAnimation();
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