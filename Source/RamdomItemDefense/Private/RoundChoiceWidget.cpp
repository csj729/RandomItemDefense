#include "RoundChoiceWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MyPlayerState.h" // PlayerState 헤더 인클루드
#include "Kismet/GameplayStatics.h" // GetPlayerState

void URoundChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// PlayerState 참조 가져오기
	MyPlayerState = GetOwningPlayerState<AMyPlayerState>();

	// PlayerState 유효성 검사 및 델리게이트 바인딩
	if (MyPlayerState)
	{
		// PlayerState의 선택 횟수 변경 델리게이트에 C++ 함수 바인딩
		MyPlayerState->OnChoiceCountChangedDelegate.AddDynamic(this, &URoundChoiceWidget::HandleChoiceCountChanged);
		// 현재 값으로 텍스트 즉시 업데이트
		HandleChoiceCountChanged(MyPlayerState->GetChoiceCount());
	}
	else
	{
		// PlayerState가 아직 준비되지 않았다면 다음 틱에 다시 시도
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			MyPlayerState = GetOwningPlayerState<AMyPlayerState>();
			if (MyPlayerState)
			{
				MyPlayerState->OnChoiceCountChangedDelegate.AddDynamic(this, &URoundChoiceWidget::HandleChoiceCountChanged);
				HandleChoiceCountChanged(MyPlayerState->GetChoiceCount());
			}
			});
	}

	// 버튼 클릭 이벤트에 C++ 함수 바인딩
	if (ItemGachaButton)
	{
		ItemGachaButton->OnClicked.AddDynamic(this, &URoundChoiceWidget::HandleItemGachaClicked);
	}
	if (GoldGambleButton)
	{
		GoldGambleButton->OnClicked.AddDynamic(this, &URoundChoiceWidget::HandleGoldGambleClicked);
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

	else if(GEngine)
    {
        // ChoiceCountText 변수가 바인딩되지 않았을 경우 로그 출력
         GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("[Widget] ERROR: ChoiceCountText is NULL!"));
    }

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