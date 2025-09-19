// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerSessionSubsystem.generated.h"

///
/// Declaring our own custom delegates for the Menu class to bind callbacks to 
///
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete,bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionComplete,const TArray<FOnlineSessionSearchResult>& SessionResult,bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete,EOnJoinSessionCompleteResult::Type Result);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete,bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete,bool, bWasSuccessful);
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionSubsystem();
	TSharedPtr<FOnlineSessionSettings> LastSessionSetting;

	///
	///To handle session functionality. The Menu class will call these
	///
	void CreateSession(int32 NumPublicConnections,FString MatchType);
	void FindSession(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	void StartSession();
	void DestroySession();


	///
	///Our own custom delegates foe the Menu class to bind callbacks to 
	///
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	
protected:
	///
	///Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	///This don't need to be called outside this class.
	///
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	FString NetIdToString(const FUniqueNetIdRepl& NetId) const;

private:
	IOnlineSessionPtr SessionInterface;
	///
	///To add to the Online Session Interface delegate list.
	///We'll bind our MultiplayerSessionSubsystem internal callbacks to these.
	///
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;
};
