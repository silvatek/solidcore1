#include "SolidDragongate.h"
#include "SolidTerrainTypes.h"
#include "SolidTowns.h"
#include "SolidWorldMap.h"

namespace
{
	bool DragongateNeighborIsMountain(const USolidWorldMap& Map, const int32 X, const int32 Y)
	{
		if (X < 0 || Y < 0 || X >= USolidWorldMap::MapWidth || Y >= USolidWorldMap::MapHeight)
		{
			return false;
		}
		return Map.GetBiomeAtCell(X, Y) == ESolidBiome::Mountain;
	}
}

int32 SolidDragongate::CollectPieces(
	const USolidWorldMap* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	TArray<FPiece>& OutPieces)
{
	OutPieces.Reset();
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return 0;
	}

	FSolidWorldLocation Town;
	if (!WorldMap->FindLocation(SolidTowns::DragongateId, Town) || Town.Cells.Num() == 0)
	{
		return 0;
	}

	int32 MountainEast = 0;
	int32 MountainWest = 0;
	int32 MountainNorth = 0;
	int32 MountainSouth = 0;
	for (const FIntPoint& Cell : Town.Cells)
	{
		MountainNorth += DragongateNeighborIsMountain(*WorldMap, Cell.X, Cell.Y - 1) ? 1 : 0;
		MountainEast += DragongateNeighborIsMountain(*WorldMap, Cell.X - 1, Cell.Y) ? 1 : 0;
		MountainSouth += DragongateNeighborIsMountain(*WorldMap, Cell.X, Cell.Y + 1) ? 1 : 0;
		MountainWest += DragongateNeighborIsMountain(*WorldMap, Cell.X + 1, Cell.Y) ? 1 : 0;
	}

	// East and west mountains: the wall crosses the pass, and the towers stand north of it.
	// A north-south pass uses the other axis, with the towers on the east face.
	const bool bEastWest = (MountainEast + MountainWest) >= (MountainNorth + MountainSouth);
	const FVector2D Along = bEastWest ? FVector2D(1.f, 0.f) : FVector2D(0.f, 1.f);
	FVector2D Side = bEastWest ? FVector2D(0.f, 1.f) : FVector2D(1.f, 0.f);

	const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
	const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
	const float CellW = ExtentX / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellH = ExtentY / static_cast<float>(USolidWorldMap::MapHeight);
	const float HalfAlong = bEastWest ? CellW * 0.5f : CellH * 0.5f;

	float SpanMin = TNumericLimits<float>::Max();
	float SpanMax = TNumericLimits<float>::Lowest();
	float SidePos = 0.f;
	int32 Count = 0;
	for (const FIntPoint& Cell : Town.Cells)
	{
		FVector2D Center = FVector2D::ZeroVector;
		if (!WorldMap->CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, Center))
		{
			continue;
		}
		const float AlongPos = FVector2D::DotProduct(Center, Along);
		SpanMin = FMath::Min(SpanMin, AlongPos - HalfAlong);
		SpanMax = FMath::Max(SpanMax, AlongPos + HalfAlong);
		SidePos += FVector2D::DotProduct(Center, Side);
		++Count;
	}
	if (Count == 0 || SpanMax <= SpanMin)
	{
		return 0;
	}
	SidePos /= static_cast<float>(Count);

	const float YawDeg = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
	const float Mid = (SpanMin + SpanMax) * 0.5f;
	const float Gap0 = Mid - GapCm * 0.5f;
	const float Gap1 = Mid + GapCm * 0.5f;

	auto AddWall = [&](const float From, const float To)
	{
		const float Length = To - From;
		if (Length < 1.f)
		{
			return;
		}
		FPiece Piece;
		Piece.CenterXY = Along * ((From + To) * 0.5f) + Side * SidePos;
		Piece.FootprintXCm = Length;
		Piece.FootprintYCm = WallThicknessCm;
		Piece.BodyHeightCm = WallHeightCm;
		Piece.RoofHeightCm = 0.f;
		Piece.YawDeg = YawDeg;
		Piece.bStoneWall = true;
		OutPieces.Add(Piece);
	};
	AddWall(SpanMin, Gap0);
	AddWall(Gap1, SpanMax);

	auto AddTower = [&](const float AlongPos)
	{
		FPiece Piece;
		const float SideOffset = WallThicknessCm * 0.5f + TowerFootprintYCm * 0.5f - ConnectCm;
		Piece.CenterXY = Along * AlongPos + Side * (SidePos + SideOffset);
		Piece.FootprintXCm = TowerFootprintXCm;
		Piece.FootprintYCm = TowerFootprintYCm;
		Piece.BodyHeightCm = TowerBodyCm;
		Piece.RoofHeightCm = TowerRoofCm;
		Piece.YawDeg = YawDeg;
		Piece.bStoneWall = false;
		OutPieces.Add(Piece);
	};
	AddTower(Gap0 - TowerFootprintXCm * 0.5f);
	AddTower(Gap1 + TowerFootprintXCm * 0.5f);

	return OutPieces.Num();
}
