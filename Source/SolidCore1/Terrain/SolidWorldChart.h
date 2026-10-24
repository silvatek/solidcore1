#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SolidRoad.h"
#include "SolidStronghold.h"
#include "SolidTerrainTypes.h"
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
	/** A catalog encounter. It does not change the land. */
	Encounter,
};

/** How wide a chart road is. The route is solved when the chart is asked. */
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
 * Asking the chart turns that position into a cell once, then keeps the cell.
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

/** A road definition on the chart. Two end names and a type. The solver runs when the chart is asked. */
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

/** What a "! row,column = ..." line names. None means the cell has no such line. */
enum class ESolidWorldBang : uint8
{
	None,
	Town,
	Stronghold,
	/** A size, and no type. The cell stays the marker's biome. */
	StrongholdSite,
	LairEntrance,
	LairExit,
};

/** One ! cell named by a description line. Row and column count from 1. */
struct FSolidWorldBang
{
	ESolidWorldBang Kind = ESolidWorldBang::None;
	int32 LocationId = INDEX_NONE;
	FString Name;
	/** Eight hex digits when this cell is a lair mouth. Empty otherwise. */
	FString LairId;
	/** Unused for a stronghold site. */
	ESolidStrongholdType StrongholdType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize StrongholdSize = ESolidStrongholdSize::Meagre;
	FIntPoint Cell = FIntPoint::ZeroValue;
};

/**
 * One named town. Iglin is location 0 and is where the captain starts.
 * Biome is Town.
 */
USTRUCT(BlueprintType)
struct FSolidWorldLocation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WorldChart")
	int32 Id = INDEX_NONE;

	/** From SolidTowns, matched by Id. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldChart")
	FString Name;

	/** Unused. Older maps wrote a role here; a named town leaves this empty. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldChart")
	FString Role;

	UPROPERTY(BlueprintReadOnly, Category = "WorldChart")
	ESolidBiome Biome = ESolidBiome::Town;

	/** Map cells that resolve to this id. Each named town is one cell. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldChart")
	TArray<FIntPoint> Cells;
};

/** One encounter mark. The position is the cell centre, in centimetres. */
struct FSolidWorldEncounterMark
{
	FString Name;
	FVector2D PositionCm = FVector2D::ZeroVector;
};

/**
 * The shipped world. Cells hold marker biomes. Marks and roads hold the towns,
 * strongholds, and roads. The same object answers the map queries: biomes,
 * towns, and solved routes. Those solved facts are not saved with the asset.
 * The game builds the world from /Game/World/WorldChart.
 */
UCLASS(BlueprintType)
class SOLIDCORE1_API USolidWorldChart : public UDataAsset
{
	GENERATED_BODY()

public:
	static constexpr int32 MapWidth = 128;
	static constexpr int32 MapHeight = 64;

	/** Marker for the sea. A wave, not the letter S. */
	static constexpr TCHAR SeaMarker = TEXT('~');

	/** Marker for grassland. A dot, not the letter G. */
	static constexpr TCHAR GrassMarker = TEXT('.');

	/** Marker for a town cell that is not a named location. A small circle, not the letter T. */
	static constexpr TCHAR TownMarker = TEXT('o');

	/** Marker for forest. A hatch, not the letter F. */
	static constexpr TCHAR ForestMarker = TEXT('#');

	/** Marker for a river channel. A star, not the letter R. */
	static constexpr TCHAR RiverMarker = TEXT('*');

	/** Marker for desert. A peak, not the letter D. */
	static constexpr TCHAR DesertMarker = TEXT('^');

	/**
	 * One described cell. "! row,column = Town Name" is a location.
	 * Stronghold, StrongholdSite, LairEntrance, and LairExit use the same coordinates.
	 * Row and column count from 1 at the top-left of the grid.
	 */
	static constexpr TCHAR LocationMarker = TEXT('!');

	/**
	 * Paint letters that are not locations. A LairEntrance ! and a LairExit !
	 * use the mountain shelf. These letters themselves are not mouths, towns,
	 * or the cave.
	 */
	static constexpr TCHAR XMarker = TEXT('X');
	static constexpr TCHAR YMarker = TEXT('Y');

	/** Editor path of the shipped chart. */
	static const TCHAR* ShippedPath();

