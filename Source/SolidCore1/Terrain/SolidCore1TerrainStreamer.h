#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidCore1TerrainStreamer.generated.h"

class ASolidCore1TerrainChunk;
class UMaterialInterface;

/**
 * Spawns / destroys runtime procedural terrain chunks around a focus actor (usually the player pawn).
 * Heights are sampled in world XY so chunk edges match.
 */
UCLASS()
class SOLIDCORE1_API ASolidCore1TerrainStreamer : public AActor
{
	GENERATED_BODY()

public:
	ASolidCore1TerrainStreamer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Spawn a streamer if the world does not already have one. Safe to call often. */
	static ASolidCore1TerrainStreamer* EnsureExists(UWorld* World);

	/** Read-only helpers for the debug HUD (no streaming side effects). */
	int32 GetLoadedChunkCount() const { return LoadedChunks.Num(); }
	float GetHeightAt(const FVector& WorldLocation) const { return SampleHeightAtWorld(WorldLocation); }
	FIntPoint GetChunkCoordAt(const FVector& WorldLocation) const { return WorldToChunkCoord(WorldLocation); }
	UMaterialInterface* GetActiveMaterial() const
	{
		return ResolvedTerrainMaterial ? ResolvedTerrainMaterial.Get() : TerrainMaterial.Get();
	}

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

	/** Optional material override. Defaults to LevelPrototyping M_PrototypeGrid (grassy dual-tone). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

	/** Lighter grass shade (PrototypeGrid background / flat-color fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FLinearColor GrassColor = FLinearColor(0.22f, 0.40f, 0.13f);

	/** Darker grass shade used for grid / noise contrast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FLinearColor GrassDarkColor = FLinearColor(0.07f, 0.16f, 0.05f);

	/** PrototypeGrid cell size — smaller => denser speckles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.01"))
	float GrassGridSize = 0.08f;

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

protected:
	void UpdateStreaming();
	FIntPoint WorldToChunkCoord(const FVector& WorldLocation) const;
	AActor* ResolveFocusActor() const;
	UMaterialInterface* ResolveMaterial() const;
	float SampleHeightAtWorld(const FVector& WorldLocation) const;
	void TrySnapFocusToTerrain(AActor* Focus);
	void DisableLandscapeActorsOnce();

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<ASolidCore1TerrainChunk>> LoadedChunks;

	/** Cached lit MID so chunks do not each create their own. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ResolvedTerrainMaterial;

	float TimeSinceUpdate = 0.f;
	bool bDidDisableLandscape = false;
};
