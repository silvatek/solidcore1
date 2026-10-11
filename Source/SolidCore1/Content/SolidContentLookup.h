#pragma once

#include "CoreMinimal.h"

class UAnimSequence;
class USkeletalMesh;

/**
 * Resolve Fab-sourced Content by asset name so Launcher "Add to Project"
 * can land under /Game/Fab/<listing>/ without a manual folder move.
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
}
