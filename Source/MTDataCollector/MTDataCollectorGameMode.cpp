// Copyright Epic Games, Inc. All Rights Reserved.

#include "MTDataCollectorGameMode.h"
#include "MTDataCollectorCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMTDataCollectorGameMode::AMTDataCollectorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
