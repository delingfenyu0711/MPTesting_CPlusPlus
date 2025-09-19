// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"

bool UMenu::Initialize()
{
	bIsFocusable = true;
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this,&UMenu::HostButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this,&UMenu::JoinButtonClicked);
	}
	
	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::MenuSetup(int32 NumberOfPublicConnections,FString TypeOfMatch)
{
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	//bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputData;

			InputData.SetWidgetToFocus(TakeWidget());
			InputData.SetWidgetToFocus(TakeWidget());
			InputData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputData);
			PlayerController->SetShowMouseCursor(true);

			SetKeyboardFocus();
		}
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
		
	}
}

void UMenu::HostButtonClicked()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("Host Button Clicked"))
		);	
	}

	if (MultiplayerSessionSubsystem)
	{
		MultiplayerSessionSubsystem->CreateSession(NumPublicConnections,MatchType);
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel("/Game/ThirdPerson/Maps/Lobby");
			
		}
	}
	
}

void UMenu::JoinButtonClicked()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString::Printf(TEXT("Join Button Clicked"))
		);	
	}

	
	
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputData;
			
			PlayerController->SetInputMode(InputData);
			PlayerController->SetShowMouseCursor(false);
			
		}
	}
}
