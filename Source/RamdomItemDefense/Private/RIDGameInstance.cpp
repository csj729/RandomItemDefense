// Source/RamdomItemDefense/Private/RIDGameInstance.cpp

#include "RIDGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "RamdomItemDefense.h"
#include "MoviePlayer.h"
#include "Framework/Application/SlateApplication.h"

// 방 이름을 세션 설정에 저장할 때 사용할 키
const FName SESSION_SETTINGS_KEY_ROOM_NAME = TEXT("RoomName");

URIDGameInstance::URIDGameInstance()
{
}

void URIDGameInstance::Init()
{
	Super::Init();

	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();

		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &URIDGameInstance::OnCreateSessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &URIDGameInstance::OnFindSessionsCompleteInternal);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &URIDGameInstance::OnJoinSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &URIDGameInstance::OnDestroySessionComplete);
			FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &URIDGameInstance::BeginLoadingScreen);
			FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &URIDGameInstance::EndLoadingScreen);
		}
	}
}

void URIDGameInstance::CreateServer(FString RoomName, int32 MaxPlayers)
{
	if (!SessionInterface.IsValid()) return;

	DesiredRoomName = RoomName.IsEmpty() ? TEXT("Default Room") : RoomName;

	// 세션 설정 생성
	FOnlineSessionSettings SessionSettings;

	// 개발 중(LAN) 설정. 스팀 출시 시 false로 변경 필요
	SessionSettings.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bShouldAdvertise = true; // 다른 사람이 검색 가능하게
	SessionSettings.bUsesPresence = true;    // 로비 기능 사용
	SessionSettings.bAllowJoinInProgress = true;

	// [중요] 방 이름을 세션 설정에 저장 (클라이언트가 검색할 때 읽을 수 있음)
	SessionSettings.Set(SESSION_SETTINGS_KEY_ROOM_NAME, DesiredRoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 기존 세션이 있다면 파괴하고 새로 생성 (안전장치)
	const FName SessionName = NAME_GameSession;
	if (SessionInterface->GetNamedSession(SessionName))
	{
		SessionInterface->DestroySession(SessionName);
	}

	// 세션 생성 시작
	SessionInterface->CreateSession(0, SessionName, SessionSettings);
}

void URIDGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("CreateSession Success: %s"), *SessionName.ToString());

		// 방이 만들어지면 로비 맵(또는 게임 맵)으로 이동 (Listen Server)
		UGameplayStatics::OpenLevel(GetWorld(), "/Game/Maps/Map_CharacterSelect", true, "listen");
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession Failed!"));
	}
}

void URIDGameInstance::FindServers()
{
	if (!SessionInterface.IsValid()) return;

	// 검색 객체 초기화
	SessionSearch = MakeShareable(new FOnlineSessionSearch());

	// 개발 중(LAN) 설정
	SessionSearch->bIsLanQuery = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");
	SessionSearch->MaxSearchResults = 100;
	// Presence(로비) 세션만 검색
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	UE_LOG(LogTemp, Log, TEXT("Starting Find Sessions..."));

	// 세션 검색 시작
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void URIDGameInstance::OnFindSessionsCompleteInternal(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("Find Sessions Complete. Success: %d"), bWasSuccessful);

	TArray<FBlueprintSessionResult> Results;

	if (bWasSuccessful && SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Found %d Sessions"), SessionSearch->SearchResults.Num());

		for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			// 검색 결과 유효성 체크
			if (SearchResult.IsValid())
			{
				FBlueprintSessionResult BpResult;
				BpResult.OnlineResult = SearchResult;
				Results.Add(BpResult);
			}
		}
	}

	// 블루프린트(UI)로 결과 목록 전송
	OnFindSessionsComplete.Broadcast(Results);
}

void URIDGameInstance::JoinServer(FBlueprintSessionResult SessionToJoin)
{
	if (!SessionInterface.IsValid()) return;
	if (!SessionToJoin.OnlineResult.IsValid()) return;

	UE_LOG(LogTemp, Log, TEXT("JoinServer Requested..."));

	// 1. 이미 세션이 존재하는지 확인 (이게 에러의 원인!)
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Session already exists! Destroying it first..."));

		// 2. 참여하려던 세션 정보를 잠시 저장해둠
		PendingSessionToJoin = SessionToJoin;

		// 3. 기존 세션 파괴 (비동기) -> 완료되면 OnDestroySessionComplete 호출됨
		SessionInterface->DestroySession(NAME_GameSession);
	}
	else
	{
		// 4. 세션이 없으면 바로 참여 시도
		UE_LOG(LogTemp, Log, TEXT("No existing session. Joining immediately..."));
		SessionInterface->JoinSession(0, NAME_GameSession, SessionToJoin.OnlineResult);
	}
}