	/**
	 * World rectangle the cell centres use, in centimetres.
	 * This is the terrain map's default square, origin at zero.
	 */
	static void DefaultWorldBounds(FVector2D& OutMin, FVector2D& OutMax);

	/**
	 * Fill this chart from map text. A ! is stored as grassland.
	 * Named towns, strongholds, sites, and lair mouths become marks.
	 * A road is stored as two end names and a type.
	 */
	bool ImportFromString(const FString& Text);

	/** Copy /Game/World/WorldChart onto this chart and solve it. Idempotent when already solved. */
	bool LoadDefault(bool bForceReload = false);

	/**
	 * Fill the solved queries from map text. A colour line is ignored.
	 * Digits and tunnel letters stay markers. An encounter is not in the text.
	 */
	bool LoadFromString(const FString& Text);

	/**
	 * Copy that chart's cells, marks, and roads onto this one, then solve the routes.
	 * Copying this chart only solves.
	 */
	bool LoadFromChart(const USolidWorldChart* Chart);

	/** Encounter marks. Empty after LoadFromString. */
	const TArray<FSolidWorldEncounterMark>& GetEncounterMarks() const { return EncounterMarks; }

	bool IsLoaded() const { return bIsLoaded; }

	/**
	 * Marker biome at map cell (clamped). The town marker is Town; the desert marker is Desert; M is Mountain.
	 * A named town is Town. A lair mouth is Mountain.
	 * A stronghold uses the shelf that stronghold names. One with no shelf keeps the marker biome.
	 * A stronghold site keeps the marker biome. A ! with no description is Grassland.
	 * The river marker is River for the whole cell. SampleBiome paints only the channel.
	 */
	ESolidBiome GetBiomeAtCell(int32 MapX, int32 MapY) const;

	/**
	 * Marker biome at a cell, before a named town, a stronghold shelf, or a lair mouth replaces it.
	 * A ! is grassland. Swamp and road are not marker biomes.
	 */
	ESolidBiome GetMarkerBiomeAtCell(int32 MapX, int32 MapY) const;

	/** Marker character at a cell, or 0 when the chart is not solved. Digits stay digits. */
	TCHAR GetMarkerAtCell(int32 MapX, int32 MapY) const;

	/** Location id on this cell, or INDEX_NONE when it is not a named town. */
	int32 GetLocationIdAtCell(int32 MapX, int32 MapY) const;

