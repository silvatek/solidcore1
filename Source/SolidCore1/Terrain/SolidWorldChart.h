#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SolidRoad.h"
#include "SolidStronghold.h"
#include "SolidTerrainTypes.h"
#include "SolidWorldChart.generated.h"

/** How wide a chart road is. The route is solved when the chart is asked. */
UENUM()
enum class ESolidWorldChartRoadType : uint8
{
	Trail,
	Track,
	Road,
	Highway,
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
 * The shipped world. Cells hold the old squares. Points hold the biome of each
 * terrain point. Marks and roads hold the towns, strongholds, and roads.
 * The same object answers the map queries: biomes, towns, and solved routes.
 * Those solved routes are not saved with the asset.
 * The game builds the world from /Game/World/WorldChart.
 * World +Y is north and world +X is west, for a terrain point and for a world cell.
 * A terrain point's row number counts up toward the north. Point row 0 is the south edge.
 * A world cell's row number counts up toward the south. Cell row 0 is the north edge.
 * Column 0 is west on both.
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
	 * A Road: line is ignored.
	 */
	bool ImportFromString(const FString& Text);

	/** Copy /Game/World/WorldChart onto this chart and solve it. Idempotent when already solved. */
	bool LoadDefault(bool bForceReload = false);

	/**
	 * Fill the solved queries from map text. A colour line is ignored.
	 * A Road: line is ignored.
	 * Digits and tunnel letters stay markers. An encounter is not in the text.
	 */
	bool LoadFromString(const FString& Text);

