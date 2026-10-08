#include "SolidCore1GameMode.h"
#include "SolidCore1Character.h"
#include "SolidCore1PlayerController.h"
#include "SolidCore1.h"
#include "Terrain/SolidCore1TerrainStreamer.h"
#include "UObject/ConstructorHelpers.h"

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
	bAutoSpawnTerrainStreamer = true;

	if (UClass* PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Characters/BP_SolidCore1Character.BP_SolidCore1Character_C")))
	{
		DefaultPawnClass = PawnClass;
	}
	else if (UClass* PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Blueprints/BP_SolidCore1Character.BP_SolidCore1Character_C")))
	{
		DefaultPawnClass = PawnClass;
	}
	else if (UClass* PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C")))
	{
		DefaultPawnClass = PawnClass;
	}
	else
	{
		DefaultPawnClass = ASolidCore1Character::StaticClass();
		UE_LOG(LogSolidCore1, Warning,
			TEXT("SolidCore1GameMode falling back to C++ SolidCore1Character. "
				 "Create /Game/Characters/BP_SolidCore1Character with SKM_Manny_Simple assigned, "
				 "or set Default Pawn via BP_SolidCore1GameMode in Project Settings."));
	}
}

void ASolidCore1GameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureTerrainStreamer();
}

void ASolidCore1GameMode::EnsureTerrainStreamer()
{
	if (!bAutoSpawnTerrainStreamer)
	{
		return;
	}

	ASolidCore1TerrainStreamer::EnsureExists(GetWorld());
}
