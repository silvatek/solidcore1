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
		// Column +1 is world -X. Row +1 is world -Y. Matches the cave mouth.
		const float WorldX = -static_cast<float>(Step.X);
		const float WorldY = -static_cast<float>(Step.Y);
		return FMath::RadiansToDegrees(FMath::Atan2(WorldY, WorldX));
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
		TArray<SolidLair::FEntrance>& OutEntrances)
	{
		if (EndMarker == 0 || AlreadyPlaced(OutEntrances, Col, Row))
		{
			return false;
		}

		// Column +1 is west. Row +1 is south. The exit tries west first.
		const bool bExit = FChar::ToUpper(EndMarker) == SolidLair::Exit;
		const FIntPoint Steps[] = {
			bExit ? FIntPoint(1, 0) : FIntPoint(0, -1),
			bExit ? FIntPoint(0, -1) : FIntPoint(-1, 0),
			bExit ? FIntPoint(-1, 0) : FIntPoint(0, 1),
			bExit ? FIntPoint(0, 1) : FIntPoint(1, 0),
		};
		bool bChosen = false;
		FIntPoint ChosenStep = FIntPoint::ZeroValue;
		FIntPoint GrassCell = FIntPoint::ZeroValue;
		for (const FIntPoint& Step : Steps)
		{
			const int32 NextX = Col + Step.X;
			const int32 NextY = Row + Step.Y;
			if (!InBounds(NextX, NextY))
			{
				continue;
			}
			if (WorldMap.GetBiomeAtCell(NextX, NextY) != ESolidBiome::Grassland)
			{
				continue;
			}
			bChosen = true;
			ChosenStep = Step;
			GrassCell = FIntPoint(NextX, NextY);
			break;
		}
		if (!bChosen)
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

	auto AddMouth = [&](const ESolidWorldChartMarkKind Kind, const TCHAR End)
	{
		FSolidWorldChartMark Mark;
		FIntPoint Cell;
		if (!WorldMap->FindLairMarker(SolidLair::IdIglinTunnels, Kind, Mark)
			|| !USolidWorldChart::CellAtTerrainPoint(Mark.TerrainPoint, Cell))
		{
			return;
		}
		SolidLairMouth::TryAdd(*WorldMap, WorldMinXY, WorldMaxXY, Cell.X, Cell.Y, End, OutEntrances);
	};
	AddMouth(ESolidWorldChartMarkKind::LairEntrance, SolidLair::Entrance);
	AddMouth(ESolidWorldChartMarkKind::LairExit, SolidLair::Exit);
	return OutEntrances.Num();
}
