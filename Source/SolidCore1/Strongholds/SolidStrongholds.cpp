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
			if (!SolidStronghold::AttributesForMarker(WorldMap->GetMarkerAtCell(Col, Row), Attributes))
			{
				continue;
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
				continue;
			}

			FVector2D CaveXY = FVector2D::ZeroVector;
			FVector2D GrassXY = FVector2D::ZeroVector;
			if (!WorldMap->CellToWorldXY(FIntPoint(Col, Row), WorldMinXY, WorldMaxXY, CaveXY)
				|| !WorldMap->CellToWorldXY(GrassCell, WorldMinXY, WorldMaxXY, GrassXY))
			{
				continue;
			}

			FEntrance Entrance;
			Entrance.Stronghold = Attributes;
			Entrance.Cell = FIntPoint(Col, Row);
			Entrance.GrassCell = GrassCell;
			Entrance.EdgeXY = (CaveXY + GrassXY) * 0.5f;
			const FVector2D TowardGrass = (GrassXY - CaveXY).GetSafeNormal();
			Entrance.WorldXY = Entrance.EdgeXY + TowardGrass * FaceInsetCm;
			Entrance.FacingYawDeg = YawFacingGrass(*ChosenStep);
			OutEntrances.Add(Entrance);
		}
	}

	return OutEntrances.Num();
}
