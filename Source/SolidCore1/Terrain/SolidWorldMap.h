#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
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

	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	FString Name;

	/** Text before the first comma, e.g. "Starting village". */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	FString Role;

	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	ESolidBiome Biome = ESolidBiome::Town;

	/** Map cells whose marker is the digit for this id. */
	UPROPERTY(BlueprintReadOnly, Category = "WorldMap")
	TArray<FIntPoint> Cells;
};

/**
 * ASCII WorldMap.txt biome overlay (64×64 markers + color key + numbered locations).
 * Scaled across the TerrainMap world bounds. Location 0 is the starting town.
 * A legacy Z marker is the start only when the grid has no 0 cells.
 */
UCLASS()
class SOLIDCORE1_API USolidWorldMap : public UObject
{
	GENERATED_BODY()

public:
	static constexpr int32 MapWidth = 64;
	static constexpr int32 MapHeight = 64;

	/** Load Content/WorldMap.txt (fallback Source path). Idempotent when already loaded. */
	bool LoadDefault(bool bForceReload = false);

	bool LoadFromString(const FString& Text);
	bool LoadFromFile(const FString& FilePath);

	bool IsLoaded() const { return bIsLoaded; }

	/** Marker biome at map cell (clamped). T and Z are Town; digits use the location key. */
	ESolidBiome GetBiomeAtCell(int32 MapX, int32 MapY) const;

	/**
	 * Sample biome for a world XY given the TerrainMap world rectangle.
	 * File row 0 → north (max Y). File column 0 → east (max X) so the text
	 * matches the in-world layout when viewed with north up.
	 */
	ESolidBiome SampleBiome(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

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

	/** Centroid of one location's cells. Same world rectangle as SampleBiome. */
	bool GetLocationWorldXY(int32 Id, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	/** Built-in fallback colors when the key line is missing a named tint. */
	static FLinearColor DefaultColorForBiome(ESolidBiome Biome);

protected:
	bool ParseKeyLine(const FString& Line);
	bool ParseLocationLine(const FString& Line);
	bool CellsToWorldXY(const TArray<FIntPoint>& Cells, FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;
	static ESolidBiome MarkerToBiome(TCHAR Marker);
	static bool BiomeFromName(const FString& Name, ESolidBiome& OutBiome);
	static FLinearColor ColorFromName(const FString& ColorName);
	static bool IsGridMarker(TCHAR Marker);

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
};
