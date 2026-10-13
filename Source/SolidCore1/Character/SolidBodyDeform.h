#pragma once

#include "CoreMinimal.h"
#include "SolidBodyParams.h"

/**
 * What a bind-pose bone contributes.
 * A bone's local translation is the segment that ends at that bone
 * (the shin bone's translation is the thigh length).
 */
enum class ESolidBodyBoneRole : uint8
{
	Root,
	Pelvis,
	Spine,
	Neck,
	Head,
	Clavicle,
	Arm,
	Hand,
	Leg,
	Foot,
};

/** One bind-pose bone. Length edits translation. Rotation is copied through. */
struct FSolidDeformBone
{
	int32 ParentIndex = INDEX_NONE;
	ESolidBodyBoneRole Role = ESolidBodyBoneRole::Root;
	FVector LocalTranslation = FVector::ZeroVector;
	FQuat LocalRotation = FQuat::Identity;
};

/** One bind-pose vertex, skinned with up to four influences. Weights should sum to 1. */
struct FSolidDeformVertex
{
	static constexpr int32 MaxInfluences = 4;

	FVector Position = FVector::ZeroVector;
	int32 BoneIndex[MaxInfluences] = { INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE };
	float Weight[MaxInfluences] = { 0.f, 0.f, 0.f, 0.f };
};

/**
 * A bind pose small enough to build in a test. The body mesh fills the same
 * bones from the Viking skeleton. Z is up.
 */
struct FSolidDeformMesh
{
	TArray<FSolidDeformBone> Bones;
	TArray<FSolidDeformVertex> Vertices;
};

/**
 * Proportion deform for a generated body.
 * Length scales bone translations. Girth and head size move vertices off
 * the bone. The Viking skeleton asset is not modified; each result is its
 * own pose that can still play Anim_Viking_* once it shares that skeleton.
 */
namespace SolidBodyDeform
{
	/** Joint positions from parent-first local transforms. */
	void ComponentPositions(const TArray<FSolidDeformBone>& Bones, TArray<FVector>& OutPositions);

	/**
	 * Apply params to a bind pose.
	 * Params are clamped first. Input mesh is left unchanged.
	 */
	FSolidDeformMesh Apply(const FSolidDeformMesh& Source, const FSolidBodyParams& Params);
}
