#include "SolidWallgate.h"
#include "SolidTerrainTypes.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

namespace
{
	/** Points at 2 m and 4 m have to be open, because the gate reaches 4.8 m from the marker. */
	constexpr int32 WallgateOpenSteps = 2;

	bool WallgateOpen(const ESolidBiome Biome)
	{
		return Biome == ESolidBiome::Grassland || Biome == ESolidBiome::Town;
	}

	float WallgateSpacingCm(const FVector2D WorldMinXY, const FVector2D WorldMaxXY)
	{
		const float Extent = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
		return Extent / static_cast<float>(USolidWorldChart::PointAxis - 1);
	}

	/**
	 * Walk one point at a time until a mountain. The first two steps are the ground the gate needs.
	 * Anything else, or the edge of the grid, leaves this side without a mountain.
	 */
	bool WallgateMountain(
		const USolidWorldChart& WorldMap,
		const FIntPoint& Marker,
		const int32 StepX,
		const FVector2D WorldMinXY,
		const FVector2D WorldMaxXY,
		FVector2D& OutMountain)
	{
		for (int32 Step = 1; Step < USolidWorldChart::PointAxis; ++Step)
		{
			const FIntPoint TerrainPoint(Marker.X + Step * StepX, Marker.Y);
			if (TerrainPoint.X < 0 || TerrainPoint.Y < 0
				|| TerrainPoint.X >= USolidWorldChart::PointAxis
				|| TerrainPoint.Y >= USolidWorldChart::PointAxis)
			{
				return false;
			}

			const ESolidBiome Ground = WorldMap.GetPointBiome(TerrainPoint);
			if (Step <= WallgateOpenSteps)
			{
				if (!WallgateOpen(Ground))
				{
					return false;
				}
				continue;
			}
			if (WallgateOpen(Ground))
			{
				continue;
			}
			if (Ground != ESolidBiome::Mountain)
			{
				return false;
			}
			return WorldMap.TerrainPointToWorld(TerrainPoint, WorldMinXY, WorldMaxXY, OutMountain);
		}
		return false;
	}

	void WallgateAppendRun(
		const USolidWorldChart& WorldMap,
		const FVector2D WorldMinXY,
		const FVector2D WorldMaxXY,
		const FVector2D& Marker,
		TArray<SolidWallgate::FPiece>& OutPieces)
	{
		FIntPoint MarkerTerrainPoint = FIntPoint::ZeroValue;
		if (!WorldMap.TerrainPointAt(Marker.X, Marker.Y, WorldMinXY, WorldMaxXY, MarkerTerrainPoint))
		{
			return;
		}

		FVector2D MountainEast = FVector2D::ZeroVector;
		FVector2D MountainWest = FVector2D::ZeroVector;
		if (!WallgateMountain(WorldMap, MarkerTerrainPoint, 1, WorldMinXY, WorldMaxXY, MountainEast)
			|| !WallgateMountain(WorldMap, MarkerTerrainPoint, -1, WorldMinXY, WorldMaxXY, MountainWest))
		{
			return;
		}

		// One point past the mountain, so the curtain crosses the slope and ends inside it.
		const float Spacing = WallgateSpacingCm(WorldMinXY, WorldMaxXY);
		const float SpanMax = MountainEast.X + Spacing;
		const float SpanMin = MountainWest.X - Spacing;
		const float Gap0 = Marker.X - SolidWallgate::GapCm * 0.5f;
		const float Gap1 = Marker.X + SolidWallgate::GapCm * 0.5f;
		if (SpanMin + 1.f >= Gap0 || SpanMax - 1.f <= Gap1)
		{
			return;
		}

		const float SidePos = Marker.Y;
		auto AddWall = [&](const float From, const float To)
		{
			const float Length = To - From;
			if (Length < 1.f)
			{
				return;
			}
			SolidWallgate::FPiece Piece;
			Piece.CenterXY = FVector2D((From + To) * 0.5f, SidePos);
			Piece.MarkerXY = Marker;
			Piece.FootprintXCm = Length;
			Piece.FootprintYCm = SolidWallgate::WallThicknessCm;
			Piece.BodyHeightCm = SolidWallgate::WallHeightCm;
			Piece.RoofHeightCm = 0.f;
			Piece.YawDeg = 0.f;
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
			Piece.CenterXY = FVector2D(AlongPos, SidePos + SideOffset);
			Piece.MarkerXY = Marker;
			Piece.FootprintXCm = SolidWallgate::TowerFootprintXCm;
			Piece.FootprintYCm = SolidWallgate::TowerFootprintYCm;
			Piece.BodyHeightCm = SolidWallgate::TowerBodyCm;
			Piece.RoofHeightCm = SolidWallgate::TowerRoofCm;
			Piece.YawDeg = 0.f;
			Piece.bStoneWall = false;
			OutPieces.Add(Piece);
		};
		AddTower(Gap0 - SolidWallgate::TowerFootprintXCm * 0.5f);
		AddTower(Gap1 + SolidWallgate::TowerFootprintXCm * 0.5f);
	}
}

int32 SolidWallgate::CollectPieces(
	const USolidWorldChart* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	TArray<FPiece>& OutPieces)
{
	OutPieces.Reset();
	if (!WorldMap)
	{
		return 0;
	}

	for (const SolidTowns::FTownInfo& Info : SolidTowns::Definitions)
	{
		if (Info.Type != SolidTowns::ETownType::WallGate)
		{
			continue;
		}

		FSolidWorldChartMark Mark;
		FVector2D Marker = FVector2D::ZeroVector;
		if (!WorldMap->FindTownMarker(Info.Id, Mark)
			|| !WorldMap->TerrainPointToWorld(Mark.TerrainPoint, WorldMinXY, WorldMaxXY, Marker))
		{
			continue;
		}
		WallgateAppendRun(*WorldMap, WorldMinXY, WorldMaxXY, Marker, OutPieces);
	}
	return OutPieces.Num();
}

bool SolidWallgate::GateCenterXY(
	const USolidWorldChart* WorldMap,
	const int32 TownId,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutCenter)
{
	OutCenter = FVector2D::ZeroVector;
	if (!WorldMap || !SolidTowns::IsWallGate(TownId))
	{
		return false;
	}
	FSolidWorldChartMark Mark;
	if (!WorldMap->FindTownMarker(TownId, Mark))
	{
		return false;
	}
	return WorldMap->TerrainPointToWorld(Mark.TerrainPoint, WorldMinXY, WorldMaxXY, OutCenter);
}
