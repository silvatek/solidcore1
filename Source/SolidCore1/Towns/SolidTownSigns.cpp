#include "SolidTownSigns.h"
#include "SolidTowns.h"
#include "SolidWorldMap.h"

static_assert(
	SolidTowns::SignFacesSouthYawDeg == SolidTownSigns::FacingYawDeg,
	"The usual welcome board faces south.");

FString SolidTownSigns::MakeLabel(const FString& TownName)
{
	return FString::Printf(TEXT("%s\n%s"), WelcomeLine, *TownName);
}

float SolidTownSigns::BoardWidthCm(const FString& Label)
{
	TArray<FString> Lines;
	Label.ParseIntoArray(Lines, TEXT("\n"), false);
	int32 Longest = 1;
	for (const FString& Line : Lines)
	{
		Longest = FMath::Max(Longest, Line.Len());
	}
	return TextWorldSize * 0.55f * static_cast<float>(Longest) + TextWorldSize * 0.9f;
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

		FVector2D Offset = FVector2D(0.f, 0.f);
		float YawDeg = FacingYawDeg;
		if (const SolidTowns::FTownInfo* Town = SolidTowns::Find(Location.Id))
		{
			Offset = Town->SignOffsetCm;
			YawDeg = Town->SignYawDeg;
		}

		// One board for the town, at the middle of its cells. A town offset is added from there.
		FVector2D Sum = FVector2D::ZeroVector;
		int32 Found = 0;
		FIntPoint AnchorCell = Cells.Num() > 0 ? Cells[0] : FIntPoint::ZeroValue;
		for (const FIntPoint& Cell : Cells)
		{
			FVector2D WorldXY = FVector2D::ZeroVector;
			if (!WorldMap->CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, WorldXY))
			{
				continue;
			}
			Sum += WorldXY;
			++Found;
			AnchorCell = Cell;
		}
		if (Found > 0)
		{
			FPlacement Placement;
			Placement.LocationId = Location.Id;
			Placement.Name = Location.Name;
			Placement.Cell = AnchorCell;
			Placement.WorldXY = (Sum / static_cast<float>(Found)) + Offset;
			Placement.YawDeg = YawDeg;
			OutPlacements.Add(Placement);
		}
	}

	return OutPlacements.Num();
}
