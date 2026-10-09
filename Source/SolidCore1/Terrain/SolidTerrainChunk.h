#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTerrainChunk.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class USolidTerrainMap;

UCLASS()
class SOLIDCORE1_API ASolidTerrainChunk : public AActor
{
	GENERATED_BODY()

public:
	ASolidTerrainChunk();

	void BuildChunk(
		FIntPoint InChunkCoord,
		float InChunkWorldSize,
		int32 InQuadsPerSide,
		int32 InSeed,
		float InFrequencyScale,
		float InAmplitude,
		float InBaseHeight,
		float InCollisionHeightBias,
		UMaterialInterface* Material,
		UMaterialInterface* FogHalfMaterial,
		UMaterialInterface* FogFullMaterial,
		UMaterialInterface* FogOuterMaterial,
		const USolidTerrainMap* TerrainMap,
		int32 InFogQuadsPerSide = 6,
		float InFogVolumeHeightCm = 2500.f,
		float InFogVolumeHeightHalfCm = 1400.f);

	/** Cheap fog-of-war refresh: rebuilds only the volume mesh (no collision terrain rebuild). */
	void RebuildExplorationFog(
		UMaterialInterface* FogHalfMaterial,
		UMaterialInterface* FogFullMaterial,
		UMaterialInterface* FogOuterMaterial,
		const USolidTerrainMap* TerrainMap,
		int32 InFogQuadsPerSide = 6,
		float InFogVolumeHeightCm = 2500.f,
		float InFogVolumeHeightHalfCm = 1400.f);

	FIntPoint GetChunkCoord() const { return ChunkCoord; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Non-colliding exploration-fog overlay (spatial fog-of-war). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<UStaticMeshComponent> FogMeshComponent;

	/** Transient runtime mesh owned by this chunk; replaced each BuildChunk. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> RuntimeStaticMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> RuntimeFogStaticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	FIntPoint ChunkCoord = FIntPoint::ZeroValue;

	float CachedChunkWorldSize = 6400.f;
	float CachedCollisionHeightBias = 0.f;
	int32 CachedFogQuadsPerSide = 6;
	float CachedFogVolumeHeightCm = 2500.f;
	float CachedFogVolumeHeightHalfCm = 1400.f;
};
