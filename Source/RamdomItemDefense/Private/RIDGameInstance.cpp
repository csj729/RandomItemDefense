// Source/RamdomItemDefense/Private/RIDGameInstance.cpp

#include "RIDGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
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

	OnJoinSessionResult.Broadcast(bSuccess);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString ConnectInfo;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo))
		{
			UE_LOG(LogTemp, Log, TEXT("Join Success! Traveling to: %s"), *ConnectInfo);

			// 해당 주소로 클라이언트 이동 (ClientTravel)
			APlayerController* PC = GetFirstLocalPlayerController();
			if (PC)
			{
				PC->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Join Session Failed."));
	}
}

void URIDGameInstance::BeginLoadingScreen(const FString& MapName)
{
	if (!IsRunningDedicatedServer())
	{
		FLoadingScreenAttributes LoadingScreen;

		// 1. 로딩이 끝나도 플레이어가 키를 누를 때까지 대기할지 여부 (False: 바로 시작)
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;

		// 2. 테스트용 기본 로딩 위젯 사용 (텍스트만 뜹니다)
		// 나중에 커스텀 Slate 위젯으로 교체 가능
		LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

		// 3. 동영상 재생을 원하면 아래 주석 해제 (Content/Movies/Loading.mp4 필요)
		// LoadingScreen.MoviePaths.Add(TEXT("Loading")); 

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
}

void URIDGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	// 로딩이 끝나면 별도 처리가 필요하다면 여기에 작성
}