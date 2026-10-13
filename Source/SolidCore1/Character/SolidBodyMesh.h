#pragma once

#include "CoreMinimal.h"
#include "SolidBodyDeform.h"

class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * Builds a unique body from SK_Viking.
 * A seed draws every channel across SolidBody::ParamMin .. ParamMax.
 * The same seed rebuilds the same body. The captain's seed does not follow his title.
 * The mesh keeps the Viking bone names and plays Anim_Viking_*.
 * Its skeleton is a transient copy, so the Viking skeleton asset is not modified.
 */
namespace SolidBodyMesh
{
	inline constexpr uint32 CaptainSeed = 0x0C0A77A1u;

	/** Stable id for a companion name. Case does not matter. */
	uint32 SeedForName(const FString& Name);

	/** Bind-pose role from a bone name. IK helpers stay Root. */
	ESolidBodyBoneRole RoleForBoneName(const FString& BoneName);

	/**
	 * Transient mesh owned by Outer. Null when Source has no editable LOD.
	 * Source is left unchanged.
	 */
	USkeletalMesh* Build(const USkeletalMesh* Source, const FSolidBodyParams& Params, UObject* Outer);

	/**
	 * Replace the component mesh with a seeded body from SK_Viking.
	 * Leaves the current mesh in place when the Viking LOD cannot be edited.
	 */
	void ApplySeed(USkeletalMeshComponent* MeshComp, uint32 Seed, UObject* Outer, const TCHAR* DebugLabel);
}
