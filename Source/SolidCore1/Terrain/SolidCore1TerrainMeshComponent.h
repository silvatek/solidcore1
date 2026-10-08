#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "SolidCore1TerrainMeshComponent.generated.h"

/**
 * Procedural mesh that can force frustum / occlusion bounds.
 * Do not assign a local FBox to Bounds (world-space) — that culled off-origin chunks.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SOLIDCORE1_API USolidCore1TerrainMeshComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	void SetForcedLocalBounds(const FBox& InLocalBox);

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

protected:
	UPROPERTY(Transient)
	FBox ForcedLocalBounds;

	UPROPERTY(Transient)
	bool bUseForcedLocalBounds = false;
};
