#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SolidRoad.h"
#include "SolidTerrainTypes.h"
#include "SolidWorldMap.generated.h"

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

	/** Map cells that resolve to this id. A two-digit run is two cells. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	TArray<FIntPoint> Cells;
};

/**
 * ASCII WorldMap.txt biome overlay (128×64 markers + color key + numbered locations).
 * Scaled across the TerrainMap world bounds. The world stays square, so a cell is
 * 8 m east–west and 16 m north–south. Two digits next to each other on a row are
 * one location number, read from the left, and each character is still one cell.
 * A longer run is taken in pairs. Location 0 is the starting town.
 * A legacy Z marker is the start only when the grid has no 0 cells.
 * Road lines are solved into routes when the file loads.
 */
UCLASS()
class SOLIDCORE1_API USolidWorldMap : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 MapWidth = 128;
	static constexpr int32 MapHeight = 64;

	/** WorldMap.txt marker for the sea. */
	static constexpr TCHAR SeaMarker = TEXT('S');

	/** Load Content/WorldMap.txt. Idempotent when already loaded. */
	bool LoadDefault(bool bForceReload = false);

	bool LoadFromString(const FString& Text);
	bool LoadFromFile(const FString& FilePath);

	bool IsLoaded() const { return bIsLoaded; }

	/**
	 * Marker biome at map cell (clamped). T and Z are Town; C is Mountain; digits use the location key.
	 * R is River for the whole cell. SampleBiome paints only the channel.
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
	 * An R cell is a 4 m river. The dry part of that square uses the neighboring biome.
	 * A solved road paints its strip as Road, a brown close to the town colour.
	 * River water stays river, and a mountain or the sea keeps its own shelf.
	 */
	ESolidBiome SampleBiome(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** Solved routes, in file order. A route with no cells had a missing end. */
	const TArray<SolidRoad::FRoute>& GetRoads() const { return Roads; }

	/** True when the point lies on a solved road strip. Does not change the biome by itself. */
	bool IsOnRoad(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** True when this cell is marked T, Z, or a location digit. A road strip is not a town. */
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

	/** Row-major MapHeight * MapWidth markers (letters uppercased, digits kept). */
	UPROPERTY()
	TArray<uint8> Markers;

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
