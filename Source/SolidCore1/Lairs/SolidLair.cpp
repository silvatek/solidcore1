#include "SolidLair.h"
#include "SolidStrongholds.h"

namespace SolidLairMouth
{
	/** Map step (column, row) toward grass. Row +1 is south; column +1 is west. Same order as the cave. */
	const FIntPoint NeighborSteps[] = {
		FIntPoint(0, -1),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(1, 0),
	};

	bool InBounds(const int32 X, const int32 Y)
	{
		return X >= 0 && X < USolidWorldMap::MapWidth && Y >= 0 && Y < USolidWorldMap::MapHeight;
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

	/** LairEntrance is the interior X end. LairExit is the interior Y end. */
	TCHAR EndForBang(const FSolidWorldBang& Bang)
	{
		if (!Bang.Name.Equals(SolidLair::IglinTunnels.Name, ESearchCase::IgnoreCase))
		{
			return 0;
		}
		if (Bang.Kind == ESolidWorldBang::LairEntrance)
		{
			return USolidWorldMap::XMarker;
		}
		if (Bang.Kind == ESolidWorldBang::LairExit)
		{
			return USolidWorldMap::YMarker;
		}
		return 0;
	}

	bool TryAdd(
		const USolidWorldMap& WorldMap,
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

		const FIntPoint* ChosenStep = nullptr;
		FIntPoint GrassCell = FIntPoint::ZeroValue;
		for (const FIntPoint& Step : NeighborSteps)
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
			ChosenStep = &Step;
			GrassCell = FIntPoint(NextX, NextY);
			break;
		}
		if (!ChosenStep)
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
		Entrance.FacingYawDeg = YawFacingGrass(*ChosenStep);
		OutEntrances.Add(Entrance);
		return true;
	}
}

int32 SolidLair::CollectEntrances(
	const USolidWorldMap* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	TArray<FEntrance>& OutEntrances)
{
	OutEntrances.Reset();
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return 0;
	}

	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			FSolidWorldBang Bang;
			if (!WorldMap->FindBang(Col, Row, Bang))
			{
				continue;
			}
			const TCHAR End = SolidLairMouth::EndForBang(Bang);
			if (End == 0)
			{
				continue;
			}
			SolidLairMouth::TryAdd(*WorldMap, WorldMinXY, WorldMaxXY, Col, Row, End, OutEntrances);
		}
	}

	return OutEntrances.Num();
}
