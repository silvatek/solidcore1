#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTerrainTypes.h"
#include "SolidTerrainStreamer.generated.h"

class AExponentialHeightFog;
class ASolidTerrainChunk;
class UMaterialInterface;
class USolidTerrainMap;

/**
 * Spawns / destroys runtime procedural terrain chunks around a focus actor (usually the player pawn).
 * Heights are sampled in world XY so chunk edges match.
 */
UCLASS()
class SOLIDCORE1_API ASolidTerrainStreamer : public AActor
{
	GENERATED_BODY()

public:
	ASolidTerrainStreamer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Spawn a streamer if the world does not already have one. Safe to call often. */
	static ASolidTerrainStreamer* EnsureExists(UWorld* World);

	/** Read-only helpers for the debug HUD (no streaming side effects). */
	int32 GetLoadedChunkCount() const { return LoadedChunks.Num(); }
	float GetHeightAt(const FVector& WorldLocation) const { return SampleHeightAtWorld(WorldLocation); }
	FIntPoint GetChunkCoordAt(const FVector& WorldLocation) const { return WorldToChunkCoord(WorldLocation); }
	UMaterialInterface* GetActiveMaterial() const
	{
		return ResolvedTerrainMaterial ? ResolvedTerrainMaterial.Get() : TerrainMaterial.Get();
	}
	USolidTerrainMap* GetTerrainMap() const { return TerrainMap; }
	FSolidTerrainPoint GetTerrainPointAt(const FVector& WorldLocation) const;

	/** Smoothed fog amount currently applied to height fog [0, 1]. */
	float GetRenderedFogAmount() const { return RenderedFogAmount; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "500.0"))
	float ChunkWorldSize = 6400.f;

	/** Quads along one chunk edge (verts = QuadsPerSide + 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "4", ClampMax = "128"))
	int32 QuadsPerSide = 32;

	/** Chebyshev radius in chunks around the focus (0 => 1 chunk, 2 => 5x5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0", ClampMax = "12"))
	int32 ViewRadiusChunks = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float FrequencyScale = 0.00012f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float Amplitude = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float BaseHeight = 0.f;

	/**
	 * Raise invisible collision above the visible mesh (cm). Capsules sink into complex
	 * procedural collision; this keeps feet on the rendered surface.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0"))
	float CollisionHeightBias = 0.f;

	/** Optional material override. Empty => Fab Mat_025_grass, then FlatCol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

	/** Lighter grass shade for the FlatCol Base Color blend. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FLinearColor GrassColor = FLinearColor(0.12f, 0.28f, 0.07f);

	/** Darker grass shade for the FlatCol Base Color blend. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FLinearColor GrassDarkColor = FLinearColor(0.04f, 0.11f, 0.03f);

	/** Seconds between streamer updates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.05"))
	float UpdateIntervalSeconds = 0.25f;

	/** If set, stream around this actor; otherwise uses the first player pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TWeakObjectPtr<AActor> FocusActor;

	/** Keep the focus pawn on the terrain surface (retries while falling / far below). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bSnapFocusToTerrain = true;

	/** Hide Open World landscape and disable its collision so the pawn walks on procedural chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bDisableLandscapeActors = true;

	/** Extra cm above the hit surface when snapping (helps with complex-collision sink). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0"))
	float SnapHeightPadding = 4.f;

	/** TerrainPoint cells along each map axis (built once at BeginPlay). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Map", meta = (ClampMin = "8", ClampMax = "1025"))
	int32 TerrainMapSize = 257;

	/** World cm between TerrainPoints. Match chunk vert step (ChunkWorldSize / QuadsPerSide) for crisp sampling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Map", meta = (ClampMin = "50.0"))
	float TerrainPointSpacing = 200.f;

	/** Drive Exponential Height Fog from TerrainPoint.Fog samples. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog")
	bool bRenderTerrainFog = true;

	/** How quickly rendered fog follows TerrainPoint samples. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.1"))
	float FogInterpSpeed = 2.5f;

	/** FogDensity at TerrainPoint.Fog == 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.0"))
	float FogDensityAtFull = 0.045f;

	/** FogMaxOpacity at TerrainPoint.Fog == 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogMaxOpacityAtFull = 0.88f;

	/** Mist inscattering color (lit fog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog")
	FLinearColor FogMistColor = FLinearColor(0.72f, 0.78f, 0.82f);

protected:
	void EnsureTerrainMap();
	void EnsureHeightFog();
	void UpdateTerrainFog(float DeltaSeconds);
	float SampleViewFogAmount() const;
	void UpdateStreaming();
	FIntPoint WorldToChunkCoord(const FVector& WorldLocation) const;
	AActor* ResolveFocusActor() const;
	UMaterialInterface* ResolveMaterial() const;
	UMaterialInterface* FindFabGrassMaterial() const;
	UMaterialInterface* CreateFlatColGrassMaterial() const;
	float SampleHeightAtWorld(const FVector& WorldLocation) const;
	void TrySnapFocusToTerrain(AActor* Focus);
	void DisableLandscapeActorsOnce();

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<ASolidTerrainChunk>> LoadedChunks;

	/** Cached lit material so chunks do not each create their own. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ResolvedTerrainMaterial;

	/** World simulation grid — source of rendered heights + biome/threat/fog. */
	UPROPERTY(Transient)
	TObjectPtr<USolidTerrainMap> TerrainMap;

	/** Height fog actor driven by TerrainPoint fog. */
	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> HeightFogActor;

	float TimeSinceUpdate = 0.f;
	float RenderedFogAmount = 0.f;
	bool bDidDisableLandscape = false;
};
