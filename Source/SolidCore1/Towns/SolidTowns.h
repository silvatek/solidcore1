#pragma once

#include "CoreMinimal.h"

/**
 * Town attributes, keyed by the WorldMap.txt location index.
 * The map file keeps the digit, role, and biome. The name, type, and where
 * its welcome sign stands live here.
 */
namespace SolidTowns
{
	/** Actor yaw. The board's front faces south, toward someone standing south of the sign. */
	inline constexpr float SignFacesSouthYawDeg = -90.f;
	/** Actor yaw. The board's front faces north, so it can be read while facing south. */
	inline constexpr float SignFacesNorthYawDeg = 90.f;
	/**
	 * Dragongate's pole, on the north edge of its 16 m cell and one gate-width east of
	 * center. The board then sits off the opening, so the path through the wall stays clear.
	 */
	inline constexpr float DragongateSignNorthCm = 800.f;
	inline constexpr float DragongateSignEastCm = 400.f;

	enum class ETownType : uint8
	{
		Town,
		WallGate,
	};

	struct FTownInfo
	{
		int32 Id = INDEX_NONE;
		const TCHAR* Name = TEXT("");
		/** Centimetres from the cell center. +X is east, +Y is north. */
		FVector2D SignOffsetCm = FVector2D(0.f, 0.f);
		float SignYawDeg = SignFacesSouthYawDeg;
		ETownType Type = ETownType::Town;
	};

	inline const FTownInfo Definitions[] = {
		{ 0, TEXT("Iglin"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg, ETownType::Town },
		{ 1, TEXT("Relion"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg, ETownType::Town },
		{ 2, TEXT("Kanfold"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg, ETownType::Town },
		{ 3, TEXT("Visolar"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg, ETownType::Town },
		{ 4, TEXT("Jethan"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg, ETownType::Town },
		{ 5, TEXT("Dragongate"), FVector2D(DragongateSignEastCm, DragongateSignNorthCm), SignFacesNorthYawDeg, ETownType::WallGate },
		{ 6, TEXT("Rathen"), FVector2D(0.f, 0.f), SignFacesSouthYawDeg, ETownType::Town },
	};

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

	inline bool IsWallGate(int32 Id)
	{
		const FTownInfo* Town = Find(Id);
		return Town && Town->Type == ETownType::WallGate;
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
