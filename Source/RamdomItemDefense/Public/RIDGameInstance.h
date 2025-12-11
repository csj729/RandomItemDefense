#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FindSessionsCallbackProxy.h"
#include "Blueprint/UserWidget.h"
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

	// --- [ Game Data ] ---
	UPROPERTY(BlueprintReadWrite, Category = "Game Data")
	TSubclassOf<class APawn> SelectedCharacterClass;

	UPROPERTY(BlueprintReadWrite, Category = "Game Data")
	FString PlayerName;

	// --- [ Session Management ] ---
	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void CreateServer(FString RoomName, int32 MaxPlayers = 2);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void FindServers();

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void JoinServer(FBlueprintSessionResult SessionToJoin);

	UFUNCTION(BlueprintPure, Category = "Network|Session")
	FString GetRoomNameFromSessionResult(const FBlueprintSessionResult& Result);

	// --- [ Delegates ] ---
	UPROPERTY(BlueprintAssignable, Category = "Network|Session")
	FRIDFindSessionsCompleteDelegate OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Network|Session")
	FOnSessionResultDelegate OnCreateSessionResult;

	UPROPERTY(BlueprintAssignable, Category = "Network|Session")
	FOnSessionResultDelegate OnJoinSessionResult;

	// --- [ Loading Screen ] ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loading")
	TSubclassOf<UUserWidget> LoadingWidgetClass;

protected:
	// --- [ Internal Session Logic ] ---
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FBlueprintSessionResult PendingSessionToJoin;
	FString DesiredRoomName;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsCompleteInternal(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// --- [ Internal Loading Logic ] ---
	UFUNCTION()
	void BeginLoadingScreen(const FString& MapName);

	UFUNCTION()
	void EndLoadingScreen(UWorld* InLoadedWorld);
};