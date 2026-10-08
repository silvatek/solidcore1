#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidCore1TerrainChunk.generated.h"

class USolidCore1TerrainMeshComponent;
class UMaterialInterface;

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
		UMaterialInterface* Material);

	FIntPoint GetChunkCoord() const { return ChunkCoord; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<USolidCore1TerrainMeshComponent> ProceduralMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	FIntPoint ChunkCoord = FIntPoint::ZeroValue;
};
