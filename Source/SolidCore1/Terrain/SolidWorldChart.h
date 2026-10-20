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
	/** A short note. It does not change the land, and it is not a road end. */
	Custom,
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
	 * Town, stronghold, site, or lair name, or the text of a custom note.
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
 * What a cell shows under the cursor.
 * Painted is the marker biome. Resolved is the biome the world map uses.
 * A mark can replace the paint: a lair mouth is mountain, a named town is town.
 */
struct FSolidWorldChartRead
{
	bool bValid = false;
	ESolidBiome Painted = ESolidBiome::Grassland;
	ESolidBiome Resolved = ESolidBiome::Grassland;
	/** True when Resolved is not the painted marker biome. */
	bool bMarkOverrides = false;
	bool bHasMark = false;
	FSolidWorldChartMark Mark;
	/** Indexes into Roads whose solved route includes this cell. */
	TArray<int32> RoadIndexes;
};

/**
 * The shipped world as a data asset. Cells hold marker biomes. Marks and roads
 * hold the same facts as Content/WorldMap.txt. The game builds the world from
 * /Game/World/WorldChart. The editor saves the import at that path.
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

	/**
	 * Centre of one map cell, in the chart's centimetre rectangle.
	 * Column 0 is east, row 0 is north. The brush is this one cell.
	 */
	static bool CellCentre(FIntPoint Cell, FVector2D& OutCm);

	/**
	 * Paint marker biomes. Swamp and road are rejected. Marks stay where they are.
	 * One call is one undo step. A run is the cells of one drag. A rectangle is inclusive.
	 */
	bool PaintCell(FIntPoint Cell, ESolidBiome Biome, FString& OutError);
	bool PaintRun(const TArray<FIntPoint>& CellsToPaint, ESolidBiome Biome, FString& OutError);
	bool PaintRectangle(FIntPoint From, FIntPoint To, ESolidBiome Biome, FString& OutError);

	/** Put a SolidTowns town on this cell, replacing any mark already there. */
	bool PlaceTown(FIntPoint Cell, const FString& Name, FString& OutError);

	/** Put a stronghold the game already has. The name is its size and type. */
	bool PlaceStronghold(FIntPoint Cell, ESolidStrongholdSize Size, ESolidStrongholdType Type, FString& OutError);

	/** Put a stronghold site. A site has a size and no type, and it is not a road end. */
	bool PlaceSite(FIntPoint Cell, ESolidStrongholdSize Size, FString& OutError);

	/** Put one end of a lair the game already has. The mouth resolves as mountain. */
	bool PlaceLairMouth(FIntPoint Cell, const FString& LairName, bool bEntrance, FString& OutError);

	/**
	 * Put a custom note on this cell, replacing any mark already there.
	 * The note is one line, at most 48 characters. It does not change the paint.
	 */
	bool PlaceCustom(FIntPoint Cell, const FString& Note, FString& OutError);

	/** Remove the mark on this cell. */
	bool DeleteMark(FIntPoint Cell, FString& OutError);

	/**
	 * Store a road from a town or stronghold to another.
	 * A missing end is named in OutError, and the road is still stored.
	 * The solver then leaves that route empty. A site is not an end.
	 */
	bool AddRoad(const FString& From, const FString& To, ESolidWorldChartRoadType Type, FString& OutError);

	/** Empty when both ends are a placed town or stronghold. Otherwise the reason. */
	FString RoadEndError(const FString& From, const FString& To) const;

	/** Solved cells of one road, including both ends. Empty when an end is missing. */
	bool RouteForRoad(int32 RoadIndex, TArray<FIntPoint>& OutCells) const;

	/** Resolved biome, painted biome, mark, and roads through this cell. */
	bool ReadCell(FIntPoint Cell, FSolidWorldChartRead& Out) const;

	/** Index of the mark on this cell, or INDEX_NONE. */
	int32 MarkAtCell(FIntPoint Cell) const;

	/** Put back the chart from before the last paint, mark, or road. */
	bool Undo();

	/**
	 * Empty when this chart matches WorldMap.txt text.
	 * Otherwise a sentence the World chart mode can show.
	 */
	FString Disagreement(const FString& WorldMapText) const;

	/** Marker biome of each cell. Row-major. Row 0 is north, column 0 is east. A ! is grassland. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<ESolidBiome> Cells;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<FSolidWorldChartMark> Marks;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<FSolidWorldChartRoad> Roads;

private:
	/** Cells, marks, and roads from before one edit. Not saved with the asset. */
	struct FSnapshot
	{
		TArray<ESolidBiome> Cells;
		TArray<FSolidWorldChartMark> Marks;
		TArray<FSolidWorldChartRoad> Roads;
	};

	void PushUndo();
	bool CellIndex(FIntPoint Cell, int32& OutIndex) const;
	bool PlaceMark(FIntPoint Cell, const FSolidWorldChartMark& Mark, FString& OutError);

	TArray<FSnapshot> UndoStack;
};
