#include "SolidTownSigns.h"
#include "SolidTowns.h"
#include "SolidWallgate.h"
#include "SolidWorldChart.h"

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
	const USolidWorldChart* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	TArray<FPlacement>& OutPlacements)
{
	OutPlacements.Reset();
	if (!WorldMap)
	{
		return 0;
	}

	for (const SolidTowns::FTownInfo& Town : SolidTowns::Definitions)
	{
		FSolidWorldChartMark Mark;
		FIntPoint Cell;
		if (!WorldMap->FindTownMarker(Town.Id, Mark)
			|| !USolidWorldChart::CellAtTerrainPoint(Mark.TerrainPoint, Cell))
		{
			continue;
		}

		FVector2D Anchor = FVector2D::ZeroVector;
		if (!WorldMap->TerrainPointToWorld(Mark.TerrainPoint, WorldMinXY, WorldMaxXY, Anchor))
		{
			continue;
		}
		FIntPoint AnchorCell = Cell;
		if (SolidTowns::IsWallGate(Town.Id))
		{
			FVector2D Gate = FVector2D::ZeroVector;
			FIntPoint Marker = FIntPoint::ZeroValue;
			if (SolidWallgate::GateCenterXY(WorldMap, Town.Id, WorldMinXY, WorldMaxXY, Gate)
				&& SolidWallgate::MarkerCell(WorldMap, Town.Id, Marker))
			{
				Anchor = Gate;
				AnchorCell = Marker;
			}
		}

		FPlacement Placement;
		Placement.LocationId = Town.Id;
		Placement.Name = Town.Name;
		Placement.Cell = AnchorCell;
		Placement.WorldXY = Anchor + Town.SignOffsetCm;
		Placement.YawDeg = Town.SignYawDeg;
		OutPlacements.Add(Placement);
	}

	return OutPlacements.Num();
}
