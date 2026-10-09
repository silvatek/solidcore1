#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SolidTerrainTypes.h"
#include "SolidWorldMap.generated.h"

/**
 * ASCII WorldMap.txt biome overlay (64×64 markers + color key).
 * Scaled across the TerrainMap world bounds; Z cells mark the starting town.
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

	/** Marker biome at map cell (clamped). Z and T both return Town. */
	ESolidBiome GetBiomeAtCell(int32 MapX, int32 MapY) const;

	/**
	 * Sample biome for a world XY given the TerrainMap world rectangle.
	 * File row 0 maps to the north edge (max Y).
	 */
	ESolidBiome SampleBiome(float WorldX, float WorldY, FVector2D WorldMinXY, FVector2D WorldMaxXY) const;

	/** Display / material color from the key (or built-in defaults). */
	FLinearColor GetBiomeColor(ESolidBiome Biome) const;

	bool HasStartTown() const { return bHasStartTown; }

	/**
	 * World XY of the starting-town (Z) centroid, using the same world rectangle as SampleBiome.
	 * Returns false if no Z cells were found.
	 */
	bool GetStartTownWorldXY(FVector2D WorldMinXY, FVector2D WorldMaxXY, FVector2D& OutWorldXY) const;

	int32 GetStartTownCellCount() const { return StartTownCells.Num(); }

	/** Built-in fallback colors when the key line is missing a named tint. */
	static FLinearColor DefaultColorForBiome(ESolidBiome Biome);

protected:
	bool ParseKeyLine(const FString& Line);
	static ESolidBiome MarkerToBiome(TCHAR Marker);
	static FLinearColor ColorFromName(const FString& ColorName);

	UPROPERTY()
	bool bIsLoaded = false;

	/** Row-major MapHeight * MapWidth markers (uppercase). */
	UPROPERTY()
	TArray<uint8> Markers;

	UPROPERTY()
	TMap<ESolidBiome, FLinearColor> BiomeColors;

	UPROPERTY()
	bool bHasStartTown = false;

	/** Map cells with marker Z (starting town). */
	UPROPERTY()
	TArray<FIntPoint> StartTownCells;
};
