#pragma once

#include "CoreMinimal.h"
#include "SolidStronghold.h"

class USolidWorldMap;

/**
 * Where each stronghold meets the open world.
 * Pure logic — no actor spawning (safe for automation).
 * The cave interior is a later level; this only places the outside mouth.
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
		ESolidStronghold Kind = ESolidStronghold::Cave;
		/** Cave (or later stronghold) map cell. */
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
	 * @return Number of entrances written.
	 */
	int32 CollectEntrances(
		const USolidWorldMap* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		TArray<FEntrance>& OutEntrances);
}
