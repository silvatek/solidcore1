#include "SolidWorldChart.h"
#include "SolidCore1.h"
#include "SolidTerrainMap.h"
#include "SolidTowns.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	ESolidWorldChartMarkKind ChartMarkKind(const ESolidWorldBang Kind)
	{
		switch (Kind)
		{
		case ESolidWorldBang::Stronghold:
			return ESolidWorldChartMarkKind::Stronghold;
		case ESolidWorldBang::StrongholdSite:
			return ESolidWorldChartMarkKind::StrongholdSite;
		case ESolidWorldBang::LairEntrance:
			return ESolidWorldChartMarkKind::LairEntrance;
		case ESolidWorldBang::LairExit:
			return ESolidWorldChartMarkKind::LairExit;
		case ESolidWorldBang::Town:
		default:
			return ESolidWorldChartMarkKind::Town;
		}
	}

	FString ChartRoadEndName(const SolidRoad::FEndpoint& End)
	{
		if (End.bStronghold)
		{
			return End.StrongholdName;
		}
		return SolidTowns::NameFor(End.LocationId);
	}
}

const TCHAR* USolidWorldChart::ShippedPath()
{
	return TEXT("/Game/World/WorldChart");
}

void USolidWorldChart::DefaultWorldBounds(FVector2D& OutMin, FVector2D& OutMax)
{
	const USolidTerrainMap* Terrain = GetDefault<USolidTerrainMap>();
	OutMin = Terrain->GetWorldMinXY();
	OutMax = Terrain->GetWorldMaxXY();
}

const TCHAR* USolidWorldChart::RoadTypeName(const ESolidWorldChartRoadType Type)
{
	switch (Type)
	{
	case ESolidWorldChartRoadType::Track:
		return TEXT("track");
	case ESolidWorldChartRoadType::Road:
		return TEXT("road");
	case ESolidWorldChartRoadType::Highway:
		return TEXT("highway");
	case ESolidWorldChartRoadType::Trail:
	default:
		return TEXT("trail");
	}
}

ESolidWorldChartRoadType USolidWorldChart::ChartRoadType(const SolidRoad::EType Type)
{
	switch (Type)
	{
	case SolidRoad::EType::Track:
		return ESolidWorldChartRoadType::Track;
	case SolidRoad::EType::Road:
		return ESolidWorldChartRoadType::Road;
	case SolidRoad::EType::Highway:
		return ESolidWorldChartRoadType::Highway;
	case SolidRoad::EType::Trail:
	default:
		return ESolidWorldChartRoadType::Trail;
	}
}

bool USolidWorldChart::ImportFromString(const FString& Text)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	if (!Map->LoadFromString(Text))
	{
		return false;
	}

	FVector2D WorldMin = FVector2D::ZeroVector;
	FVector2D WorldMax = FVector2D::ZeroVector;
	DefaultWorldBounds(WorldMin, WorldMax);

	TArray<ESolidBiome> NewCells;
	NewCells.SetNum(USolidWorldMap::MapWidth * USolidWorldMap::MapHeight);
	TArray<FSolidWorldChartMark> NewMarks;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			const int32 Index = Row * USolidWorldMap::MapWidth + Col;
			NewCells[Index] = Map->GetMarkerBiomeAtCell(Col, Row);

			FSolidWorldBang Bang;
			if (!Map->FindBang(Col, Row, Bang))
			{
				continue;
			}

			FSolidWorldChartMark Mark;
			Mark.Kind = ChartMarkKind(Bang.Kind);
			Mark.Name = Bang.Name;
			Mark.StrongholdType = Bang.StrongholdType;
			Mark.StrongholdSize = Bang.StrongholdSize;
			Mark.LairId = Bang.LairId;
			FVector2D Centre = FVector2D::ZeroVector;
			if (!Map->CellToWorldXY(Bang.Cell, WorldMin, WorldMax, Centre))
			{
				continue;
			}
			Mark.PositionCm = Centre;
			NewMarks.Add(Mark);
		}
	}

	TArray<FSolidWorldChartRoad> NewRoads;
	for (const SolidRoad::FRoute& Route : Map->GetRoads())
	{
		FSolidWorldChartRoad Road;
		Road.FromName = ChartRoadEndName(Route.Definition.From);
		Road.ToName = ChartRoadEndName(Route.Definition.To);
		Road.Type = ChartRoadType(Route.Definition.Type);
		NewRoads.Add(Road);
	}

	Cells = MoveTemp(NewCells);
	Marks = MoveTemp(NewMarks);
	Roads = MoveTemp(NewRoads);
	return true;
}

void USolidWorldChart::ImportShippedWorldMap()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("WorldMap.txt");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !ImportFromString(Text))
	{
		UE_LOG(LogSolid, Error, TEXT("WorldChart: failed to import Content/WorldMap.txt."));
		return;
	}
	Modify();
	UE_LOG(LogSolid, Warning, TEXT("WorldChart imported from Content/WorldMap.txt. Save this asset as %s."), ShippedPath());
}
