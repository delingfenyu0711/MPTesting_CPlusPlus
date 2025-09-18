// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPTesting_CPlusPlusCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMPTesting_CPlusPlusCharacter

AMPTesting_CPlusPlusCharacter::AMPTesting_CPlusPlusCharacter():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this,&ThisClass::OnFindSessionComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		if (OnlineSessionInterface.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Online session interface is valid"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Online session interface is null"));
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString::Printf(TEXT("Found subsystem %s"),*OnlineSubsystem->GetSubsystemName().ToString())
				);
		}
	}
}

void AMPTesting_CPlusPlusCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 确保使用正确的在线子系统
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
		UE_LOG(LogTemp, Warning, TEXT("Using OnlineSubsystem: %s"), *SubsystemName);

		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		
		if (OnlineSessionInterface.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Online session interface is valid"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Online session interface is invalid"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OnlineSubsystem is null"));
	}

	// 调试登录状态
	DebugLoginStatus();
}

FUniqueNetIdRepl AMPTesting_CPlusPlusCharacter::GetPlayerNetId() const
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

void AMPTesting_CPlusPlusCharacter::CreateGameSession()
{
    if (!OnlineSessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OnlineSessionInterface is not valid"));
        return;
    }

    // 检查登录状态
    DebugLoginStatus();

    // 销毁现有会话
    auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        OnlineSessionInterface->DestroySession(NAME_GameSession);
    }

    OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

    TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
    SessionSettings->bIsLANMatch = false;
    SessionSettings->NumPublicConnections = 4; // 使用PublicConnections而不是PrivateConnections
    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->bAllowJoinViaPresence = true;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bUsesPresence = true;
    SessionSettings->bUseLobbiesIfAvailable = true;
    SessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    // 使用统一的网络ID获取方法
    FUniqueNetIdRepl NetId = GetPlayerNetId();
    if (!NetId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create session: Invalid NetId"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("Cannot create session: Player not logged in"));
        }
        return;
    }

	if (const FUniqueNetId* RawNetId = NetId.GetUniqueNetId().Get())
	{
		FString NetIdStr = RawNetId->ToString();
		UE_LOG(LogTemp, Warning, TEXT("NetId: %s"), *NetIdStr);
	}
    OnlineSessionInterface->CreateSession(*NetId, NAME_GameSession, *SessionSettings);
}

void AMPTesting_CPlusPlusCharacter::JoinGameSession()
{
    if (!OnlineSessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OnlineSessionInterface is not valid"));
        return;
    }

    // 检查登录状态
    DebugLoginStatus();

    OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    if (!SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create SessionSearch"));
        return;
    }

    SessionSearch->MaxSearchResults = 10000;
    SessionSearch->bIsLanQuery = false;
    SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

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
    OnlineSessionInterface->FindSessions(*NetId, SessionSearch.ToSharedRef());
}

void AMPTesting_CPlusPlusCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        // 会话创建成功
        UE_LOG(LogTemp, Warning, TEXT("Session created successfully: %s"), *SessionName.ToString());
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Blue,
                FString::Printf(TEXT("Created session: %s"), *SessionName.ToString())
            );
        }

        UWorld* World = GetWorld();
        if (World)
        {
            World->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
        }
    }
    else
    {
        // 会话创建失败
        UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Red,
                TEXT("Failed to create session")
            );
        }
    }
}

void AMPTesting_CPlusPlusCharacter::OnFindSessionComplete(bool bWasSuccessful)
{
    if (!OnlineSessionInterface.IsValid() || !SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid interfaces in OnFindSessionComplete"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Find sessions completed. Successful: %d, Results: %d"), 
        bWasSuccessful, SessionSearch->SearchResults.Num());

    if (!bWasSuccessful || SessionSearch->SearchResults.Num() == 0)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, 
                FString::Printf(TEXT("No sessions found or search failed. Results: %d"), 
                SessionSearch->SearchResults.Num()));
        }
        return;
    }

    for (auto Result : SessionSearch->SearchResults)
    {
        FString Id = Result.GetSessionIdStr();
        FString User = Result.Session.OwningUserName;
        FString MatchType;
        Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);
        
        UE_LOG(LogTemp, Warning, TEXT("Found session - ID: %s, User: %s, MatchType: %s"), 
            *Id, *User, *MatchType);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan,
                FString::Printf(TEXT("Session: %s, User: %s, MatchType: %s"), 
                *Id, *User, *MatchType));
        }

        if (MatchType.Equals(TEXT("FreeForAll")))
        {
            UE_LOG(LogTemp, Warning, TEXT("Attempting to join matching session"));
            
            FUniqueNetIdRepl NetId = GetPlayerNetId();
            if (!NetId.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot join: Invalid NetId"));
                continue;
            }

            OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
            OnlineSessionInterface->JoinSession(*NetId, NAME_GameSession, Result);
            break;
        }
    }
}

void AMPTesting_CPlusPlusCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (!OnlineSessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OnlineSessionInterface is not valid"));
        return;
    }

    FString Address;
    if (OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
    {
        UE_LOG(LogTemp, Warning, TEXT("Resolved connect string: %s"), *Address);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Yellow,
                FString::Printf(TEXT("Connect String: %s"), *Address)
            );
        }

        APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
        if (PlayerController)
        {
            PlayerController->ClientTravel(Address, TRAVEL_Absolute);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get resolved connect string"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.f,
                FColor::Red,
                TEXT("Failed to get connect string")
            );
        }
    }
}

FString AMPTesting_CPlusPlusCharacter::NetIdToString(const FUniqueNetIdRepl& NetId) const
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


void AMPTesting_CPlusPlusCharacter::DebugLoginStatus()
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    if (!OnlineSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("No OnlineSubsystem found"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("OnlineSubsystem: %s"), *OnlineSubsystem->GetSubsystemName().ToString());

    IOnlineIdentityPtr IdentityInterface = OnlineSubsystem->GetIdentityInterface();
    if (!IdentityInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Identity interface not valid"));
        return;
    }

    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if (!LocalPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("No local player found"));
        return;
    }

    int32 ControllerId = LocalPlayer->GetControllerId();
    ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(ControllerId);
    
    FString StatusStr;
    switch (LoginStatus)
    {
        case ELoginStatus::NotLoggedIn: StatusStr = TEXT("Not Logged In"); break;
        case ELoginStatus::UsingLocalProfile: StatusStr = TEXT("Using Local Profile"); break;
        case ELoginStatus::LoggedIn: StatusStr = TEXT("Logged In"); break;
        default: StatusStr = TEXT("Unknown Status"); break;
    }

    UE_LOG(LogTemp, Warning, TEXT("Login Status: %s"), *StatusStr);

    // 获取并显示用户ID
    TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(ControllerId);
    if (UserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("User ID: %s"), *UserId->ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid User ID"));
    }

    // 显示在屏幕上
    if (GEngine)
    {
        FString Message = FString::Printf(TEXT("Login Status: %s\nUser ID: %s"), 
            *StatusStr, 
            UserId.IsValid() ? *UserId->ToString() : TEXT("Invalid"));
        
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, Message);
    }
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMPTesting_CPlusPlusCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMPTesting_CPlusPlusCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMPTesting_CPlusPlusCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMPTesting_CPlusPlusCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMPTesting_CPlusPlusCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}