// [추가된 함수] 세션 파괴가 완료되면 호출됨
void URIDGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Existing Session Destroyed."));

		// 대기 중인 입장 요청이 있다면 진행
		if (PendingSessionToJoin.OnlineResult.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("Resuming Join Session..."));
			SessionInterface->JoinSession(0, NAME_GameSession, PendingSessionToJoin.OnlineResult);

			// 대기 변수 초기화
			PendingSessionToJoin = FBlueprintSessionResult();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to destroy existing session."));
	}
}

void URIDGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

	// [디버그] 결과 로그
	if (bSuccess)
	{
		RID_LOG(FColor::Green, TEXT("OnJoinSessionComplete: Success! Session='%s'"), *SessionName.ToString());
	}
	else
	{
		RID_LOG(FColor::Red, TEXT("OnJoinSessionComplete: FAILED! Result Code: %d"), (int32)Result);
	}

	OnJoinSessionResult.Broadcast(bSuccess);

	if (bSuccess)
	{
		FString ConnectInfo;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo))
		{
			RID_LOG(FColor::Cyan, TEXT(">> Original ConnectString: [%s]"), *ConnectInfo);

			// [핵심 수정] 포트 번호가 0인 경우 (:0) 감지 및 보정
			if (ConnectInfo.EndsWith(TEXT(":0")))
			{
				RID_LOG(FColor::Yellow, TEXT(">> [Warning] Port 0 Detected! Applying Fallback Port..."));

				// 기존의 :0 을 제거하고, 기본 포트(:7777)를 붙입니다.
				// 참고: 에디터 PIE 환경에서는 17777일 수도 있으니, 접속이 안 되면 17777로 바꿔보세요.
				ConnectInfo = ConnectInfo.LeftChop(2);
				ConnectInfo.Append(TEXT(":7777"));

				RID_LOG(FColor::Green, TEXT(">> Fixed ConnectString: [%s]"), *ConnectInfo);
			}

			APlayerController* PC = GetFirstLocalPlayerController();
			if (PC)
			{
				RID_LOG(FColor::Yellow, TEXT(">> Executing ClientTravel..."));
				PC->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
			}
		}
		else
		{
			RID_LOG(FColor::Red, TEXT(">> ERROR: GetResolvedConnectString Failed!"));
		}
	}
}

void URIDGameInstance::BeginLoadingScreen(const FString& MapName)
{
	if (!IsRunningDedicatedServer())
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;

		// [수정] LoadingWidgetClass가 설정되어 있다면 해당 위젯을 생성하여 사용
		if (LoadingWidgetClass)
		{
			UUserWidget* LoadingWidget = CreateWidget<UUserWidget>(this, LoadingWidgetClass);
			if (LoadingWidget)
			{
				// UMG 위젯을 Slate 위젯으로 변환하여 등록
				LoadingScreen.WidgetLoadingScreen = LoadingWidget->TakeWidget();
			}
		}
		else
		{
			// 설정된 위젯이 없으면 기존 테스트 위젯 사용 (안전장치)
			LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();
		}

		// (선택 사항) 로딩 중 동영상 재생이 필요 없다면 MoviePaths 관련 코드는 주석 처리 유지
		// LoadingScreen.MoviePaths.Add(TEXT("Loading")); 

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
}

void URIDGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	// 로딩이 끝나면 별도 처리가 필요하다면 여기에 작성
}

FString URIDGameInstance::GetRoomNameFromSessionResult(const FBlueprintSessionResult& Result)
{
	// 검색 결과가 유효한지 확인
	if (Result.OnlineResult.IsValid())
	{
		// 세션 설정(Settings) 가져오기
		const FOnlineSessionSettings& Settings = Result.OnlineResult.Session.SessionSettings;

		// [수정] Find는 FOnlineSessionSetting 포인터를 반환합니다.
		const FOnlineSessionSetting* RoomNameSetting = Settings.Settings.Find(SESSION_SETTINGS_KEY_ROOM_NAME);

		// 데이터가 존재하면
		if (RoomNameSetting)
		{
			// [수정] Setting 구조체 안의 'Data' 멤버에 접근해서 문자열로 변환합니다.
			return RoomNameSetting->Data.ToString();
		}
	}

	return TEXT("Unknown Room"); // 없으면 기본값 반환
}