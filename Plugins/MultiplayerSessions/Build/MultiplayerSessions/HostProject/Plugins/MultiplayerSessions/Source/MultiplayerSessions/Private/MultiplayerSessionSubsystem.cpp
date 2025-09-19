// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

UMultiplayerSessionSubsystem::UMultiplayerSessionSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnCreateSessionComplete)),
	FindSessionCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this,&ThisClass::OnFindSessionComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this,&ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnStartSessionComplete))
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
}

void UMultiplayerSessionSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	//Store the delegate in a FDelegateHandle so can later remove it from the delegate list
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	LastSessionSetting = MakeShareable(new FOnlineSessionSettings());
	LastSessionSetting->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ?true : false;
	LastSessionSetting->NumPublicConnections = NumPublicConnections;
	LastSessionSetting->bAllowJoinInProgress = true;
	LastSessionSetting->bAllowJoinInProgress = true;
	LastSessionSetting->bShouldAdvertise = true;
	LastSessionSetting->bUsesPresence = true;
	LastSessionSetting->bUseLobbiesIfAvailable = true;
	LastSessionSetting->Set(FName("MatchType"),MatchType , EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	FUniqueNetIdRepl NetIdPtr = *LocalPlayer->GetPreferredUniqueNetId();
	if (NetIdPtr.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Using NetId from Identity interface: %s"), *NetIdToString(NetIdPtr));
	}
	if(!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSetting))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	
}

void UMultiplayerSessionSubsystem::FindSession(int32 MaxSearchResults)
{
	
}

void UMultiplayerSessionSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	
}

void UMultiplayerSessionSubsystem::StartSession()
{
	
}

void UMultiplayerSessionSubsystem::DestroySession()
{
	
}

void UMultiplayerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
}

void UMultiplayerSessionSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
}

void UMultiplayerSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
}

void UMultiplayerSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
}

void UMultiplayerSessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}

FString UMultiplayerSessionSubsystem::NetIdToString(const FUniqueNetIdRepl& NetId) const
{
	if (!NetId.IsValid())
	{
		return TEXT("Invalid_NetId");
	}
    
	// 使用 GetUniqueNetId() 方法获取原始指针
	if (const FUniqueNetId* RawNetId = NetId.GetUniqueNetId().Get())
	{
		return RawNetId->ToString();
	}
    
	return TEXT("Invalid_RawNetId");
}
