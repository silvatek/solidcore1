#include "SolidCore1TerrainMeshComponent.h"

void USolidCore1TerrainMeshComponent::SetForcedLocalBounds(const FBox& InLocalBox)
{
	bUseForcedLocalBounds = InLocalBox.IsValid;
	ForcedLocalBounds = InLocalBox;
	UpdateBounds();
	MarkRenderStateDirty();
}

FBoxSphereBounds USolidCore1TerrainMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (bUseForcedLocalBounds)
	{
		return FBoxSphereBounds(ForcedLocalBounds.TransformBy(LocalToWorld));
	}
	return Super::CalcBounds(LocalToWorld);
}
