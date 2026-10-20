#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SolidWorldMap.h"
#include "SolidWorldChart.generated.h"

/** What a mark on the world chart is. A site has a size and no type. */
UENUM()
enum class ESolidWorldChartMarkKind : uint8
{
	Town,
	Stronghold,
	StrongholdSite,
	LairEntrance,
	LairExit,
};

/** How wide a chart road is. The route is solved when a world map loads the chart. */
UENUM()
enum class ESolidWorldChartRoadType : uint8
{
	Trail,
	Track,
	Road,
	Highway,
};

/**
 * One named place on the chart. Its position is the cell centre, in centimetres.
 * A world map turns that position into a cell once, then keeps the cell.
 */
USTRUCT()
struct FSolidWorldChartMark
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	ESolidWorldChartMarkKind Kind = ESolidWorldChartMarkKind::Town;

	/**
	 * Town, stronghold, site, or lair name.
	 * MeagreDungeon and Meagre Dungeon are the same stronghold name.
	 */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FString Name;

	/** World position in centimetres, snapped to the cell centre. +X is east, +Y is north. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FVector2D PositionCm = FVector2D::ZeroVector;

	/** Set for a stronghold. A site has no type. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	ESolidStrongholdType StrongholdType = ESolidStrongholdType::Dungeon;

	/** Set for a stronghold and a stronghold site. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	ESolidStrongholdSize StrongholdSize = ESolidStrongholdSize::Meagre;

	/** Eight hex digits when this mark is a lair mouth. Empty otherwise. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FString LairId;
};

/** A road definition on the chart. Two end names and a type. The solver runs on load. */
USTRUCT()
struct FSolidWorldChartRoad
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FString FromName;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FString ToName;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	ESolidWorldChartRoadType Type = ESolidWorldChartRoadType::Trail;
};

/**
 * The shipped world as a data asset. Cells hold marker biomes. Marks and roads
 * hold the same facts as Content/WorldMap.txt. The game still builds the world
 * from that text file. The editor saves the import at /Game/World/WorldChart.
 */
UCLASS(BlueprintType)
class SOLIDCORE1_API USolidWorldChart : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Editor path of the shipped chart. */
	static const TCHAR* ShippedPath();

	/**
	 * World rectangle the cell centres use, in centimetres.
	 * This is the terrain map's default square, origin at zero.
	 */
	static void DefaultWorldBounds(FVector2D& OutMin, FVector2D& OutMax);

	/**
	 * Fill this chart from WorldMap.txt text. A ! is stored as grassland.
	 * Named towns, strongholds, sites, and lair mouths become marks.
	 * A road is stored as two end names and a type. The route is not solved here.
	 */
	bool ImportFromString(const FString& Text);

	/** Fill this asset from Content/WorldMap.txt so the editor can save it. */
	UFUNCTION(CallInEditor, Category = "WorldChart", meta = (DisplayName = "Import WorldMap.txt"))
	void ImportShippedWorldMap();

	/** Word the road line uses for this type: trail, track, road, or highway. */
	static const TCHAR* RoadTypeName(ESolidWorldChartRoadType Type);

	static ESolidWorldChartRoadType ChartRoadType(SolidRoad::EType Type);

	/** Marker biome of each cell. Row-major. Row 0 is north, column 0 is east. A ! is grassland. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<ESolidBiome> Cells;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<FSolidWorldChartMark> Marks;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<FSolidWorldChartRoad> Roads;
};
