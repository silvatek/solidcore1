#include "SolidWallgate.h"
#include "SolidTerrainTypes.h"
#include "SolidTowns.h"
#include "SolidWorldMap.h"

namespace
{
	bool WallgateNeighborIsMountain(const USolidWorldMap& Map, const int32 X, const int32 Y)
	{
		if (X < 0 || Y < 0 || X >= USolidWorldMap::MapWidth || Y >= USolidWorldMap::MapHeight)
		{
			return false;
		}
		return Map.GetBiomeAtCell(X, Y) == ESolidBiome::Mountain;
	}

	void WallgateAppendLocation(
		const USolidWorldMap& WorldMap,
		const FVector2D WorldMinXY,
		const FVector2D WorldMaxXY,
		const TArray<FIntPoint>& Cells,
		TArray<SolidWallgate::FPiece>& OutPieces)
	{
		int32 MountainEast = 0;
		int32 MountainWest = 0;
		int32 MountainNorth = 0;
		int32 MountainSouth = 0;
		for (const FIntPoint& Cell : Cells)
		{
			MountainNorth += WallgateNeighborIsMountain(WorldMap, Cell.X, Cell.Y - 1) ? 1 : 0;
			MountainEast += WallgateNeighborIsMountain(WorldMap, Cell.X - 1, Cell.Y) ? 1 : 0;
			MountainSouth += WallgateNeighborIsMountain(WorldMap, Cell.X, Cell.Y + 1) ? 1 : 0;
			MountainWest += WallgateNeighborIsMountain(WorldMap, Cell.X + 1, Cell.Y) ? 1 : 0;
		}

		// East and west mountains: the wall crosses the pass, and the towers stand north of it.
		// A north-south pass uses the other axis, with the towers on the east face.
		const bool bEastWest = (MountainEast + MountainWest) >= (MountainNorth + MountainSouth);
		const FVector2D Along = bEastWest ? FVector2D(1.f, 0.f) : FVector2D(0.f, 1.f);
		const FVector2D Side = bEastWest ? FVector2D(0.f, 1.f) : FVector2D(1.f, 0.f);

		const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
		const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
		const float CellW = ExtentX / static_cast<float>(USolidWorldMap::MapWidth);
		const float CellH = ExtentY / static_cast<float>(USolidWorldMap::MapHeight);
		const float HalfAlong = bEastWest ? CellW * 0.5f : CellH * 0.5f;

		float SpanMin = TNumericLimits<float>::Max();
		float SpanMax = TNumericLimits<float>::Lowest();
		float SidePos = 0.f;
		int32 Count = 0;
		for (const FIntPoint& Cell : Cells)
		{
			FVector2D Center = FVector2D::ZeroVector;
			if (!WorldMap.CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, Center))
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
			return;
		}
		SidePos /= static_cast<float>(Count);

		const float YawDeg = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
		const float Mid = (SpanMin + SpanMax) * 0.5f;
		const float Gap0 = Mid - SolidWallgate::GapCm * 0.5f;
		const float Gap1 = Mid + SolidWallgate::GapCm * 0.5f;

		auto AddWall = [&](const float From, const float To)
		{
			const float Length = To - From;
			if (Length < 1.f)
			{
				return;
			}
			SolidWallgate::FPiece Piece;
			Piece.CenterXY = Along * ((From + To) * 0.5f) + Side * SidePos;
			Piece.FootprintXCm = Length;
			Piece.FootprintYCm = SolidWallgate::WallThicknessCm;
			Piece.BodyHeightCm = SolidWallgate::WallHeightCm;
			Piece.RoofHeightCm = 0.f;
			Piece.YawDeg = YawDeg;
			Piece.bStoneWall = true;
			OutPieces.Add(Piece);
		};
		AddWall(SpanMin, Gap0);
		AddWall(Gap1, SpanMax);

		auto AddTower = [&](const float AlongPos)
		{
			SolidWallgate::FPiece Piece;
			const float SideOffset = SolidWallgate::WallThicknessCm * 0.5f
				+ SolidWallgate::TowerFootprintYCm * 0.5f
				- SolidWallgate::ConnectCm;
			Piece.CenterXY = Along * AlongPos + Side * (SidePos + SideOffset);
			Piece.FootprintXCm = SolidWallgate::TowerFootprintXCm;
			Piece.FootprintYCm = SolidWallgate::TowerFootprintYCm;
			Piece.BodyHeightCm = SolidWallgate::TowerBodyCm;
			Piece.RoofHeightCm = SolidWallgate::TowerRoofCm;
			Piece.YawDeg = YawDeg;
			Piece.bStoneWall = false;
			OutPieces.Add(Piece);
		};
		AddTower(Gap0 - SolidWallgate::TowerFootprintXCm * 0.5f);
		AddTower(Gap1 + SolidWallgate::TowerFootprintXCm * 0.5f);
	}
}

int32 SolidWallgate::CollectPieces(
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

	for (const SolidTowns::FTownInfo& Info : SolidTowns::Definitions)
	{
		if (Info.Type != SolidTowns::ETownType::WallGate)
		{
			continue;
		}

		FSolidWorldLocation Town;
		if (!WorldMap->FindLocation(Info.Id, Town) || Town.Cells.Num() == 0)
		{
			continue;
		}
		WallgateAppendLocation(*WorldMap, WorldMinXY, WorldMaxXY, Town.Cells, OutPieces);
	}
	return OutPieces.Num();
}
