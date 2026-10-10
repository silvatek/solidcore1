#include "SolidTownSigns.h"
#include "SolidWorldMap.h"

FString SolidTownSigns::MakeLabel(const FString& TownName)
{
	return FString::Printf(TEXT("Welcome to %s"), *TownName);
}

int32 SolidTownSigns::CollectPlacements(
	const USolidWorldMap* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	TArray<FPlacement>& OutPlacements)
{
	OutPlacements.Reset();
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return 0;
	}

	TArray<FSolidWorldLocation> Locations;
	WorldMap->CollectLocations(Locations);

	for (const FSolidWorldLocation& Location : Locations)
	{
		if (Location.Name.IsEmpty())
		{
			continue;
		}

		TArray<FIntPoint> Cells = Location.Cells;
		Cells.Sort([](const FIntPoint& A, const FIntPoint& B)
		{
			if (A.Y != B.Y)
			{
				return A.Y < B.Y;
			}
			return A.X < B.X;
		});

		for (const FIntPoint& Cell : Cells)
		{
			FVector2D WorldXY = FVector2D::ZeroVector;
			if (!WorldMap->CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, WorldXY))
			{
				continue;
			}

			FPlacement Placement;
			Placement.LocationId = Location.Id;
			Placement.Name = Location.Name;
			Placement.Cell = Cell;
			Placement.WorldXY = WorldXY;
			OutPlacements.Add(Placement);
		}
	}

	return OutPlacements.Num();
}
