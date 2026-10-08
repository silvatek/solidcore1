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
	float FrequencyScale = 0.00018f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float Amplitude = 2800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float BaseHeight = 0.f;

	/** Optional material; if null a basic engine material is used when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

	/** Seconds between streamer updates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.05"))
	float UpdateIntervalSeconds = 0.25f;

	/** If set, stream around this actor; otherwise uses the first player pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TWeakObjectPtr<AActor> FocusActor;

	/** On first successful stream, move the focus pawn onto the generated height so you aren't stuck in the Open World landscape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bSnapFocusToTerrainOnce = true;

	/** Hide Open World landscape and disable its collision so the pawn walks on procedural chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bDisableLandscapeActors = true;

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

	float TimeSinceUpdate = 0.f;
	bool bDidSnapFocus = false;
	bool bDidDisableLandscape = false;
};
