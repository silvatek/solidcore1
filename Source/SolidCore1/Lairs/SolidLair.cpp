#include "SolidLair.h"
#include "SolidStronghold.h"

namespace SolidLairMouth
{
	bool OnGrid(const FIntPoint& Point)
	{
		return Point.X >= 0 && Point.Y >= 0
			&& Point.X < USolidWorldChart::PointAxis
			&& Point.Y < USolidWorldChart::PointAxis;
	}

	/** One terrain point toward the grass. Terrain X grows west and terrain Y grows north. */
	FIntPoint StepFor(const SolidLair::EOrientation Orientation)
	{
		switch (Orientation)
		{
		case SolidLair::EOrientation::North: return FIntPoint(0, 1);
		case SolidLair::EOrientation::East: return FIntPoint(-1, 0);
		case SolidLair::EOrientation::South: return FIntPoint(0, -1);
		case SolidLair::EOrientation::West: return FIntPoint(1, 0);
		default: return FIntPoint(0, 1);
		}
	}

	bool AlreadyPlaced(const TArray<SolidLair::FEntrance>& Entrances, const FIntPoint& TerrainPoint)
	{
		for (const SolidLair::FEntrance& Entrance : Entrances)
		{
			if (Entrance.TerrainPoint == TerrainPoint)
			{
				return true;
			}
		}
		return false;
	}

	bool TryAdd(
		const USolidWorldChart& WorldMap,
		const FVector2D WorldMinXY,
		const FVector2D WorldMaxXY,
		const FIntPoint TerrainPoint,
		const TCHAR EndMarker,
		const SolidLair::EOrientation Orientation,
		TArray<SolidLair::FEntrance>& OutEntrances)
	{
		if (EndMarker == 0 || AlreadyPlaced(OutEntrances, TerrainPoint) || !OnGrid(TerrainPoint))
		{
			return false;
		}

		const FIntPoint ChosenStep = StepFor(Orientation);
		const FIntPoint GrassPoint = TerrainPoint + ChosenStep;
		if (!OnGrid(GrassPoint))
		{
			return false;
		}

		FVector2D MouthXY = FVector2D::ZeroVector;
		FVector2D GrassXY = FVector2D::ZeroVector;
		if (!WorldMap.TerrainPointToWorld(TerrainPoint, WorldMinXY, WorldMaxXY, MouthXY)
			|| !WorldMap.TerrainPointToWorld(GrassPoint, WorldMinXY, WorldMaxXY, GrassXY))
		{
			return false;
		}

		SolidLair::FEntrance Entrance;
		Entrance.EndMarker = FChar::ToUpper(EndMarker);
		Entrance.TerrainPoint = TerrainPoint;
		Entrance.GrassPoint = GrassPoint;
		Entrance.EdgeXY = (MouthXY + GrassXY) * 0.5f;
		const FVector2D TowardGrass = (GrassXY - MouthXY).GetSafeNormal();
		Entrance.WorldXY = Entrance.EdgeXY + TowardGrass * SolidStrongholds::FaceInsetCm;
		Entrance.FacingYawDeg = FMath::RadiansToDegrees(FMath::Atan2(TowardGrass.Y, TowardGrass.X));
		OutEntrances.Add(Entrance);
		return true;
	}
}

int32 SolidLair::CollectEntrances(
	const USolidWorldChart* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	TArray<FEntrance>& OutEntrances)
{
	OutEntrances.Reset();
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return 0;
	}

	const SolidLair::FDefinition& Lair = SolidLair::IglinTunnels;
	auto AddMouth = [&](const ESolidWorldChartMarkKind Kind, const TCHAR End, const SolidLair::EOrientation Orientation)
	{
		FSolidWorldChartMark Mark;
		if (!WorldMap->FindLairMarker(Lair.Id, Kind, Mark))
		{
			return;
		}
		SolidLairMouth::TryAdd(
			*WorldMap, WorldMinXY, WorldMaxXY, Mark.TerrainPoint, End, Orientation, OutEntrances);
	};
	AddMouth(ESolidWorldChartMarkKind::LairEntrance, SolidLair::Entrance, Lair.EntranceOrientation);
	AddMouth(ESolidWorldChartMarkKind::LairExit, SolidLair::Exit, Lair.ExitOrientation);
	return OutEntrances.Num();
}
