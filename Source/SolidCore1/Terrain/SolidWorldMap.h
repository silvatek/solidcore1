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
	LairEntrance,
	LairExit,
};

/** One ! cell named by a description line. Row and column in the file count from 1. */
struct FSolidWorldBang
{
	ESolidWorldBang Kind = ESolidWorldBang::None;
	int32 LocationId = INDEX_NONE;
	FString Name;
	ESolidStrongholdType StrongholdType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize StrongholdSize = ESolidStrongholdSize::Meagre;
	FIntPoint Cell = FIntPoint::ZeroValue;
};

/**
 * One numbered place in WorldMap.txt (`0` = Iglin, `1` = Relion, …).
 * Grid digits use Biome; the captain starts on location 0.
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

	/** Text before the first comma, e.g. "Starting village". */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	FString Role;

	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	ESolidBiome Biome = ESolidBiome::Town;

	/** Map cells that resolve to this id. A two-digit run is two cells. Each named ! is one cell. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	TArray<FIntPoint> Cells;
};

/**
 * ASCII WorldMap.txt biome overlay (128×64 markers + color key + numbered locations).
 * Scaled across the TerrainMap world bounds. The world stays square, so a cell is
 * 8 m east–west and 16 m north–south. Two digits next to each other on a row are
 * one location number, read from the left, and each character is still one cell.
 * A longer run is taken in pairs. A ! is one location cell, named by a line
 * "! 8,21 = 0": row 8, column 21, counting from 1 at the top-left of the grid.
 * "! 8,21 = Town Iglin", "! 9,12 = Stronghold MeagreDungeon",
 * "! 10,11 = LairEntrance Iglin tunnels", and "! 11,40 = LairExit Iglin tunnels"
 * name the same cell by type. Anything after # is a comment.
 * A ! with no line is not a location. Location 0 is the starting town.
 * A legacy Z marker is the start only when the grid has no 0 cells.
 * A road end is a location id, the letter C, a town name, or a stronghold name.
 * Road lines are solved into routes when the file loads.
 */
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

	/** WorldMap.txt marker for a town cell that is not a numbered location. A small circle, not the letter T. */
	static constexpr TCHAR TownMarker = TEXT('o');

	/** WorldMap.txt marker for forest. A hatch, not the letter F. */
	static constexpr TCHAR ForestMarker = TEXT('#');

	/** WorldMap.txt marker for a river channel. A star, not the letter R. */
	static constexpr TCHAR RiverMarker = TEXT('*');

	/** WorldMap.txt marker for desert. A peak, not the letter D. */
	static constexpr TCHAR DesertMarker = TEXT('^');

	/**
	 * One described cell. A line "! row,column = id" or "! row,column = Town Name"
	 * is a location. Stronghold, LairEntrance, and LairExit use the same coordinates.
	 * Row and column count from 1 at the top-left of the grid.
	 */
	static constexpr TCHAR LocationMarker = TEXT('!');

	/**
	 * Legacy Iglin tunnel mouths. A LairEntrance ! is the same X end, and a
	 * LairExit ! is the same Y end. Letters and those lines use the mountain shelf.
	 * They are not towns, locations, or the cave.
	 */
	static constexpr TCHAR XMarker = TEXT('X');
	static constexpr TCHAR YMarker = TEXT('Y');

	/** Load Content/WorldMap.txt. Idempotent when already loaded. */
	bool LoadDefault(bool bForceReload = false);

	bool LoadFromString(const FString& Text);
	bool LoadFromFile(const FString& FilePath);

	bool IsLoaded() const { return bIsLoaded; }

	/**
	 * Marker biome at map cell (clamped). The town marker and Z are Town; the desert marker is Desert; C, X, and Y are Mountain; digits and a named ! use the location key.
	 * A ! with no coordinate line is Grassland. The river marker is River for the whole cell. SampleBiome paints only the channel.
	 */
	ESolidBiome GetBiomeAtCell(int32 MapX, int32 MapY) const;

	/** Marker character at a cell, or 0 when the map is not loaded. Digits stay digits. */
	TCHAR GetMarkerAtCell(int32 MapX, int32 MapY) const;

	/** Location id on this cell, or INDEX_NONE when the marker is not a digit. A pair on the row is one id. */
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

	/** True when this cell is the town marker, Z, a location digit, or a ! named by a coordinate line. A road strip is not a town. */
	bool IsMarkedTown(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** Display / material color from the key (or built-in defaults). */
	FLinearColor GetBiomeColor(ESolidBiome Biome) const;

	bool HasStartTown() const { return bHasStartTown; }

	/**
	 * World XY of the starting-town centroid, using the same world rectangle as SampleBiome.
	 * Location 0 when the grid has any 0 cells; otherwise legacy Z cells.
	 * Returns false if neither was found.
	 */
	bool GetStartTownWorldXY(FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	int32 GetStartTownCellCount() const { return StartTownCells.Num(); }

	int32 GetLocationCount() const { return Locations.Num(); }

	/** Copy of a numbered location, including its map cells. */
	bool FindLocation(int32 Id, FSolidWorldLocation& OutLocation) const;

	/** The description of a ! cell, or false when that cell has none. */
	bool FindBang(int32 MapX, int32 MapY, FSolidWorldBang& Out) const;

	/** True when a ! line names this cell as a stronghold. */
	bool StrongholdAtCell(int32 MapX, int32 MapY, FSolidStronghold& Out) const;

	/** Every numbered location, sorted by id. */
	void CollectLocations(TArray<FSolidWorldLocation>& OutLocations) const;

	/** Centroid of one location's cells. Same world rectangle as SampleBiome. */
	bool GetLocationWorldXY(int32 Id, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** World XY of one map cell center. Same rectangle as SampleBiome. */
	bool CellToWorldXY(FIntPoint Cell, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Built-in fallback colors when the key line is missing a named tint. */
	static FLinearColor DefaultColorForBiome(ESolidBiome Biome);

protected:
	bool ParseKeyLine(const FString& Line);
	bool ParseLocationLine(const FString& Line);
	bool WorldXYToCell(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY, int32& OutMapX, int32& OutMapY) const;
	void BuildRoads(const TArray<SolidRoad::FDefinition>& Definitions);
	bool CellsToWorldXY(const TArray<FIntPoint>& Cells, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;
	static ESolidBiome MarkerToBiome(TCHAR Marker);
	static bool BiomeFromName(const FString& Name, ESolidBiome& OutBiome);
	static FLinearColor ColorFromName(const FString& ColorName);
	static bool IsGridMarker(TCHAR Marker);

	/**
	 * Location number under a digit. A pair on the same row, read east to west
	 * (left to right in the file), is one two-digit id. A lone digit is itself.
	 */
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

	/** Map cells for the start: location 0, or legacy Z when no 0 cells exist. */
	UPROPERTY()
	TArray<FIntPoint> StartTownCells;

	UPROPERTY()
	TMap<int32, FSolidWorldLocation> Locations;

	TArray<SolidRoad::FRoute> Roads;
	TArray<SolidRoad::FSpan> RoadSpans;
	/** Span indexes that can paint each cell, including the cells beside the route. */
	TArray<TArray<int32>> RoadSpansByCell;
};
