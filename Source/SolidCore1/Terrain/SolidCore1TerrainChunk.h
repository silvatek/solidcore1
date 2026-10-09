#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidCore1TerrainChunk.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class USolidCore1TerrainMap;

UCLASS()
class SOLIDCORE1_API ASolidCore1TerrainChunk : public AActor
{
	GENERATED_BODY()

public:
	ASolidCore1TerrainChunk();

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
		const USolidCore1TerrainMap* TerrainMap);

	FIntPoint GetChunkCoord() const { return ChunkCoord; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Transient runtime mesh owned by this chunk; replaced each BuildChunk. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> RuntimeStaticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	FIntPoint ChunkCoord = FIntPoint::ZeroValue;
};
