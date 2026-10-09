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

	/** Grass surface roughness (1 = fully matte). Applied via MID on Fab / override materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GrassRoughness = 0.97f;

	/** Grass specular amount (0 = no shiny highlights). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GrassSpecular = 0.05f;

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

	/**
	 * Opaque mesh "fog banks" (debug/legacy). Off by default — they always read as solid
	 * walls. Exploration fog is Exponential Height Fog driven by TerrainPoint.Fog.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog")
	bool bRenderExplorationFogMeshes = false;

	/** Low-res fog-of-war cells per chunk edge (only if bRenderExplorationFogMeshes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "2", ClampMax = "32"))
	int32 FogQuadsPerSide = 6;

	/** Vertical extent of full exploration-fog volumes (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "200.0"))
	float FogVolumeHeightCm = 2500.f;

	/** Vertical extent of half-fog pillars (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "200.0"))
	float FogVolumeHeightHalfCm = 1400.f;

	/** Min focus travel (cm) before re-applying trail fog / queuing mesh refreshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "50.0"))
	float FogUpdateMoveThresholdCm = 250.f;

	/** Max fog-overlay chunk rebuilds per streamer tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxFogChunkRebuildsPerUpdate = 2;

	/** Soft Exponential Height Fog driven by TerrainPoint fog (true mist). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog")
	bool bRenderTerrainFog = true;

	/** How quickly rendered fog follows TerrainPoint samples. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.1"))
	float FogInterpSpeed = 2.0f;

	/** FogDensity at TerrainPoint.Fog == 0.5 (hard to see through, still misty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.0"))
	float FogDensityAtHalf = 0.12f;

	/** FogDensity at TerrainPoint.Fog == 1 (near-impenetrable mist). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.0"))
	float FogDensityAtFull = 0.35f;

	/** FogMaxOpacity at TerrainPoint.Fog == 0.5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogMaxOpacityAtHalf = 0.75f;

	/** FogMaxOpacity at TerrainPoint.Fog == 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogMaxOpacityAtFull = 0.95f;

	/** Mist inscattering color (lit fog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog")
	FLinearColor FogMistColor = FLinearColor(0.72f, 0.78f, 0.82f);

protected:
	void EnsureTerrainMap();
	void EnsureHeightFog();
	void EnsureExplorationFogMaterials();
	int32 ClearExplorationFogAtFocus();
	void QueueExplorationFogMeshRefresh(float WorldX, float WorldY, float RadiusCm);
	void ProcessExplorationFogMeshRebuilds();
	void BuildChunkActor(ASolidTerrainChunk* Chunk, FIntPoint Coord);
	void UpdateTerrainFog(float DeltaSeconds);
	float SampleViewFogAmount() const;
	void UpdateStreaming();
	FIntPoint WorldToChunkCoord(const FVector& WorldLocation) const;
	AActor* ResolveFocusActor() const;
	UMaterialInterface* ResolveMaterial() const;
	UMaterialInterface* FindFabGrassMaterial() const;
	UMaterialInterface* CreateFlatColGrassMaterial() const;
	UMaterialInterface* MakeMatteGrassInstance(UMaterialInterface* Parent) const;
	UMaterialInterface* CreateSolidColorMaterial(const FLinearColor& Color, const TCHAR* DebugName) const;
	UMaterialInterface* CreateFogVolumeMaterial(
		const FLinearColor& Color,
		float Opacity,
		const TCHAR* DebugName) const;
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

	/** Height fog actor driven by TerrainPoint fog (legacy; unused when bRenderTerrainFog is false). */
	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> HeightFogActor;

	/** Spatial exploration fog overlay materials (half / full). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ExplorationFogHalfMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ExplorationFogFullMaterial;

	float TimeSinceUpdate = 0.f;
	float RenderedFogAmount = 0.f;
	bool bDidDisableLandscape = false;
	bool bHasFogApplyLocation = false;
	FVector LastFogApplyLocation = FVector::ZeroVector;
	TSet<FIntPoint> DirtyFogChunkCoords;
};
