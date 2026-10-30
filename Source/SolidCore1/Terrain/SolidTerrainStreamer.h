#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTerrainTypes.h"
#include "SolidTerrainStreamer.generated.h"

class AExponentialHeightFog;
class ASolidTerrainChunk;
class UMaterialInterface;
class USolidTerrainMap;
class USolidWorldChart;

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

	/** Return an existing streamer, or nullptr (never spawns). Safe for HUD / read-only queries. */
	static ASolidTerrainStreamer* FindExisting(UWorld* World);

	/**
	 * Spawn a streamer if the world does not already have one.
	 * Primary owners: SolidGameMode and USolidTerrainWorldSubsystem (PIE/game backup).
	 * Other callers may use this when they need a streamer for height/snap.
	 */
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

	/** Set Fog to 0 on every terrain point, keep that clear, and rebuild the loaded fog sheets. */
	void ClearAllExplorationFog();

	/** Coarsest terrain mesh the chunk builder allows. Eagle Sight streams the map at this. */
	static constexpr int32 EagleQuadsPerSide = 4;

	/** Coarsest fog mesh the chunk builder allows. Eagle Sight uses this with the terrain quads. */
	static constexpr int32 EagleFogQuadsPerSide = 2;

	/**
	 * Inclusive chunk coordinates covering WorldMin through the point just inside WorldMax.
	 * The same edges the chart view uses, so the span is the whole map and nothing past it.
	 */
	static void ChunkSpan(
		const FVector2D& WorldMin,
		const FVector2D& WorldMax,
		float ChunkSize,
		FIntPoint& OutMin,
		FIntPoint& OutMax);

	/**
	 * Chunk radius around the camera while Eagle Sight climbs.
	 * Low down it stays the nearby radius. It reaches the map radius at the top of the climb.
	 */
	static int32 EagleStreamRadius(
		float HeightAboveGroundCm,
		float ChunkSizeCm,
		int32 NearbyRadius,
		int32 MapRadius);

	/** New coarse chunks built on one streaming pass while the camera is climbing. */
	static constexpr int32 EagleChunksPerUpdate = 12;

	/**
	 * Editor ground for the world chart. Every chunk of the centred world, and no
	 * exploration fog, so the chart stays visible. Play builds from the chart.
	 */
	void BuildChartView(const USolidWorldChart* Chart);

	/** Rebuild the chart ground, and only the chunks that contain these cells. */
	void RebuildChartView(const USolidWorldChart* Chart, const TArray<FIntPoint>& Cells);

	/** Rebuild the chart ground, and only the chunks that contain these world positions. */
	void RebuildChartPoints(const USolidWorldChart* Chart, const TArray<FVector2D>& Positions);

	/** Destroy the chart chunks. */
	void ClearChartView();

	FSolidTerrainPoint GetTerrainPointAt(const FVector& WorldLocation) const;

	/**
	 * True after the one-shot start-town relocate has run (or been skipped because
	 * WorldMap has no location 0 cells). Companions should wait on this before first spawn.
	 */
	bool HasAttemptedStartTownRelocate() const { return bDidRelocateToStartTown; }

	/** Move the focus pawn once to the WorldMap location-0 centroid. */
	void TryRelocateFocusToStartTown();

	/** Apply the same XY delta (and terrain Z snap) to every companion in the world. */
	void RelocateCompanionsByDelta(const FVector& DeltaXY);

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

	/**
	 * Planar offset from the WorldMap location-0 centroid for the Captain spawn.
	 * Keeps the pawn in the town plaza, facing that centroid.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Start Town")
	FVector2D StartTownPawnOffsetXY = FVector2D(0.f, -450.f);

	/** Keep the focus pawn on the terrain surface (retries while falling / far below). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bSnapFocusToTerrain = true;

	/** Hide Open World landscape and disable its collision so the pawn walks on procedural chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bDisableLandscapeActors = true;

	/** Extra cm above the hit surface when snapping (helps with complex-collision sink). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0"))
	float SnapHeightPadding = 4.f;

	/**
	 * TerrainPoint cells along each map axis (built once at BeginPlay).
	 * 513 doubles the original 257-point span at the same spacing: world side
	 * goes from 512 m to 1024 m, and the world stays square. The chart is
	 * 128 columns by 64 rows across that square, so a cell is 8 m east–west
	 * and 16 m north–south.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Map", meta = (ClampMin = "8", ClampMax = "1025"))
	int32 TerrainMapSize = 513;

	/** World cm between TerrainPoints. Match chunk vert step (ChunkWorldSize / QuadsPerSide) for crisp sampling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Map", meta = (ClampMin = "50.0"))
	float TerrainPointSpacing = 200.f;

	/**
	 * World-space FoW curtains (SolidTerrainFog boundary faces). On by default — clears
	 * with the pawn trail and does not depend on camera look/orbit.
	 * Height fog cannot do spatial fog-of-war (StartDistance is camera-relative).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog")
	bool bRenderExplorationFogMeshes = true;

	/** Fog curtain cells per chunk edge. Matches the 2 m terrain points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "2", ClampMax = "32"))
	int32 FogQuadsPerSide = 32;

	/**
	 * Vertical extent of the outer (full) fog curtain (cm).
	 * Same height as the inner curtain, and above every sight. Matches SolidTerrainFog::OuterCurtainHeightCm.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "200.0"))
	float FogVolumeHeightCm = 600000.f;

	/**
	 * Inner clear|fogged curtain (cm). Same height as the outer curtain.
	 * Matches SolidTerrainFog::InnerCurtainHeightCm.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "200.0"))
	float FogVolumeHeightHalfCm = 600000.f;

	/** Min focus travel (cm) before re-applying trail fog / queuing mesh refreshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog", meta = (ClampMin = "50.0"))
	float FogUpdateMoveThresholdCm = 250.f;

	/**
	 * Global Exponential Height Fog. Off by default — it is camera-relative and cannot
	 * represent trail-based exploration fog. Prefer bRenderExplorationFogMeshes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Fog")
	bool bRenderTerrainFog = false;

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
	/** Mark the chunk that holds this point, and a neighbor if the point sits on the join. */
	void MarkFogChunksAt(float WorldX, float WorldY);
	/** Rebuild every dirty curtain in this tick so neighboring sheets stay closed. */
	void ProcessExplorationFogMeshRebuilds();
	/** Quads and FogQuads below 1 use QuadsPerSide and FogQuadsPerSide. */
	void BuildChunkActor(ASolidTerrainChunk* Chunk, FIntPoint Coord, int32 Quads = -1, int32 FogQuads = -1);
	/** Rebuild the chart ground. Cells and positions name the chunks to build again. */
	void RebuildChartGround(const USolidWorldChart* Chart, const TArray<FIntPoint>& Cells, const TArray<FVector2D>& Positions);
	void UpdateTerrainFog(float DeltaSeconds);
	float SampleViewFogAmount() const;
	void UpdateStreaming();
	FIntPoint WorldToChunkCoord(const FVector& WorldLocation) const;
	AActor* ResolveFocusActor() const;
	/** Resolve via SolidTerrainMaterials (override → Fab grass → FlatCol); caches result. */
	UMaterialInterface* ResolveMaterial() const;
	/** Grassland → ResolveMaterial(); other biomes → FlatCol tinted by WorldMap key color. */
	UMaterialInterface* ResolveBiomeMaterial(ESolidBiome Biome) const;
	void CollectBiomeMaterials(TMap<ESolidBiome, UMaterialInterface*>& OutMaterials) const;
	float SampleHeightAtWorld(const FVector& WorldLocation) const;
	void TrySnapFocusToTerrain(AActor* Focus);
	/** After the player is at their start position, center fog on that pawn. */
	void CenterExplorationFogOnFocus();
	void DisableLandscapeActorsOnce();

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<ASolidTerrainChunk>> LoadedChunks;

	/** True while the World chart mode owns this streamer. Tick and streaming leave it alone. */
	bool bChartView = false;

