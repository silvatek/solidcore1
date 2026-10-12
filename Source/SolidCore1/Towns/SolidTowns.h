#pragma once

#include "CoreMinimal.h"

/**
 * Town attributes, keyed by the WorldMap.txt location index.
 * The map file keeps the digit, role, and biome. The name lives here.
 */
namespace SolidTowns
{
	struct FTownInfo
	{
		int32 Id = INDEX_NONE;
		const TCHAR* Name = TEXT("");
	};

	inline const FTownInfo Definitions[] = {
		{ 0, TEXT("Iglin") },
		{ 1, TEXT("Relion") },
		{ 2, TEXT("Kanfold") },
		{ 3, TEXT("Visolar") },
		{ 4, TEXT("Jethan") },
		{ 5, TEXT("Dragongate") },
		{ 6, TEXT("Rathen") },
	};

	/** WorldMap digit for the walled pass. Houses are not scattered on its cells. */
	inline constexpr int32 DragongateId = 5;

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	inline const FTownInfo* Find(int32 Id)
	{
		for (const FTownInfo& Town : Definitions)
		{
			if (Town.Id == Id)
			{
				return &Town;
			}
		}
		return nullptr;
	}

	/** Empty when this index has no town definition. */
	inline FString NameFor(int32 Id)
	{
		if (const FTownInfo* Town = Find(Id))
		{
			return FString(Town->Name);
		}
		return FString();
	}
}
