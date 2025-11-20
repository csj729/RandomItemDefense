#include "RoundChoiceWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MyPlayerState.h" // PlayerState 헤더 인클루드
#include "Kismet/GameplayStatics.h" // GetPlayerState
#include "RamdomItemDefense.h" // RID_LOG 매크로용
#include "Engine/Engine.h" // GEngine 디버그 메시지 (선택 사항)

/** 위젯 생성 시 '단 한 번' 호출 (버튼 클릭 바인딩 전용) */
void URoundChoiceWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 버튼 클릭 이벤트는 위젯이 생성될 때 단 한 번만 바인딩합니다.
	if (ItemGachaButton)
	{
		ItemGachaButton->OnClicked.AddDynamic(this, &URoundChoiceWidget::HandleItemGachaClicked);
	}
	if (GoldGambleButton)
	{
		GoldGambleButton->OnClicked.AddDynamic(this, &URoundChoiceWidget::HandleGoldGambleClicked);
	}
}

/** 위젯이 뷰포트에 '추가될 때마다' 호출 */
void URoundChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindPlayerState();
}

// [추가] 재귀적 바인딩 함수
void URoundChoiceWidget::BindPlayerState()
{
	if (MyPlayerState) return;

	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();

	if (MyPlayerState)
	{
		if (!MyPlayerState->OnChoiceCountChangedDelegate.IsAlreadyBound(this, &URoundChoiceWidget::HandleChoiceCountChanged))
		{
			MyPlayerState->OnChoiceCountChangedDelegate.AddDynamic(this, &URoundChoiceWidget::HandleChoiceCountChanged);
		}
		// 초기값 갱신
		HandleChoiceCountChanged(MyPlayerState->GetChoiceCount());
	}
	else
	{
		// [핵심] 무한 재시도
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &URoundChoiceWidget::BindPlayerState);
	}
}

/** 위젯이 뷰포트에서 '제거될 때마다' 호출 */
void URoundChoiceWidget::NativeDestruct()
{
	Super::NativeDestruct();

	// 위젯이 화면에서 사라질 때, PlayerState 델리게이트 바인딩을 해제합니다.
	// 이렇게 하지 않으면 위젯이 숨겨진 상태에서도 델리게이트를 계속 수신할 수 있습니다.
	if (MyPlayerState)
	{
		MyPlayerState->OnChoiceCountChangedDelegate.RemoveDynamic(this, &URoundChoiceWidget::HandleChoiceCountChanged);
	}
}

/**
 * @brief PlayerState의 선택 횟수 변경 시 호출되어 텍스트 업데이트
 */
void URoundChoiceWidget::HandleChoiceCountChanged(int32 NewCount)
{
	if (ChoiceCountText)
	{
		// 텍스트 포맷 설정 (예: "남은 선택 횟수: 1")
		FFormatNamedArguments Args;
		Args.Add(TEXT("Count"), FText::AsNumber(NewCount));
		// 직접 한글 문자열 리터럴을 사용합니다.
		ChoiceCountText->SetText(FText::Format(FText::FromString(TEXT("남은 선택 횟수: {Count}")), Args));
	}
	// --- [코드 수정] GEngine을 RID_LOG로 대체 ---
	else
	{
		// ChoiceCountText 변수가 바인딩되지 않았을 경우 로그 출력
		RID_LOG(FColor::Red, TEXT("[Widget] ERROR: ChoiceCountText is NULL!"));
	}
	// -----------------------------------------

	// 선택 횟수가 0 이하이면 버튼 비활성화 (클릭 방지)
	const bool bCanChoose = (NewCount > 0);
	if (ItemGachaButton) ItemGachaButton->SetIsEnabled(bCanChoose);
	if (GoldGambleButton) GoldGambleButton->SetIsEnabled(bCanChoose);

	// 위젯 자체를 화면에서 보이거나 숨기는 것은 PlayerController가 담당합니다.
}

/**
 * @brief [아이템 뽑기] 버튼 클릭 시 PlayerState의 서버 함수 호출
 */
void URoundChoiceWidget::HandleItemGachaClicked()
{
	if (MyPlayerState)
	{
		// 서버에 아이템 뽑기를 선택했다고 알림 (true 전달)
		MyPlayerState->Server_UseRoundChoice(true);
	}
}

/**
 * @brief [골드 도박] 버튼 클릭 시 PlayerState의 서버 함수 호출
 */
void URoundChoiceWidget::HandleGoldGambleClicked()
{
	if (MyPlayerState)
	{
		// 서버에 골드 도박을 선택했다고 알림 (false 전달)
		MyPlayerState->Server_UseRoundChoice(false);
	}
}