	/**
	 * Copy that chart's cells, marks, roads, and terrain points onto this one, then solve the routes.
	 * A chart saved as cells has no points yet. Solving copies the play biomes onto them.
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

	/**
	 * Town under a world XY, or INDEX_NONE. The town is the painted town ground
	 * connected to its marker. Town paint that does not meet a marker is not a town.
	 * Same rectangle as SampleBiome.
	 */
	int32 SampleLocationId(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/**
	 * Sample biome for a world XY given the terrain world rectangle.
	 * The ground is the terrain point under that position. A solved road paints
	 * its strip as Road where that point shares the road's shelf. River water
	 * stays river. A stronghold that names a shelf keeps the point's biome.
	 */
	ESolidBiome SampleBiome(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/**
	 * Biome of the terrain point under this cell, the point at the cell centre.
	 * A road step reads this. The cell's own biome is GetBiomeAtCell.
	 */
	ESolidBiome GetPointBiomeAtCell(int32 MapX, int32 MapY) const;

	/** Deprecated. Prefer GetRoutedRoads. Solved routes, in file order. A route with no cells had a missing end. */
	const TArray<SolidRoad::FRoute>& GetRoads() const { return SolvedRoads; }

	/** Preferred road data. Each road keeps the terrain points of its route. */
	const TArray<SolidRoad::FSolidRoad>& GetRoutedRoads() const { return RoutedRoads; }

	/** True when the point lies within the routed road's width of its terrain points. */
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

	/** Map cell under a world XY. Column 0 is west, row 0 is north. */
	bool CellAtWorld(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, FIntPoint& OutCell) const;

	/** The biome's colour. The same answer as GetBiomeColor. */
	static FLinearColor DefaultColorForBiome(ESolidBiome Biome);

	/** Word the road line uses for this type: trail, track, road, or highway. */
	static const TCHAR* RoadTypeName(ESolidWorldChartRoadType Type);

	static ESolidWorldChartRoadType ChartRoadType(SolidRoad::EType Type);

	/**
	 * Centre of one map cell, in the chart's centimetre rectangle.
	 * Column 0 is west, row 0 is north.
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

	/**
	 * Terrain point nearest this position. X increases west from the east edge.
	 * Y increases north from the south edge. The same rectangle as SampleBiome.
	 */
	bool TerrainPointAt(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, FIntPoint& OutPoint) const;

	/** World position of one terrain point. The same rectangle as TerrainPointAt. */
	bool TerrainPointToWorld(FIntPoint Point, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Biome stored on that terrain point. The road strip is not part of it. */
	ESolidBiome GetPointBiome(FIntPoint Point) const;

	/**
	 * Points a round brush covers. Radius 0 is the one point, 2 m across.
	 * A larger radius reaches that many points further. A point is inside when
	 * its distance in point steps is at most Radius. Centres join in a straight
	 * run, so a jump still lays one stroke.
	 */
	static void BrushPoints(const TArray<FIntPoint>& Centres, int32 Radius, TArray<FIntPoint>& OutPoints);

	/** Inclusive axis-aligned rectangle of terrain points. Edges run north–south and east–west. */
	static void PointRectangle(FIntPoint From, FIntPoint To, TArray<FIntPoint>& OutPoints);

	/**
	 * Every terrain point in this inclusive rectangle takes this biome.
	 * The points already cover the grid.
	 */
	bool SetPointRectangle(FIntPoint From, FIntPoint To, ESolidBiome Biome);

	/**
	 * Paint the points under a round brush. One call is one undo step.
	 * The cell array stays. A marker point the brush covers stays town or grassland.
	 * Swamp and road are rejected.
	 */
	bool PaintBrush(const TArray<FIntPoint>& Centres, int32 Radius, ESolidBiome Biome, FString& OutError);

	/**
	 * Fill the inclusive rectangle of terrain points. One call is one undo step.
	 * The cell array stays. A marker point inside stays town or grassland.
	 */
	bool PaintPointRectangle(FIntPoint From, FIntPoint To, ESolidBiome Biome, FString& OutError);

	/** Put a SolidTowns town on this cell. The same town name moves here. */
	bool PlaceTown(FIntPoint Cell, const FString& Name, FString& OutError);

	/** Put a stronghold the game already has. The same size moves here. */
	bool PlaceStronghold(FIntPoint Cell, ESolidStrongholdSize Size, ESolidStrongholdType Type, FString& OutError);

	/** Put a stronghold site. The same size moves here. A site is not a road end. */
	bool PlaceSite(FIntPoint Cell, ESolidStrongholdSize Size, FString& OutError);

	/** Put one end of a lair the game already has. The same lair and end move here. */
	bool PlaceLairMouth(FIntPoint Cell, const FString& LairName, bool bEntrance, FString& OutError);

	/**
	 * Put a custom note on this cell. The same text moves here.
	 * The note is one line, at most 48 characters. It does not change the paint.
	 */
	bool PlaceCustom(FIntPoint Cell, const FString& Note, FString& OutError);

	/**
	 * Put a catalog encounter on this cell. The same name moves here.
	 * The name is the one in SolidEncounter::StaticEncounters. The paint stays.
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

	/** Terrain points on one side. The same count as the terrain map's default grid. */
	static constexpr int32 PointAxis = 513;

	/** Marker biome of each cell. Row-major. Row 0 is north, so a higher row is further south. Column 0 is west. A ! is grassland. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<ESolidBiome> Cells;

	/**
	 * Biome of each terrain point. Row-major. Point row 0 is the south edge, so a
	 * higher row is further north. Index 0 is the south-east corner. World +Y is
	 * north, the same direction as a cell. A cell row counts the other way.
	 * Empty on a chart saved as cells; loading copies those cells onto the points.
	 * Mutable because solving a const chart fills it.
	 */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	mutable TArray<ESolidBiome> Points;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<FSolidWorldChartMark> Marks;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<FSolidWorldChartRoad> Roads;

private:
	/** Cells, marks, and roads from before one edit. Not saved with the asset. */
	struct FSnapshot
	{
		TArray<ESolidBiome> Cells;
		TArray<ESolidBiome> Points;
		TArray<FSolidWorldChartMark> Marks;
		TArray<FSolidWorldChartRoad> Roads;
	};

	void PushUndo();
	/** Copy each cell's play biome onto the points, then set each marker point. The road strip is not copied. */
	void CopyPlayOntoPoints(bool bForce, bool bUseStoredMarks) const;
	/** The biome a cell shows in play at this position, without the road strip. */
	ESolidBiome SampleCellPlay(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;
	void SetPointsInCells(const TArray<int32>& CellIndexes, ESolidBiome Biome) const;
	void ApplyMarkerPoint(const FVector2D& PositionCm, ESolidWorldChartMarkKind Kind) const;
	/** Put a marker point back to the biome of another point in its cell. */
	void ReleaseMarkerPoint(const FVector2D& PositionCm) const;
	/** Write these terrain points, then put each covered marker point back. One undo step. */
	bool PaintPoints(const TArray<FIntPoint>& Covered, ESolidBiome Biome, FString& OutError);
	/** Claim each run of town points for the first marker that touches it. */
	void AssignTownPoints() const;
	bool EnsurePoints() const;
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

	/** A town marker, in the order the chart stored it. The first seed claims a shared region. */
	struct FTownSeed
	{
		int32 LocationId = INDEX_NONE;
		int32 Column = 0;
		int32 Row = 0;
	};

	mutable TArray<FTownSeed> TownSeeds;
	/** Location id of each terrain point, or INDEX_NONE. Same order as Points. */
	mutable TArray<int32> PointTowns;
	/** Deprecated. Prefer RoutedRoads. One solved route per listed road, including an empty route. */
	mutable TArray<SolidRoad::FRoute> SolvedRoads;
	/** Preferred road data. Listed roads that found a route. Empty until the chart loads. */
	mutable TArray<SolidRoad::FSolidRoad> RoutedRoads;
	/** Deprecated. The strip is measured from RoutedRoads. These stay empty. */
	mutable TArray<SolidRoad::FSpan> RoadSpans;
	/** Deprecated. The strip is measured from RoutedRoads. This stays empty. */
	mutable TArray<TArray<int32>> RoadSpansByCell;

protected:
	bool WorldXYToCell(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, int32& OutMapX, int32& OutMapY) const;
	/** Clear the solved roads. */
	void ResetRoads() const;
	/** Route one listed road. */
	void BuildRoad(SolidRoad::FSolidRoad* Road) const;
	/** Build each listed road and solve it. */
	void BuildListedRoads() const;
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
