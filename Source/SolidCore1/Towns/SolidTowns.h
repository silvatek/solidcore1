#pragma once

#include "CoreMinimal.h"

/**
 * Town attributes, keyed by the WorldMap.txt location index.
 * The map file keeps the digit, role, and biome. The name, and where its
 * welcome sign stands, live here.
 */
namespace SolidTowns
{
	/** Actor yaw. The board's front faces south, toward someone standing south of the sign. */
	inline constexpr float SignFacesSouthYawDeg = -90.f;
	/** Actor yaw. The board's front faces north, so it can be read while facing south. */
	inline constexpr float SignFacesNorthYawDeg = 90.f;
	/**
	 * Dragongate's pole, north of the cell center. The gate is that center.
	 * 400 cm clears the towers on the north face of the wall and stays inside the cell.
	 */
	inline constexpr float DragongateSignNorthCm = 400.f;

	struct FTownInfo
	{
		int32 Id = INDEX_NONE;
		const TCHAR* Name = TEXT("");
		/** Centimetres from the cell center. +X is east, +Y is north. */
		FVector2D SignOffsetCm = FVector2D(0.f, 0.f);
		float SignYawDeg = SignFacesSouthYawDeg;
	};

	inline const FTownInfo Definitions[] = {
		{ 0, TEXT("Iglin"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg },
		{ 1, TEXT("Relion"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg },
		{ 2, TEXT("Kanfold"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg },
		{ 3, TEXT("Visolar"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg },
		{ 4, TEXT("Jethan"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg },
		{ 5, TEXT("Dragongate"), FVector2D(0.f, DragongateSignNorthCm), SignFacesNorthYawDeg },
		{ 6, TEXT("Rathen"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg },
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
