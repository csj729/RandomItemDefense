// Source/RamdomItemDefense/Public/RIDGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FindSessionsCallbackProxy.h"
#include "RIDGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRIDFindSessionsCompleteDelegate, const TArray<FBlueprintSessionResult>&, SessionResults);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionResultDelegate, bool, bSuccessful);

UCLASS()
class RAMDOMITEMDEFENSE_API URIDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	URIDGameInstance();
	virtual void Init() override;

	UPROPERTY(BlueprintReadWrite, Category = "Game Data")
	TSubclassOf<class APawn> SelectedCharacterClass;

	UPROPERTY(BlueprintReadWrite, Category = "Game Data")
	FString PlayerName;

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void CreateServer(FString RoomName, int32 MaxPlayers = 2);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void FindServers();

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void JoinServer(FBlueprintSessionResult SessionToJoin);

	UPROPERTY(BlueprintAssignable, Category = "Network|Session")
	FRIDFindSessionsCompleteDelegate OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Network|Session")
	FOnSessionResultDelegate OnCreateSessionResult;

	UPROPERTY(BlueprintAssignable, Category = "Network|Session")
	FOnSessionResultDelegate OnJoinSessionResult;

protected:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FBlueprintSessionResult PendingSessionToJoin;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsCompleteInternal(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	UFUNCTION()
	void BeginLoadingScreen(const FString& MapName);

	UFUNCTION()
	void EndLoadingScreen(UWorld* InLoadedWorld);

	FString DesiredRoomName;
};