// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "MultiplayerSessionSubsystem.h"
#include "Menu.generated.h"


/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MultiplayerSessions|Menu")
	void MenuSetup(int32 NumberOfPublicConnections = 4,FString TypeOfMatch = FString(TEXT("FreeForAll")),FString PathToLobby = FString(TEXT("/Game/ThirdPerson/Maps/Lobby")));
	
	
protected:
	
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	///
	///Callbacks for the custom delegates on the MultiplayerSessionSubsystem
	///
	UFUNCTION(BlueprintCallable, Category = "MultiplayerSessions|Menu")
	void OnCreateSession(bool bWasSuccessful);
	void OnFindSession(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION(BlueprintCallable, Category = "MultiplayerSessions|Menu")
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION(BlueprintCallable, Category = "MultiplayerSessions|Menu")
	void OnStartSession(bool bWasSuccessful);
	
	
private:
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;

	UFUNCTION(BlueprintCallable, Category = "MultiplayerSessions|Menu")
	void HostButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "MultiplayerSessions|Menu")
	void JoinButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "MultiplayerSessions|Menu")
	void QuitButtonClicked();
	
	void MenuTearDown();

	//The subsystem designed to handle all online session functonality
	class UMultiplayerSessionSubsystem* MultiplayerSessionSubsystem;

	int32 NumPublicConnections{4};
	FString MatchType{TEXT("FreeForAll")};
	FString PathToLobby{TEXT("")};
};