	/** Numbered location under a world XY, or INDEX_NONE. Same rectangle as SampleBiome. */
	int32 SampleLocationId(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/**
	 * Sample biome for a world XY given the TerrainMap world rectangle.
	 * File row 0 → north (max Y). File column 0 → east (max X) so the text
	 * matches the in-world layout when viewed with north up.
	 * A river-marker cell is a 4 m river. The dry part of that square uses the neighboring biome.
	 * A solved road paints its strip as Road, a brown close to the town colour.
	 * River water stays river, and a mountain or the sea keeps its own shelf.
	 */
	ESolidBiome SampleBiome(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** Solved routes, in file order. A route with no cells had a missing end. */
	const TArray<SolidRoad::FRoute>& GetRoads() const { return SolvedRoads; }

	/** True when the point lies on a solved road strip. Does not change the biome by itself. */
	bool IsOnRoad(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** True when this cell is the town marker, or a named town. A road strip is not a town. */
	bool IsMarkedTown(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** Tint for this biome. */
	FLinearColor GetBiomeColor(ESolidBiome Biome) const;

	/**
	 * World XY of the starting-town centroid, using the same world rectangle as SampleBiome.
	 * Location 0. Returns false when that town has no cells.
	 */
	bool GetStartTownWorldXY(FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Copy of a numbered location, including its map cells. */
	bool FindLocation(int32 Id, FSolidWorldLocation& OutLocation) const;

	/** The description of a named cell, or false when that cell has none. */
	bool FindBang(int32 MapX, int32 MapY, FSolidWorldBang& Out) const;

	/** True when this cell is a stronghold. A stronghold site is not one. */
	bool StrongholdAtCell(int32 MapX, int32 MapY, ESolidStrongholdType& OutType, ESolidStrongholdSize& OutSize) const;

	/** True when this cell is a stronghold site. OutSize is that size. */
	bool StrongholdSiteAtCell(int32 MapX, int32 MapY, ESolidStrongholdSize& OutSize) const;

	/** Every numbered location, sorted by id. */
	void CollectLocations(TArray<FSolidWorldLocation>& OutLocations) const;

	/** Centroid of one location's cells. Same world rectangle as SampleBiome. */
	bool GetLocationWorldXY(int32 Id, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** World XY of one map cell center. Same rectangle as SampleBiome. */
	bool CellToWorldXY(FIntPoint Cell, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Map cell under a world XY. Column 0 is east, row 0 is north. */
	bool CellAtWorld(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, FIntPoint& OutCell) const;

	/** The biome's colour. The same answer as GetBiomeColor. */
	static FLinearColor DefaultColorForBiome(ESolidBiome Biome);

	/** Word the road line uses for this type: trail, track, road, or highway. */
	static const TCHAR* RoadTypeName(ESolidWorldChartRoadType Type);

	static ESolidWorldChartRoadType ChartRoadType(SolidRoad::EType Type);

	/**
	 * Centre of one map cell, in the chart's centimetre rectangle.
	 * Column 0 is east, row 0 is north. The brush is this one cell.
	 */
	static bool CellCentre(FIntPoint Cell, FVector2D& OutCm);

	/**
	 * A stored mark position in a terrain map's centimetres.
	 * The chart rectangle starts at the origin. A built map is centred on the origin.
	 * Play and the chart mode both use this shift.
	 */
	static FVector2D TerrainPosition(const FVector2D& PositionCm, const FVector2D& TerrainMin);

	/**
	 * Paint marker biomes. Swamp and road are rejected. Marks stay where they are.
	 * One call is one undo step. A run is the cells of one drag. A rectangle is inclusive.
	 */
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

	/**
	 * Put a catalog encounter on this cell, replacing any mark already there.
	 * The name is the one in SolidEncounter::Definitions. The paint stays.
	 */
	bool PlaceEncounter(FIntPoint Cell, const FString& Name, FString& OutError);

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

	/** Index of the mark on this cell, or INDEX_NONE. */
	int32 MarkAtCell(FIntPoint Cell) const;

	/** Put back the chart from before the last paint, mark, or road. */
	bool Undo();

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
	/** Solve from Cells, Marks, and Roads when a query finds the chart unsolved. */
	bool EnsureQuery() const;
	bool BuildQuery() const;
	/** Drop the solved copy. The next query solves Cells, Marks, and Roads again. */
	void InvalidateQuery();

	TArray<FSnapshot> UndoStack;

	/** Solved facts. Not saved with the asset. A paint, mark, or undo clears them. */
	mutable bool bIsLoaded = false;
	mutable TArray<uint8> Markers;
	mutable TArray<int32> BangLocationIds;
	mutable TMap<int32, FSolidWorldBang> BangMarks;
	mutable bool bHasStartTown = false;
	mutable TArray<FIntPoint> StartTownCells;
	mutable TMap<int32, FSolidWorldLocation> Locations;
	mutable TArray<FSolidWorldEncounterMark> EncounterMarks;
	mutable TArray<SolidRoad::FRoute> SolvedRoads;
	mutable TArray<SolidRoad::FSpan> RoadSpans;
	mutable TArray<TArray<int32>> RoadSpansByCell;

protected:
	bool WorldXYToCell(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, int32& OutMapX, int32& OutMapY) const;
	void BuildRoads(const TArray<SolidRoad::FDefinition>& Definitions) const;
	bool CellsToWorldXY(const TArray<FIntPoint>& InCells, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;
	void ResetLoad() const;
	/** Turn one chart mark into the named cell the queries read. A custom note and an encounter are skipped. */
	bool RememberChartMark(const FSolidWorldChartMark& Mark, FIntPoint Cell) const;
	/** One location per town id. Two cells with the same name are one town. Iglin is the start. */
	void CollectNamedTowns() const;
	static ESolidBiome MarkerToBiome(TCHAR Marker);
	static bool IsGridMarker(TCHAR Marker);
	/** Location id of a named town on this cell, or INDEX_NONE. */
	int32 ResolveLocationId(int32 MapX, int32 MapY) const;
};
