// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"  
#include "Engine/LocalPlayer.h"
#include "Interfaces/OnlineIdentityInterface.h"

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
	LastSessionSetting->bIsLANMatch = Online::GetSubsystem(GetWorld())->GetSubsystemName() == "NULL" ?true : false;
	LastSessionSetting->NumPublicConnections = NumPublicConnections;
	LastSessionSetting->bAllowJoinInProgress = true;
	LastSessionSetting->bAllowJoinInProgress = true;
	LastSessionSetting->bShouldAdvertise = true;
	LastSessionSetting->bUsesPresence = true;
	LastSessionSetting->bUseLobbiesIfAvailable = true;
	LastSessionSetting->Set(FName("MatchType"),MatchType , EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSetting->BuildUniqueId = 1;
	
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	FUniqueNetIdRepl NetIdPtr = *LocalPlayer->GetPreferredUniqueNetId();
	if (NetIdPtr.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Using NetId from Identity interface: %s"), *NetIdToString(NetIdPtr));
	}
	if(!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSetting))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		//Broadcast our own custom delegate
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
	
}

void UMultiplayerSessionSubsystem::FindSession(int32 MaxSearchResults)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionCompleteDelegate);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = Online::GetSubsystem(GetWorld())->GetSubsystemName() == "NULL" ?true : false;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	
	// 使用统一的网络ID获取方法
	FUniqueNetIdRepl NetId = GetPlayerNetId();
	if (!NetId.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot join session: Invalid NetId"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Cannot join session: Player not logged in"));
		}
		return;
	}

	if (const FUniqueNetId* RawNetId = NetId.GetUniqueNetId().Get())
	{
		FString NetIdStr = RawNetId->ToString();
		UE_LOG(LogTemp, Warning, TEXT("NetId: %s"), *NetIdStr);
	}
	if (!SessionInterface->FindSessions(*NetId, LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(),false);
	}

}

void UMultiplayerSessionSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}
	FUniqueNetIdRepl NetId = GetPlayerNetId();
	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	if (SessionInterface->JoinSession(*NetId, NAME_GameSession,SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionSubsystem::StartSession()
{
	
}

void UMultiplayerSessionSubsystem::DestroySession()
{
	
}

void UMultiplayerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
	
}

void UMultiplayerSessionSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(),false);
		return;
	}
	
	MultiplayerOnFindSessionComplete.Broadcast(LastSessionSearch->SearchResults,bWasSuccessful);
	
}

void UMultiplayerSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
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

FUniqueNetIdRepl UMultiplayerSessionSubsystem::GetPlayerNetId() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("No OnlineSubsystem found"));
		return FUniqueNetIdRepl();
	}

	IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Identity interface not valid"));
		return FUniqueNetIdRepl();
	}

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("No local player found"));
		return FUniqueNetIdRepl();
	}

	int32 ControllerId = LocalPlayer->GetControllerId();
    
	// 首先尝试从身份接口获取
	TSharedPtr<const FUniqueNetId> NetIdPtr = IdentityInterface->GetUniquePlayerId(ControllerId);
	if (NetIdPtr.IsValid())
	{
		FUniqueNetIdRepl NetIdRepl(NetIdPtr);
		UE_LOG(LogTemp, Warning, TEXT("Using NetId from Identity interface: %s"), *NetIdToString(NetIdRepl));
		return NetIdRepl;
	}

	// 如果身份接口没有，尝试其他方法
	FUniqueNetIdRepl PreferredNetId = LocalPlayer->GetPreferredUniqueNetId();
	if (PreferredNetId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Using PreferredUniqueNetId: %s"), *NetIdToString(PreferredNetId));
		return PreferredNetId;
	}

	FUniqueNetIdRepl CachedNetId = LocalPlayer->GetCachedUniqueNetId();
	if (CachedNetId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Using CachedUniqueNetId: %s"), *NetIdToString(CachedNetId));
		return CachedNetId;
	}

	UE_LOG(LogTemp, Warning, TEXT("Could not get valid NetId from any source"));
	return FUniqueNetIdRepl();
}
