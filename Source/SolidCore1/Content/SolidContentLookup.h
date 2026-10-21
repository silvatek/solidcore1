#pragma once

#include "CoreMinimal.h"

class UAnimSequence;
class UMaterialInterface;
class USkeletalMesh;

/**
 * Resolve Fab-sourced Content without a manual folder move after Add to Project.
 * Viking and grass are found by asset name under /Game/Viking or /Game/Fab.
 * Recorded paths in tools/fab-assets.json are tried first.
 */
namespace SolidContentLookup
{
	inline constexpr const TCHAR* VikingMeshName = TEXT("SK_Viking");
	inline constexpr const TCHAR* VikingIdleName = TEXT("Anim_Viking_idle1");
	inline constexpr const TCHAR* VikingWalkName = TEXT("Anim_Viking_walk");
	inline constexpr const TCHAR* VikingRunName = TEXT("Anim_Viking_run");
	inline constexpr const TCHAR* VikingJumpName = TEXT("Anim_Viking_jump");

	/**
	 * Prefer /Game/Viking (repo layout), then /Game/Fab (Launcher Add to Project).
	 * Returns nullptr if neither copy is present.
	 */
	USkeletalMesh* FindVikingMesh();
	UAnimSequence* FindVikingClip(const TCHAR* AssetName);

	UAnimSequence* FindVikingIdle();
	UAnimSequence* FindVikingWalk();
	UAnimSequence* FindVikingRun();
	UAnimSequence* FindVikingJump();

	/**
	 * The green Velociraptor body.
	 * Recorded paths, then the recorded folder, then a Velociraptor skeletal mesh under Content.
	 * A name that starts with SK_ wins, and otherwise the shortest name wins.
	 * Skeleton, physics, and morph assets are skipped. A find is written back.
	 */
	USkeletalMesh* FindVelociraptorMesh();

	/**
	 * The in-place idle in the mesh's folder.
	 * The name contains Idle and none of Root, Attack, Death, Hit, Roar, Walk, and Run.
	 * The shortest such name wins. A find is written back.
	 */
	UAnimSequence* FindVelociraptorIdle(const USkeletalMesh* Mesh);

	/** The material whose name contains Green, in the Velociraptor pack. The shortest name wins. */
	UMaterialInterface* FindVelociraptorGreen(const USkeletalMesh* Mesh);
}
