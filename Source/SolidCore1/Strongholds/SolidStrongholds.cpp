#include "SolidStrongholds.h"
#include "SolidWorldMap.h"

namespace
{
	/** Map step (column, row) toward grass. Row +1 is south; column +1 is west. */
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
		// Column +1 is world -X. Row +1 is world -Y.
		const float WorldX = -static_cast<float>(Step.X);
		const float WorldY = -static_cast<float>(Step.Y);
		return FMath::RadiansToDegrees(FMath::Atan2(WorldY, WorldX));
	}
}

bool SolidStrongholds::MakeEntrance(
	const USolidWorldMap* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	const FIntPoint Cell,
	const FSolidStronghold& Stronghold,
	FEntrance& OutEntrance)
{
	OutEntrance = FEntrance();
	if (!WorldMap || !WorldMap->IsLoaded() || !InBounds(Cell.X, Cell.Y))
	{
		return false;
	}

	const FIntPoint* ChosenStep = nullptr;
	FIntPoint GrassCell = FIntPoint::ZeroValue;
	for (const FIntPoint& Step : NeighborSteps)
	{
		const int32 NextX = Cell.X + Step.X;
		const int32 NextY = Cell.Y + Step.Y;
		if (!InBounds(NextX, NextY))
		{
			continue;
		}
		if (WorldMap->GetBiomeAtCell(NextX, NextY) != ESolidBiome::Grassland)
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

	FVector2D CaveXY = FVector2D::ZeroVector;
	FVector2D GrassXY = FVector2D::ZeroVector;
	if (!WorldMap->CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, CaveXY)
		|| !WorldMap->CellToWorldXY(GrassCell, WorldMinXY, WorldMaxXY, GrassXY))
	{
		return false;
	}

	OutEntrance.Stronghold = Stronghold;
	OutEntrance.Cell = Cell;
	OutEntrance.GrassCell = GrassCell;
	OutEntrance.EdgeXY = (CaveXY + GrassXY) * 0.5f;
	const FVector2D TowardGrass = (GrassXY - CaveXY).GetSafeNormal();
	OutEntrance.WorldXY = OutEntrance.EdgeXY + TowardGrass * FaceInsetCm;
	OutEntrance.FacingYawDeg = YawFacingGrass(*ChosenStep);
	return true;
}

int32 SolidStrongholds::CollectEntrances(
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
			FSolidStronghold Attributes;
			if (!WorldMap->StrongholdAtCell(Col, Row, Attributes))
			{
				continue;
			}

			FEntrance Entrance;
			if (!MakeEntrance(
				WorldMap,
				WorldMinXY,
				WorldMaxXY,
				FIntPoint(Col, Row),
				Attributes,
				Entrance))
			{
				continue;
			}
			OutEntrances.Add(Entrance);
		}
	}

	return OutEntrances.Num();
}

bool SolidStrongholds::FindGrassFace(
	const USolidWorldMap* WorldMap,
	const FIntPoint Cell,
	FGrassFace& Out)
{
	Out = FGrassFace();
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return false;
	}

	for (const FIntPoint& Step : NeighborSteps)
	{
		const int32 NextX = Cell.X + Step.X;
		const int32 NextY = Cell.Y + Step.Y;
		if (!InBounds(NextX, NextY))
		{
			continue;
		}
		if (WorldMap->GetBiomeAtCell(NextX, NextY) != ESolidBiome::Grassland)
		{
			continue;
		}
		Out.GrassCell = FIntPoint(NextX, NextY);
		Out.FacingYawDeg = YawFacingGrass(Step);
		return true;
	}
	return false;
}
