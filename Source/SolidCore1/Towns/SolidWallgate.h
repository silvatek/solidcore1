#pragma once

#include "CoreMinimal.h"

class USolidWorldChart;

/**
 * A WallGate town is a stone wall across a gap in the mountains, not a pack of houses.
 * The marker stands on one terrain point. The wall runs east and west from that point
 * across town and grassland until a mountain, and the towers stand on its north face.
 * Both towers are the same height, and that height is above the wall.
 */
namespace SolidWallgate
{
	inline constexpr float GapCm = 400.f;
	inline constexpr float WallThicknessCm = 200.f;
	/** The curtain is 4 m high. */
	inline constexpr float WallHeightCm = 400.f;
	/** How far each tower sinks into the wall so the two read as one structure. */
	inline constexpr float ConnectCm = 40.f;

	/** Same footprint as the default town building. The body rises above the wall. */
	inline constexpr float TowerFootprintXCm = 280.f;
	inline constexpr float TowerFootprintYCm = 240.f;
	/** Body above the curtain, before the roof, so the tower reads as taller. */
	inline constexpr float TowerAboveWallCm = 160.f;
	inline constexpr float TowerBodyCm = WallHeightCm + TowerAboveWallCm;
	inline constexpr float TowerRoofCm = 140.f;

	static_assert(TowerBodyCm > WallHeightCm, "Gate towers stand above the wall.");

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
	 * The marker the gate stands on. Several markers of one wall gate use the eastern one.
	 * False when that id is not a wall gate, or it has no marker.
	 */
	bool MarkerCell(const USolidWorldChart* WorldMap, int32 TownId, FIntPoint& OutCell);

	/**
	 * Two wall lengths and two towers for each WallGate town on the map.
	 * A run that is too narrow, or that has no mountain to the east or the west, writes nothing.
	 * @return Number of pieces written.
	 */
	int32 CollectPieces(
		const USolidWorldChart* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		TArray<FPiece>& OutPieces);

	/** The 4 m gap, on the marker the gate uses. False when that id is not a wall gate, or it has no marker. */
	bool GateCenterXY(
		const USolidWorldChart* WorldMap,
		int32 TownId,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		FVector2D& OutCenter);
}
