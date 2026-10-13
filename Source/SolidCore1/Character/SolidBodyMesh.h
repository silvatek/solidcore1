#pragma once

#include "CoreMinimal.h"
#include "SolidBodyDeform.h"

class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * Builds a low-poly humanoid on the Viking skeleton.
 * The Viking sculpt is not copied. Bone names stay, so Anim_Viking_* still plays.
 * One transient skeleton copy serves every body, so the Viking skeleton asset is not modified.
 * A seed draws every channel across SolidBody::ParamMin .. ParamMax.
 * The same seed rebuilds the same body.
 * The captain stays on SK_Viking for now. Companions wear the generated body.
 */
namespace SolidBodyMesh
{
	inline constexpr uint32 CaptainSeed = 0x0C0A77A1u;

	/** Stable id for a companion name. Case does not matter. */
	uint32 SeedForName(const FString& Name);

	/** Bind-pose role from a bone name. IK helpers stay Root. */
	ESolidBodyBoneRole RoleForBoneName(const FString& BoneName);

	/**
	 * Transient low-poly mesh owned by Outer.
	 * Source supplies the Viking skeleton and bind pose. Its mesh and skeleton stay unchanged.
	 * Null when Source has no skeleton, or the body has no wearable LOD.
	 */
	USkeletalMesh* Build(const USkeletalMesh* Source, const FSolidBodyParams& Params, UObject* Outer);

	/**
	 * Replace the component mesh with a seeded low-poly body.
	 * The same seed reuses one mesh. Leaves the current mesh when the body cannot be built.
	 * The captain does not call this.
	 */
	void ApplySeed(USkeletalMeshComponent* MeshComp, uint32 Seed, UObject* Outer, const TCHAR* DebugLabel);
}
