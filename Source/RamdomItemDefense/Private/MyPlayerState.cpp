#include "MyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "MonsterSpawner.h"
#include "RamdomItemDefenseCharacter.h"
#include "InventoryComponent.h"

AMyPlayerState::AMyPlayerState()
{
	Gold = 0;
	ChoiceCount = 0;
}

void AMyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMyPlayerState, Gold);
	DOREPLIFETIME(AMyPlayerState, ChoiceCount);
	DOREPLIFETIME(AMyPlayerState, MySpawner);
}

// --- 골드 관련 (수정됨) ---
void AMyPlayerState::AddGold(int32 Amount)
{
	if (HasAuthority())
	{
		Gold += Amount;
		OnRep_Gold(); // 서버에서도 델리게이트 호출
	}
}

void AMyPlayerState::OnRep_Gold()
{
	// 골드 변경 델리게이트를 방송합니다.
	OnGoldChangedDelegate.Broadcast(Gold);
}

void AMyPlayerState::OnRep_MySpawner()
{
	// 스포너가 유효하게 할당되었음을 UI(MainHUD)에 알립니다.
	OnSpawnerAssignedDelegate.Broadcast(0); // 0은 의미 없는 값
}

// --- [코드 추가] 라운드 선택 관련 ---

/**
 * @brief (서버 전용) 라운드 선택 횟수를 설정합니다.
 */
void AMyPlayerState::AddChoiceCount(int32 Count)
{
	if (HasAuthority())
	{
		ChoiceCount += Count;
		OnRep_ChoiceCount(); // 서버에서도 델리게이트 호출
	}
}

/**
 * @brief 클라이언트에서 ChoiceCount가 복제되었을 때 호출됩니다.
 */
void AMyPlayerState::OnRep_ChoiceCount()
{
	// 선택 횟수 변경 델리게이트를 방송합니다.
	OnChoiceCountChangedDelegate.Broadcast(ChoiceCount);
}

/**
 * @brief (UI에서 호출) 라운드 선택(아이템/골드)을 사용합니다.
 */
void AMyPlayerState::Server_UseRoundChoice_Implementation(bool bChoseItemGacha)
{
	// (서버에서 실행됨)
	if (ChoiceCount <= 0)
	{
		return; // 선택 횟수가 없으면 무시
	}

	ChoiceCount--;
	OnRep_ChoiceCount(); // 서버에서 즉시 델리게이트 호출

	if (bChoseItemGacha)
	{
		// 이 PlayerState가 소유한 폰(캐릭터)을 가져옵니다.
		ARamdomItemDefenseCharacter* Character = GetPawn<ARamdomItemDefenseCharacter>();
		if (Character && Character->GetInventoryComponent())
		{
			// 캐릭터의 인벤토리 컴포넌트에게 무작위 아이템을 추가하라고 명령합니다.
			Character->GetInventoryComponent()->AddRandomItem();
		}

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("Player chose: Item Gacha"));
	}
	else
	{
		// TODO: 골드 도박 로직 (랜덤 골드 AddGold)
		const int32 GambleAmount = FMath::RandRange(100, 500); // 예: 100~500 골드
		AddGold(GambleAmount);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("Player chose: Gold Gamble (+%d)"), GambleAmount));
	}
}
// ------------------------------------