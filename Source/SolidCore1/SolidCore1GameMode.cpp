#include "SolidCore1GameMode.h"
#include "SolidCore1Character.h"
#include "SolidCore1PlayerController.h"
#include "SolidCore1.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

namespace SolidCore1GameModePrivate
{
	static UClass* TryLoadPawnClass(const TCHAR* Path)
	{
		if (UClass* Loaded = LoadClass<APawn>(nullptr, Path))
		{
			UE_LOG(LogSolidCore1, Warning, TEXT("SolidCore1GameMode using pawn class: %s"), *Loaded->GetPathName());
			return Loaded;
		}
		return nullptr;
	}
}

ASolidCore1GameMode::ASolidCore1GameMode()
{
	PlayerControllerClass = ASolidCore1PlayerController::StaticClass();

	// Prefer Blueprint pawns (mesh/anim assigned in Content). Try common paths.
	if (UClass* PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Characters/BP_SolidCore1Character.BP_SolidCore1Character_C")))
	{
		DefaultPawnClass = PawnClass;
		return;
	}

	if (UClass* PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Blueprints/BP_SolidCore1Character.BP_SolidCore1Character_C")))
	{
		DefaultPawnClass = PawnClass;
		return;
	}

	if (UClass* PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")))
	{
		DefaultPawnClass = PawnClass;
		return;
	}

	DefaultPawnClass = ASolidCore1Character::StaticClass();
	UE_LOG(LogSolidCore1, Warning,
		TEXT("SolidCore1GameMode falling back to C++ SolidCore1Character. "
			 "Create /Game/Characters/BP_SolidCore1Character with SKM_Manny_Simple assigned, "
			 "or set Default Pawn via BP_SolidCore1GameMode in Project Settings."));
}
