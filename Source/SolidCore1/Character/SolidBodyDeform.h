#pragma once

#include "CoreMinimal.h"
#include "SolidBodyParams.h"

/**
 * One bind-pose bone.
 * A body part scales the translation of each bone it wraps.
 * Rotation is copied through. The Viking skeleton asset is not modified.
 */
struct FSolidDeformBone
{
	int32 ParentIndex = INDEX_NONE;
	FVector LocalTranslation = FVector::ZeroVector;
	FQuat LocalRotation = FQuat::Identity;

	/** Skeleton bone name. Empty on a hand-built test pose. */
	FName Name;
};

/** Bind-pose bones for one generated body. */
struct FSolidDeformMesh
{
	TArray<FSolidDeformBone> Bones;
};

namespace SolidBodyDeform
{
	/** Mesh-space joint positions from the adjusted local pose. Parents are applied first. */
	void ComponentPositions(const TArray<FSolidDeformBone>& AdjustedBones, TArray<FVector>& OutJointPositions);
}
