#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"

/**
 * Dungeon interior, traveled to from the open world.
 * Every dungeon size uses this trip. The Meagre size is the cave.
 * F10 enters and leaves. A left click on the dark opening does the same.
 * Only the captain makes the trip. Companions are removed with the
 * open world and spawned again when he comes back.
 */
namespace SolidDungeonLevel
{
	inline const TCHAR* EnterLabel = TEXT("Enter cave");
	inline const TCHAR* LeaveLabel = TEXT("Leave cave");

	/** Meagre ceiling. */
	inline constexpr float RoomHeightCm = 320.f;
	inline constexpr float ShellCm = 48.f;
	/** Dark opening on the entrance hex. The rock mouth outside is larger. */
	inline constexpr float MouthHeightCm = 240.f;
	/**
	 * Clear width of the cave door. Wider than the captain, and wider than one hex edge,
	 * so the vestibule is not pinched shut by the wall ends.
	 */
	inline constexpr float DoorwayWidthCm = FSolidStronghold::OpeningCm;

	/**
	 * Meagre floor, mouth to the west. Fourteen cells, with a vestibule on the entrance.
	 *     (-1,1) (0,1) (1,1) (2,1)
	 * (-2,0) (-1,0) (0,0) (1,0) (2,0)
	 *        (0,-1) (1,-1) (2,-1)
	 *           (0,-2) (1,-2)
	 */
	inline void MeagreCells(TArray<SolidStrongholdGrid::FCell>& OutCells)
	{
		OutCells.Reset();
		const SolidStrongholdGrid::FCell Cells[] = {
			{ -2, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 }, { 2, 0 },
			{ -1, 1 }, { 0, 1 }, { 1, 1 }, { 2, 1 },
			{ 0, -1 }, { 1, -1 }, { 2, -1 },
			{ 0, -2 }, { 1, -2 },
		};
		for (const SolidStrongholdGrid::FCell& Cell : Cells)
		{
			OutCells.Add(Cell);
		}
	}

	inline SolidStrongholdGrid::FCell MeagreEntranceCell()
	{
		return SolidStrongholdGrid::FCell{ -2, 0 };
	}

	inline SolidStrongholdGrid::FCell MeagreCaptainCell()
	{
		return SolidStrongholdGrid::FCell{ -1, 0 };
	}

	/** Cell coordinates are measured from here. */
	inline FVector2D MeagreOrigin()
	{
		return FVector2D::ZeroVector;
	}

	inline void CollectMeagreHexes(TArray<SolidStrongholdGrid::FHex>& OutHexes)
	{
		OutHexes.Reset();
		TArray<SolidStrongholdGrid::FCell> Cells;
		MeagreCells(Cells);
		const FVector2D Origin = MeagreOrigin();
		for (const SolidStrongholdGrid::FCell& Cell : Cells)
		{
			SolidStrongholdGrid::FHex Hex;
			Hex.Center = SolidStrongholdGrid::HexCenter(Cell.Col, Cell.Row, Origin);
			Hex.Col = Cell.Col;
			Hex.Row = Cell.Row;
			OutHexes.Add(Hex);
		}
	}

	/** Midpoint of the entrance hex's west flat, where the mouth sits. */
	inline FVector2D MeagreMouthMidpoint()
	{
		const SolidStrongholdGrid::FCell Entrance = MeagreEntranceCell();
		const FVector2D Center = SolidStrongholdGrid::HexCenter(Entrance.Col, Entrance.Row, MeagreOrigin());
		return Center + FVector2D(-SolidStrongholdGrid::FlatToFlatCm * 0.5f, 0.f);
	}

	/** Feet on the cell inside the entrance. The visit view turns him into the cave. */
	inline FVector CaptainSpawnLocal()
	{
		const SolidStrongholdGrid::FCell Cell = MeagreCaptainCell();
		const FVector2D Center = SolidStrongholdGrid::HexCenter(Cell.Col, Cell.Row, MeagreOrigin());
		return FVector(Center.X, Center.Y, 0.f);
	}

	/** Local +X runs away from the mouth, into the cave. Yaw 180 would look back at the mouth. */

	inline bool ContainsLocal(const FVector& Point)
	{
		if (Point.Z < 0.f || Point.Z >= RoomHeightCm)
		{
			return false;
		}
		TArray<SolidStrongholdGrid::FHex> Hexes;
		CollectMeagreHexes(Hexes);
		const FVector2D Plan(Point.X, Point.Y);
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			if (SolidStrongholdGrid::PointInHex(Plan, Hex.Center))
			{
				return true;
			}
		}
		return false;
	}
}
