#pragma once

#include "CoreMinimal.h"

/**
 * Town attributes, keyed by the WorldMap.txt location index.
 * The map file keeps the digit, role, and biome. The name lives here.
 */
namespace SolidTowns
{
	struct FDefinition
	{
		int32 Id = INDEX_NONE;
		const TCHAR* Name = TEXT("");
	};

	inline const FDefinition Definitions[] = {
		{ 0, TEXT("Iglin") },
		{ 1, TEXT("Relion") },
		{ 2, TEXT("Kanfold") },
		{ 3, TEXT("Visolar") },
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	inline const FDefinition* Find(int32 Id)
	{
		for (const FDefinition& Town : Definitions)
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
		if (const FDefinition* Town = Find(Id))
		{
			return FString(Town->Name);
		}
		return FString();
	}
}
