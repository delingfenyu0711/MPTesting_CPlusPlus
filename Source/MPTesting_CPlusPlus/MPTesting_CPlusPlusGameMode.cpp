// Copyright Epic Games, Inc. All Rights Reserved.

#include "MPTesting_CPlusPlusGameMode.h"
#include "MPTesting_CPlusPlusCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMPTesting_CPlusPlusGameMode::AMPTesting_CPlusPlusGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
