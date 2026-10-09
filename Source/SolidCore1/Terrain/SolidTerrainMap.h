#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SolidTerrainTypes.h"
#include "SolidTerrainMap.generated.h"

/**
 * Fixed 2D TerrainPoint grid built once at startup. Chunk meshes sample Height from this map
 * (bilinear) so gameplay data and rendered landscape share one source of truth.
 */
UCLASS(BlueprintType)
class SOLIDCORE1_API USolidTerrainMap : public UObject
{
	GENERATED_BODY()

public:
	/** Points along each axis (total cells = Width * Height). */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	int32 GridWidth = 257;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	int32 GridHeight = 257;

	/** World cm between neighboring points. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float PointSpacing = 200.f;

	/** World XY of index (0, 0). Grid is centered on the origin by default. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	FVector2D OriginXY = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	int32 Seed = 1337;

	/** True after Build() fills Points. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	bool bIsBuilt = false;

	/**
	 * Allocate and fill the TerrainPoint grid from deterministic noise + biome rules.
	 * Safe to call once; ignored if already built unless bForceRebuild.
	 */
	void Build(
		int32 InSeed,
		float FrequencyScale,
		float Amplitude,
		float BaseHeight,
		int32 InGridWidth = 257,
		int32 InGridHeight = 257,
		float InPointSpacing = 200.f,
		bool bForceRebuild = false);

	bool IsBuilt() const { return bIsBuilt && Points.Num() == GridWidth * GridHeight; }

	int32 GetPointCount() const { return Points.Num(); }

	const TArray<FSolidTerrainPoint>& GetPoints() const { return Points; }

	/** Row-major access; clamps indices. */
	const FSolidTerrainPoint& GetPoint(int32 IndexX, int32 IndexY) const;

	/** Nearest grid point (no interpolation). */
	const FSolidTerrainPoint& GetNearestPoint(float WorldX, float WorldY) const;

	/** Bilinear height sample in world cm. */
	float SampleHeight(float WorldX, float WorldY) const;

	/**
	 * Gameplay sample: bilinear Height / Threat / Fog, nearest-neighbor Biome.
	 * Coordinates on the result are the query world XY.
	 */
	FSolidTerrainPoint SamplePoint(float WorldX, float WorldY) const;

	/**
	 * Set Fog=0 on every TerrainPoint within RadiusCm of (WorldX, WorldY).
	 * Used to clear exploration fog along the player's trail.
	 * @return Number of points whose Fog value changed.
	 */
	int32 ClearFogAround(float WorldX, float WorldY, float RadiusCm);

	FVector2D GetWorldMinXY() const { return OriginXY; }
	FVector2D GetWorldMaxXY() const
	{
		return OriginXY + FVector2D(
			static_cast<float>(FMath::Max(GridWidth - 1, 0)) * PointSpacing,
			static_cast<float>(FMath::Max(GridHeight - 1, 0)) * PointSpacing);
	}

protected:
	UPROPERTY()
	TArray<FSolidTerrainPoint> Points;

	void WorldToIndex(float WorldX, float WorldY, int32& OutX, int32& OutY) const;
	ESolidBiome ChooseBiome(float WorldX, float WorldY, float HeightNorm, int32 InSeed) const;
	void FillThreatAndFog(FSolidTerrainPoint& Point, float HeightNorm, int32 InSeed) const;
};
