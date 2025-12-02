// Source/RamdomItemDefense/Private/MainHUDWidget.cpp

#include "MainHUDWidget.h"
#include "MyPlayerState.h"
#include "RamdomItemDefensePlayerController.h"
#include "MonsterSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameState.h"       // [헤더 추가] GameState 접근용
#include "Components/TextBlock.h" // [헤더 추가] 텍스트 설정용

bool UMainHUDWidget::Initialize()
{
	bool bSuccess = Super::Initialize();
	if (!bSuccess) return false;

	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	MyPlayerController = GetOwningPlayer<ARamdomItemDefensePlayerController>();

	if (MyPlayerState)
	{
		BindPlayerStateEvents();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UMainHUDWidget::BindPlayerStateEvents);
	}

	if (MyPlayerState && MyPlayerState->MySpawner)
	{
		BindSpawnerEvents();
	}

	return true;
}

// [추가] 매 프레임 호출
void UMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// PVP 몬스터 현황판 갱신
	UpdatePVPMonsterCounts();
}

// [추가] 몬스터 현황 업데이트 로직
void UMainHUDWidget::UpdatePVPMonsterCounts()
{
	// 1. GameState 가져오기
	AMyGameState* GS = GetWorld()->GetGameState<AMyGameState>();
	if (!GS) return;

	// 2. 모든 플레이어 순회
	for (APlayerState* PS : GS->PlayerArray)
	{
		AMyPlayerState* MyPS = Cast<AMyPlayerState>(PS);

		// 플레이어에게 할당된 스포너가 있는지 확인
		if (MyPS && MyPS->MySpawner)
		{
			int32 CurrentCount = MyPS->MySpawner->GetCurrentMonsterCount();
			// GAMEOVER_MONSTER_NUM(60) 매크로 사용 (ItemTypes.h에 정의됨)
			FString StatusText = FString::Printf(TEXT("%d / %d"), CurrentCount, GAMEOVER_MONSTER_NUM);

			// [★★★ 코드 추가: 플레이어 이름 가져오기 ★★★]
			FString PlayerName = MyPS->GetPlayerName();

			// 3. 스포너 태그에 따라 UI 업데이트 (이름과 숫자를 동시에 갱신)
			if (MyPS->MySpawner->ActorHasTag(FName("Player1")))
			{
				// [Player 1 UI 갱신]
				if (P1_MonsterCountText)
				{
					P1_MonsterCountText->SetText(FText::FromString(StatusText));
				}
				// 이름도 같이 갱신하여 매칭 보장
				if (P1_NameText)
				{
					P1_NameText->SetText(FText::FromString(PlayerName));
				}
			}
			else if (MyPS->MySpawner->ActorHasTag(FName("Player2")))
			{
				// [Player 2 UI 갱신]
				if (P2_MonsterCountText)
				{
					P2_MonsterCountText->SetText(FText::FromString(StatusText));
				}
				// 이름도 같이 갱신하여 매칭 보장
				if (P2_NameText)
				{
					P2_NameText->SetText(FText::FromString(PlayerName));
				}
			}
		}
	}
}

void UMainHUDWidget::BindPlayerStateEvents()
{
	if (MyPlayerState == nullptr)
	{
		MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
	}

	if (MyPlayerState)
	{
		MyPlayerState->OnGoldChangedDelegate.AddDynamic(this, &UMainHUDWidget::OnGoldChanged);
		MyPlayerState->OnSpawnerAssignedDelegate.AddDynamic(this, &UMainHUDWidget::HandleSpawnerAssigned);
		MyPlayerState->OnUltimateChargeChangedDelegate.AddDynamic(this, &UMainHUDWidget::HandleUltimateChargeChanged);

		OnGoldChanged(MyPlayerState->GetGold());
		HandleUltimateChargeChanged(MyPlayerState->GetUltimateCharge());

		if (MyPlayerState->MySpawner)
		{
			BindSpawnerEvents();
		}

		UE_LOG(LogRamdomItemDefense, Log, TEXT("MainHUD: PlayerState Bound Successfully!"));
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UMainHUDWidget::BindPlayerStateEvents);
	}
}

void UMainHUDWidget::BindSpawnerEvents()
{
	if (MyPlayerState && MyPlayerState->MySpawner)
	{
		MyPlayerState->MySpawner->OnMonsterCountChangedDelegate.AddDynamic(this, &UMainHUDWidget::HandleMonsterCountChanged);
		HandleMonsterCountChanged(MyPlayerState->MySpawner->GetCurrentMonsterCount());
	}
}

// ... (나머지 함수들 HandleStatUpgradeClicked 등 기존 코드 그대로 유지) ...

void UMainHUDWidget::HandleStatUpgradeClicked()
{
	if (MyPlayerController)
	{
		MyPlayerController->ToggleStatUpgradeWidget();
	}
}

void UMainHUDWidget::HandleInventoryClicked()
{
	if (MyPlayerController)
	{
		MyPlayerController->ToggleInventoryWidget();
	}
}

void UMainHUDWidget::HandleSpawnerAssigned(int32 Dummy)
{
	BindSpawnerEvents();
}

void UMainHUDWidget::HandleMonsterCountChanged(int32 NewCount)
{
	const int32 MaxCount = GAMEOVER_MONSTER_NUM;
	OnMonsterCountChanged(NewCount, MaxCount);
}

void UMainHUDWidget::HandleUltimateChargeChanged(int32 NewValue)
{
	OnUltimateChargeChanged(NewValue, MAX_ULTIMATE_CHARGE);
}