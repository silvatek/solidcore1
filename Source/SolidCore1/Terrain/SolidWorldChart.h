#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SolidRoad.h"
#include "SolidStronghold.h"
#include "SolidTerrainTypes.h"
#include "SolidWorldChart.generated.h"

/**
 * The shipped world. Cells hold the old squares. Points hold the biome of each
 * terrain point. Marks hold the towns, strongholds, and the other placed names.
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

	/** Editor path of the shipped chart. */
	static const TCHAR* ShippedPath();

	/** Copy /Game/World/WorldChart onto this chart and solve it. Idempotent when already solved. */
	bool LoadDefault(bool bForceReload = false);

	/**
	 * Copy that chart's marks and terrain points onto this one, then solve the routes.
	 * The solve needs the terrain points to cover the grid.
	 * Copying this chart only solves.
	 */
	bool LoadFromChart(const USolidWorldChart* Chart);

	/** Encounter marks on the chart. */
	int32 CollectEncounterMarks(TArray<FSolidWorldChartMark>& OutMarks) const;

	bool IsLoaded() const { return bIsLoaded; }

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

	/** Each road keeps the terrain points of its route. */
	const TArray<SolidRoad::FSolidRoad>& GetRoutedRoads() const { return RoutedRoads; }

	/** True when the point lies within the routed road's width of its terrain points. */
	bool IsOnRoad(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** Tint for this biome. */
	FLinearColor GetBiomeColor(ESolidBiome Biome) const;

	/** Deprecated. Cell of this town's marker. False when the chart has no such town. */
	bool TownCell(int32 TownId, FIntPoint& OutCell) const;

	/** This town's mark. False when the chart has no such town. */
	bool FindTownMarker(int32 TownId, FSolidWorldChartMark& OutMark) const;

	/** This lair mouth. Kind is LairEntrance or LairExit. False when the chart has no such mouth. */
	bool FindLairMarker(const FString& LairId, ESolidWorldChartMarkKind Kind, FSolidWorldChartMark& OutMark) const;

	/** This stronghold's mark. The name ignores spaces. False when the chart has no such stronghold. */
	bool FindStrongholdMarker(const FString& Name, FSolidWorldChartMark& OutMark) const;

	/** The stronghold site of this size. False when the chart has none. */
	bool FindStrongholdSiteMarker(ESolidStrongholdSize Size, FSolidWorldChartMark& OutMark) const;

	/** True when the mark on this terrain point is a stronghold. A site is not one. */
	bool StrongholdAtTerrain(int32 TerrainX, int32 TerrainY, ESolidStrongholdType& OutType, ESolidStrongholdSize& OutSize) const;

	/** Deprecated. World XY of this town's cell. Same world rectangle as SampleBiome. */
	bool GetLocationWorldXY(int32 Id, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Deprecated. World XY of one map cell center. Same rectangle as SampleBiome. */
	bool CellToWorldXY(FIntPoint Cell, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Deprecated. Map cell under a world XY. Column 0 is west, row 0 is north. */
	bool CellAtWorld(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, FIntPoint& OutCell) const;

	/** Deprecated. Terrain point at a cell centre. Column 0 is west, row 0 is north. */
	static void PointUnderCell(int32 CellX, int32 CellY, int32& OutTerrainX, int32& OutTerrainY);

	/** Deprecated. Cell that contains a terrain point. Column 0 is west, row 0 is north. */
	static bool CellAtTerrainPoint(FIntPoint TerrainPoint, FIntPoint& OutCell);

	/** The biome's colour. The same answer as GetBiomeColor. */
	static FLinearColor DefaultColorForBiome(ESolidBiome Biome);

	/** Word the road line uses for this type: trail, track, road, or highway. */
	static const TCHAR* RoadTypeName(SolidRoad::EType Type);

	/**
	 * Write these terrain points. Swamp and road are rejected.
	 * One call is one undo step. A mark on a point does not change that paint.
	 */
	bool PaintPoints(const TArray<FIntPoint>& Covered, ESolidBiome Biome, FString& OutError);

	/**
	 * Terrain point nearest this position. TerrainPoint.X increases west from the east edge.
	 * TerrainPoint.Y increases north from the south edge. The same rectangle as SampleBiome.
	 */
	bool TerrainPointAt(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, FIntPoint& OutTerrainPoint) const;

	/** World position of one terrain point. The same rectangle as TerrainPointAt. */
	bool TerrainPointToWorld(FIntPoint TerrainPoint, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Biome stored on that terrain point. The road strip is not part of it. */
	ESolidBiome GetPointBiome(FIntPoint TerrainPoint) const;

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
	 * The cell array stays. A mark on a point does not change that paint.
	 * Swamp and road are rejected.
	 */
	bool PaintBrush(const TArray<FIntPoint>& Centres, int32 Radius, ESolidBiome Biome, FString& OutError);

	/** Fill every terrain point with this biome. */
	void Init(ESolidBiome Biome);

	/**
	 * Fill the inclusive rectangle of terrain points. One call is one undo step.
	 * The cell array stays. A mark inside the rectangle does not change that paint.
	 */
	bool PaintPointRectangle(FIntPoint From, FIntPoint To, ESolidBiome Biome, FString& OutError);

	/** Put a SolidTowns town on this terrain point. The same town name moves here. */
	bool PlaceTownAt(FIntPoint TerrainPoint, const FString& Name, FString& OutError);

	/** Put a stronghold the game already has on this terrain point. The same size moves here. */
	bool PlaceStrongholdAt(FIntPoint TerrainPoint, ESolidStrongholdSize Size, ESolidStrongholdType Type, FString& OutError);

	/** Put a stronghold site on this terrain point. The same size moves here. A site is not a road end. */
	bool PlaceSiteAt(FIntPoint TerrainPoint, ESolidStrongholdSize Size, FString& OutError);

	/** Put one end of a lair the game already has on this terrain point. The same lair and end move here. */
	bool PlaceLairMouthAt(FIntPoint TerrainPoint, const FString& LairName, bool bEntrance, FString& OutError);

	/** Put a custom note on this terrain point. The same text moves here. */
	bool PlaceCustomAt(FIntPoint TerrainPoint, const FString& Note, FString& OutError);

	/** Put a catalog encounter on this terrain point. The same name moves here. */
	bool PlaceEncounterAt(FIntPoint TerrainPoint, const FString& Name, FString& OutError);

	/** Put a bridge on this terrain point. The same name moves here. */
	bool PlaceBridgeAt(FIntPoint TerrainPoint, const FString& Name, FString& OutError);

	/** Remove the mark on this terrain point. */
	bool DeleteMarkAt(FIntPoint TerrainPoint, FString& OutError);

	/** Deprecated. Index of the mark on this cell, or INDEX_NONE. */
	int32 MarkAtCell(FIntPoint Cell) const;

	/** Put back the chart from before the last paint or mark. */
	bool Undo();

	/** Terrain points on one side. The same count as the terrain map's default grid. */
	static constexpr int32 PointAxis = 513;

	/**
	 * Deprecated. The cell grid. The land is the terrain points.
	 * Marker biome of each cell. Row-major. Row 0 is north, so a higher row is further south. Column 0 is west. A ! is grassland.
	 */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<ESolidBiome> Cells;

	/**
	 * Biome of each terrain point. Row-major. Point row 0 is the south edge, so a
	 * higher row is further north. Index 0 is the south-east corner. World +Y is
	 * north, the same direction as a cell. A cell row counts the other way.
	 * Mutable because a const helper writes a point.
	 */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	mutable TArray<ESolidBiome> Points;

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	TArray<FSolidWorldChartMark> Marks;

private:
	/** Points and marks from before one edit. Not saved with the asset. */
	struct FSnapshot
	{
		TArray<ESolidBiome> Points;
		TArray<FSolidWorldChartMark> Marks;
	};

	void PushUndo();
	/** Claim each run of town points for the first town mark that touches it. */
	void AssignTownPoints() const;
	bool EnsurePoints() const;
	bool PlaceOn(FIntPoint TerrainPoint, const FSolidWorldChartMark& Mark, FString& OutError);
	/** Solve when a query finds the chart unsolved. The terrain points cover the map. */
	bool EnsureQuery() const;
	/**
	 * AssignTownPoints claims each run of town points for the town mark that touches it.
	 * BuildListedRoads routes each road in SolidRoads.h across the terrain points.
	 */
	bool BuildQuery() const;
	/** Drop the solved routes. The next query solves the points and marks again. */
	void InvalidateQuery();

	TArray<FSnapshot> UndoStack;

	/** Solved facts. Not saved with the asset. A paint, mark, or undo clears them. */
	mutable bool bIsLoaded = false;

	/** Location id of each terrain point, or INDEX_NONE. Same order as Points. */
	mutable TArray<int32> PointTowns;
	/** Listed roads that found a route. Empty until the chart loads. */
	mutable TArray<SolidRoad::FSolidRoad> RoutedRoads;

protected:
	/** Deprecated. Map cell under a world XY. Column 0 is west, row 0 is north. */
	bool WorldXYToCell(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, int32& OutCellX, int32& OutCellY) const;
	/** Clear the solved roads. */
	void ResetRoads() const;
	/** Route one listed road. */
	void BuildRoad(SolidRoad::FSolidRoad* Road) const;
	/** Build each listed road and solve it. */
	void BuildListedRoads() const;
	/** Deprecated. Mean world centre of these cells. Same rectangle as SampleBiome. */
	bool CellsToWorldXY(const TArray<FIntPoint>& InCells, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;
	void ResetLoad() const;
};
