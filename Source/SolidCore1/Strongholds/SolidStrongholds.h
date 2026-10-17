#pragma once

#include "CoreMinimal.h"
#include "SolidStronghold.h"

class USolidWorldMap;

/**
 * Where each stronghold meets the open world.
 * Pure logic — no actor spawning (safe for automation).
 * The dungeon interior is a separate visit; this only places the outside mouth.
 */
namespace SolidStrongholds
{
	/**
	 * How far the mouth stands from the shared cell edge, toward the grass (cm).
	 * Terrain points are 200 cm apart and the mountain shelf is the point on
	 * the cave side of that edge. 280 cm clears the slope so the sill sits on
	 * the grassland and the back of the frame meets the cliff.
	 */
	inline constexpr float FaceInsetCm = 280.f;

	inline constexpr float OpeningWidthCm = 380.f;
	inline constexpr float OpeningHeightCm = 420.f;
	inline constexpr float OpeningDepthCm = 240.f;
	/**
	 * Grass-facing face of the rock frame, in actor space (cm).
	 * Local +X faces the grass. The face sits toward the cliff so the actor
	 * origin, where the captain stands, is in front of the stone.
	 */
	inline constexpr float FrameFrontLocalX = -OpeningDepthCm * 0.5f;
	inline constexpr float FrameThicknessCm = 80.f;
	/** Black mouth sits this far behind the rock front so the faces do not z-fight. */
	inline constexpr float MouthRecessCm = 16.f;
	/** Side gap so the mouth does not share a face with the jambs. */
	inline constexpr float MouthGapCm = 6.f;

	inline FLinearColor RockColor()
	{
		return FLinearColor(0.30f, 0.28f, 0.26f);
	}

	inline FLinearColor MouthColor()
	{
		return FLinearColor(0.015f, 0.012f, 0.01f);
	}

	struct FEntrance
	{
		FSolidStronghold Stronghold;
		/** Stronghold map cell. */
		FIntPoint Cell = FIntPoint::ZeroValue;
		/** Adjacent grassland the mouth faces. */
		FIntPoint GrassCell = FIntPoint::ZeroValue;
		/** Midpoint of the shared edge. */
		FVector2D EdgeXY = FVector2D::ZeroVector;
		/** Spawn point on the grass side of that edge. */
		FVector2D WorldXY = FVector2D::ZeroVector;
		/** Local +X faces the grassland. */
		float FacingYawDeg = 0.f;
	};

	/**
	 * One entrance per stronghold cell that shares an edge with grassland.
	 * Neighbors are tried north, east, south, then west; the first grassland wins.
	 * Ordered by map row, then column.
	 * A tower is included here. Only a dungeon spawns the cliff mouth.
	 * A tower with a plan spawns its shell at the cell center, turned so the door faces this grass.
	 * A click on that door enters. Leaving stands on the grass outside the same door.
	 * @return Number of entrances written.
	 */
	int32 CollectEntrances(
		const USolidWorldMap* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		TArray<FEntrance>& OutEntrances);

	/** Grass neighbor of one map cell. North, then west, south, and east, matching CollectEntrances. */
	struct FGrassFace
	{
		FIntPoint GrassCell = FIntPoint::ZeroValue;
		float FacingYawDeg = 0.f;
	};

	bool FindGrassFace(const USolidWorldMap* WorldMap, FIntPoint Cell, FGrassFace& Out);

	/** The first dungeon mouth. A tower entrance is not a place to leave the cave. */
	inline const FEntrance* FindCaveMouth(const TArray<FEntrance>& Entrances)
	{
		for (const FEntrance& Entrance : Entrances)
		{
			if (SolidStronghold::UsesCaveMouth(Entrance.Stronghold))
			{
				return &Entrance;
			}
		}
		return nullptr;
	}
}
