#pragma once

#include "CoreMinimal.h"

/**
 * Canonical Content Blueprint soft-class paths for pawn / GameMode resolution.
 * Content assets: /Game/Characters/BP_SolidCharacter and BP_SolidGameMode.
 */
namespace SolidContentPaths
{
	/** Null-terminated list of pawn Blueprint class paths. */
	inline const TCHAR* const* PawnBlueprintClasses()
	{
		static const TCHAR* Paths[] = {
			TEXT("/Game/Characters/BP_SolidCharacter.BP_SolidCharacter_C"),
			nullptr
		};
		return Paths;
	}

	/** Null-terminated list of GameMode Blueprint class paths. */
	inline const TCHAR* const* GameModeBlueprintClasses()
	{
		static const TCHAR* Paths[] = {
			TEXT("/Game/Characters/BP_SolidGameMode.BP_SolidGameMode_C"),
			nullptr
		};
		return Paths;
	}

	/** Load the first class in Paths that resolves; nullptr if none. */
	template <typename T>
	UClass* LoadFirstClass(const TCHAR* const* Paths)
	{
		for (int32 Index = 0; Paths[Index] != nullptr; ++Index)
		{
			if (UClass* Loaded = LoadClass<T>(nullptr, Paths[Index]))
			{
				return Loaded;
			}
		}
		return nullptr;
	}
}
