#pragma once

#include "CoreMinimal.h"

class USolidWorldMap;

/**
 * Dragongate (WorldMap digit 5) is a stone wall across its cells, not a pack of houses.
 * The wall runs between the mountain sides. A 4 m gap is the gate. A normal building
 * stands each side of that gap, on the north face of an east-west wall (the east face
 * when the pass runs north-south). Both towers use the same height.
 */
namespace SolidDragongate
{
	inline constexpr float GapCm = 400.f;
	inline constexpr float WallThicknessCm = 200.f;
	inline constexpr float WallHeightCm = 800.f;
	/** How far each tower sinks into the wall so the two read as one structure. */
	inline constexpr float ConnectCm = 40.f;

	/** Same footprint and height as the default town building, so the towers match. */
	inline constexpr float TowerFootprintXCm = 280.f;
	inline constexpr float TowerFootprintYCm = 240.f;
	inline constexpr float TowerBodyCm = 320.f;
	inline constexpr float TowerRoofCm = 140.f;

	inline FLinearColor StoneColor()
	{
		return FLinearColor(0.55f, 0.53f, 0.50f);
	}

	struct FPiece
	{
		FVector2D CenterXY = FVector2D::ZeroVector;
		float FootprintXCm = 0.f;
		float FootprintYCm = 0.f;
		float BodyHeightCm = 0.f;
		float RoofHeightCm = 0.f;
		float YawDeg = 0.f;
		bool bStoneWall = false;
	};

	/**
	 * Two wall lengths and two towers, or nothing when digit 5 is not on the map.
	 * @return Number of pieces written.
	 */
	int32 CollectPieces(
		const USolidWorldMap* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		TArray<FPiece>& OutPieces);
}
