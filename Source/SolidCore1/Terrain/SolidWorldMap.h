#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SolidRoad.h"
#include "SolidStronghold.h"
#include "SolidTerrainTypes.h"
#include "SolidWorldMap.generated.h"

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

/** One ! cell named by a description line. Row and column in the file count from 1. */
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
 * One named town in WorldMap.txt. Iglin is location 0 and is where the captain starts.
 * Biome is Town. WorldMap.txt does not override the role or the biome.
 */
USTRUCT(BlueprintType)
struct FSolidWorldLocation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	int32 Id = INDEX_NONE;

	/** From SolidTowns, matched by Id. WorldMap.txt does not supply this. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	FString Name;

	/** Unused. Older maps wrote a role here; a named town leaves this empty. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	FString Role;

	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	ESolidBiome Biome = ESolidBiome::Town;

	/** Map cells that resolve to this id. Each named town ! is one cell. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	TArray<FIntPoint> Cells;
};

/**
 * ASCII WorldMap.txt biome overlay (128×64 markers + color key + named locations).
 * Scaled across the TerrainMap world bounds. The world stays square, so a cell is
 * 8 m east–west and 16 m north–south.
 * A ! is one cell, named by a line such as "! 8,21 = Town Iglin": row 8, column 21,
 * counting from 1 at the top-left of the grid.
 * "! 9,12 = Stronghold MeagreDungeon", "! 12,12 = StrongholdSite (Humble)",
 * "! 10,11 = LairEntrance Iglin tunnels", and "! 11,40 = LairExit Iglin tunnels"
 * name that cell. A site names a size and no type.
 * Anything after # is a comment. A ! with no line is not a location.
 * Location 0 is the starting town.
 * A road end is a town name or a stronghold name.
 * Road lines are solved into routes when the file loads.
 * A world chart holds the same cells, marks, and roads. LoadDefault reads /Game/World/WorldChart.
 */
class USolidWorldChart;
struct FSolidWorldChartMark;