#if WITH_EDITOR
	/** Landscapes hidden for the chart view. Restored when the mode exits. Not saved. */
	TArray<TWeakObjectPtr<AActor>> ChartHiddenLandscapes;
	void HideChartLandscapes();
	void RestoreChartLandscapes();
#endif

	/** Cached lit grassland material so chunks do not each create their own. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ResolvedTerrainMaterial;

	/** Cached solid-color materials for non-grassland biomes. */
	UPROPERTY(Transient)
	TMap<ESolidBiome, TObjectPtr<UMaterialInterface>> ResolvedBiomeMaterials;

	/** World simulation grid — source of rendered heights + biome/threat/fog. */
	UPROPERTY(Transient)
	TObjectPtr<USolidTerrainMap> TerrainMap;

	/** Height fog actor driven by TerrainPoint fog (legacy; unused when bRenderTerrainFog is false). */
	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> HeightFogActor;

	/** Spatial exploration fog overlay materials (half / full / outer white). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ExplorationFogHalfMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ExplorationFogFullMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ExplorationFogOuterMaterial;

	float TimeSinceUpdate = 0.f;
	float RenderedFogAmount = 0.f;
	bool bDidDisableLandscape = false;
	bool bHeightFogSilenced = false;
	bool bHasFogApplyLocation = false;
	bool bDidRelocateToStartTown = false;
	bool bDidCenterFogOnPlayer = false;
	/** True while Eagle Sight is on. New chunks then use the coarse mesh. */
	bool bEagleMapStream = false;
	/** Chunks still to build or drop. Streaming runs every tick until this clears. */
	bool bStreamWorkLeft = false;
	FVector LastFogApplyLocation = FVector::ZeroVector;
	TSet<FIntPoint> DirtyFogChunkCoords;
};
