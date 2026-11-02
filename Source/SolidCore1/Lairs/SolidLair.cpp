#include "SolidLair.h"
#include "SolidStronghold.h"

namespace SolidLairMouth
{
	bool InBounds(const int32 X, const int32 Y)
	{
		return X >= 0 && X < USolidWorldChart::MapWidth && Y >= 0 && Y < USolidWorldChart::MapHeight;
	}

	float YawFacingGrass(const FIntPoint& Step)
	{
		// Column +1 is world -X, which is east. Row +1 is world -Y, which is south.
		const float WorldX = -static_cast<float>(Step.X);
		const float WorldY = -static_cast<float>(Step.Y);
		return FMath::RadiansToDegrees(FMath::Atan2(WorldY, WorldX));
	}

	FIntPoint StepFor(const SolidLair::EOrientation Orientation)
	{
		switch (Orientation)
		{
		case SolidLair::EOrientation::North: return FIntPoint(0, -1);
		case SolidLair::EOrientation::East: return FIntPoint(1, 0);
		case SolidLair::EOrientation::South: return FIntPoint(0, 1);
		case SolidLair::EOrientation::West: return FIntPoint(-1, 0);
		default: return FIntPoint(0, -1);
		}
	}

	bool AlreadyPlaced(const TArray<SolidLair::FEntrance>& Entrances, const int32 Col, const int32 Row)
	{
		for (const SolidLair::FEntrance& Entrance : Entrances)
		{
			if (Entrance.Cell.X == Col && Entrance.Cell.Y == Row)
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
		const int32 Col,
		const int32 Row,
		const TCHAR EndMarker,
		const SolidLair::EOrientation Orientation,
		TArray<SolidLair::FEntrance>& OutEntrances)
	{
		if (EndMarker == 0 || AlreadyPlaced(OutEntrances, Col, Row))
		{
			return false;
		}

		const FIntPoint ChosenStep = StepFor(Orientation);
		const FIntPoint GrassCell(Col + ChosenStep.X, Row + ChosenStep.Y);
		if (!InBounds(GrassCell.X, GrassCell.Y))
		{
			return false;
		}

		FVector2D MouthXY = FVector2D::ZeroVector;
		FVector2D GrassXY = FVector2D::ZeroVector;
		if (!WorldMap.CellToWorldXY(FIntPoint(Col, Row), WorldMinXY, WorldMaxXY, MouthXY)
			|| !WorldMap.CellToWorldXY(GrassCell, WorldMinXY, WorldMaxXY, GrassXY))
		{
			return false;
		}

		SolidLair::FEntrance Entrance;
		Entrance.EndMarker = FChar::ToUpper(EndMarker);
		Entrance.Cell = FIntPoint(Col, Row);
		Entrance.GrassCell = GrassCell;
		Entrance.EdgeXY = (MouthXY + GrassXY) * 0.5f;
		const FVector2D TowardGrass = (GrassXY - MouthXY).GetSafeNormal();
		Entrance.WorldXY = Entrance.EdgeXY + TowardGrass * SolidStrongholds::FaceInsetCm;
		Entrance.FacingYawDeg = YawFacingGrass(ChosenStep);
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
		FIntPoint Cell = FIntPoint::ZeroValue;
		if (!WorldMap->FindLairMarker(Lair.Id, Kind, Mark)
			|| !USolidWorldChart::CellAtTerrainPoint(Mark.TerrainPoint, Cell))
		{
			return;
		}
		SolidLairMouth::TryAdd(
			*WorldMap, WorldMinXY, WorldMaxXY, Cell.X, Cell.Y, End, Orientation, OutEntrances);
	};
	AddMouth(ESolidWorldChartMarkKind::LairEntrance, SolidLair::Entrance, Lair.EntranceOrientation);
	AddMouth(ESolidWorldChartMarkKind::LairExit, SolidLair::Exit, Lair.ExitOrientation);
	return OutEntrances.Num();
}