UCLASS()
class SOLIDCORE1_API USolidWorldMap : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 MapWidth = 128;
	static constexpr int32 MapHeight = 64;

	/** WorldMap.txt marker for the sea. A wave, not the letter S. */
	static constexpr TCHAR SeaMarker = TEXT('~');

	/** WorldMap.txt marker for grassland. A dot, not the letter G. */
	static constexpr TCHAR GrassMarker = TEXT('.');

	/** WorldMap.txt marker for a town cell that is not a named location. A small circle, not the letter T. */
	static constexpr TCHAR TownMarker = TEXT('o');

	/** WorldMap.txt marker for forest. A hatch, not the letter F. */
	static constexpr TCHAR ForestMarker = TEXT('#');

	/** WorldMap.txt marker for a river channel. A star, not the letter R. */
	static constexpr TCHAR RiverMarker = TEXT('*');

	/** WorldMap.txt marker for desert. A peak, not the letter D. */
	static constexpr TCHAR DesertMarker = TEXT('^');

	/**
	 * One described cell. "! row,column = Town Name" is a location.
	 * Stronghold, StrongholdSite, LairEntrance, and LairExit use the same coordinates.
	 * Row and column count from 1 at the top-left of the grid.
	 */
	static constexpr TCHAR LocationMarker = TEXT('!');

	/**
	 * Interior Iglin tunnel ends. A LairEntrance ! is the X end, and a LairExit !
	 * is the Y end. Those ! cells use the mountain shelf. The letters themselves
	 * are not mouths, towns, or the cave on the world map.
	 */
	static constexpr TCHAR XMarker = TEXT('X');
	static constexpr TCHAR YMarker = TEXT('Y');

	/** Load /Game/World/WorldChart. A missing asset fails. Idempotent when already loaded. */
	bool LoadDefault(bool bForceReload = false);

	bool LoadFromString(const FString& Text);
	bool LoadFromFile(const FString& FilePath);

	/**
	 * Build this map from a world chart. Each mark's position becomes a cell once.
	 * Biomes, marks, and solved roads match the text file that filled that chart.
	 * A custom note is not a mark on this map. LoadDefault reads the shipped chart.
	 */
	bool LoadFromChart(const USolidWorldChart* Chart);

	bool IsLoaded() const { return bIsLoaded; }

	/**
	 * Marker biome at map cell (clamped). The town marker is Town; the desert marker is Desert; M is Mountain.
	 * A named town ! uses the Town biome. A lair ! is Mountain.
	 * A stronghold ! uses the shelf that stronghold names. One with no shelf keeps the marker biome.
	 * A stronghold site keeps the marker biome. A ! with no description is Grassland.
	 * The river marker is River for the whole cell. SampleBiome paints only the channel.
	 */
	ESolidBiome GetBiomeAtCell(int32 MapX, int32 MapY) const;

	/**
	 * Marker biome at a cell, before a named town, a stronghold shelf, or a lair mouth replaces it.
	 * A ! is grassland. Swamp and road are not marker biomes.
	 */
	ESolidBiome GetMarkerBiomeAtCell(int32 MapX, int32 MapY) const;

	/** Marker character at a cell, or 0 when the map is not loaded. Digits stay digits. */
	TCHAR GetMarkerAtCell(int32 MapX, int32 MapY) const;

	/** Location id on this cell, or INDEX_NONE when it is not a named town !. */
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
	const TArray<SolidRoad::FRoute>& GetRoads() const { return Roads; }

	/** True when the point lies on a solved road strip. Does not change the biome by itself. */
	bool IsOnRoad(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** True when this cell is the town marker, or a ! named as a town. A road strip is not a town. */
	bool IsMarkedTown(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** Display / material color from the key (or built-in defaults). */
	FLinearColor GetBiomeColor(ESolidBiome Biome) const;

	/**
	 * World XY of the starting-town centroid, using the same world rectangle as SampleBiome.
	 * Location 0. Returns false when that town has no cells.
	 */
	bool GetStartTownWorldXY(FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Copy of a numbered location, including its map cells. */
	bool FindLocation(int32 Id, FSolidWorldLocation& OutLocation) const;

	/** The description of a ! cell, or false when that cell has none. */
	bool FindBang(int32 MapX, int32 MapY, FSolidWorldBang& Out) const;

	/** True when a ! line names this cell as a stronghold. A stronghold site is not one. */
	bool StrongholdAtCell(int32 MapX, int32 MapY, ESolidStrongholdType& OutType, ESolidStrongholdSize& OutSize) const;

	/** True when a ! line names this cell as a stronghold site. OutSize is that size. */
	bool StrongholdSiteAtCell(int32 MapX, int32 MapY, ESolidStrongholdSize& OutSize) const;

	/** Every numbered location, sorted by id. */
	void CollectLocations(TArray<FSolidWorldLocation>& OutLocations) const;

	/** Centroid of one location's cells. Same world rectangle as SampleBiome. */
	bool GetLocationWorldXY(int32 Id, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** World XY of one map cell center. Same rectangle as SampleBiome. */
	bool CellToWorldXY(FIntPoint Cell, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Map cell under a world XY. Column 0 is east, row 0 is north. */
	bool CellAtWorld(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, FIntPoint& OutCell) const;

	/** Built-in fallback colors when the key line is missing a named tint. */
	static FLinearColor DefaultColorForBiome(ESolidBiome Biome);

protected:
	bool ParseKeyLine(const FString& Line);
	bool WorldXYToCell(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, int32& OutMapX, int32& OutMapY) const;
	void BuildRoads(const TArray<SolidRoad::FDefinition>& Definitions);
	bool CellsToWorldXY(const TArray<FIntPoint>& Cells, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;
	void ResetLoad();
	/** Turn one chart mark into the ! description the map already reads. An unknown name is skipped. */
	bool RememberChartMark(const FSolidWorldChartMark& Mark, FIntPoint Cell);
	/** One location per town id. Two cells with the same name are one town. Iglin is the start. */
	void CollectNamedTowns();
	/** Give every biome a colour. A colour the text file already set stays. */
	void AddDefaultBiomeColors();
	static ESolidBiome MarkerToBiome(TCHAR Marker);
	static FLinearColor ColorFromName(const FString& ColorName);
	static bool IsGridMarker(TCHAR Marker);

	/** Location id of a named town ! on this cell, or INDEX_NONE. */
	int32 ResolveLocationId(int32 MapX, int32 MapY) const;

	UPROPERTY()
	bool bIsLoaded = false;

	/** Row-major MapHeight * MapWidth markers (letters uppercased, digits and ! kept). */
	UPROPERTY()
	TArray<uint8> Markers;

	/** Location id for each ! cell, or INDEX_NONE. Same order as Markers. */
	TArray<int32> BangLocationIds;

	/** Description of each ! cell that has a line. Key is row-major cell index. */
	TMap<int32, FSolidWorldBang> BangMarks;

	UPROPERTY()
	TMap<ESolidBiome, FLinearColor> BiomeColors;

	UPROPERTY()
	bool bHasStartTown = false;

	/** Map cells for location 0, the starting town. */
	UPROPERTY()
	TArray<FIntPoint> StartTownCells;

	UPROPERTY()
	TMap<int32, FSolidWorldLocation> Locations;

	TArray<SolidRoad::FRoute> Roads;
	TArray<SolidRoad::FSpan> RoadSpans;
	/** Span indexes that can paint each cell, including the cells beside the route. */
	TArray<TArray<int32>> RoadSpansByCell;
};
