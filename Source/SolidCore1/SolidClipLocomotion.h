#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "UObject/SoftObjectPtr.h"

class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * Shared single-node clip locomotion for Captain / Companion (and later other Party members).
 * Does not assume a shared mesh — each character supplies its own skeletal mesh soft ptr.
 * Default Content paths below are Fab Viking; callers may point soft ptrs elsewhere.
 */
namespace SolidClipLocomotion
{
	inline constexpr const TCHAR* DefaultMeshPath = TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking");
	inline constexpr const TCHAR* DefaultIdlePath = TEXT("/Game/Viking/Animations/Anim_Viking_idle1.Anim_Viking_idle1");
	inline constexpr const TCHAR* DefaultWalkPath = TEXT("/Game/Viking/Animations/Anim_Viking_walk.Anim_Viking_walk");
	inline constexpr const TCHAR* DefaultRunPath = TEXT("/Game/Viking/Animations/Anim_Viking_run.Anim_Viking_run");
	inline constexpr const TCHAR* DefaultJumpPath = TEXT("/Game/Viking/Animations/Anim_Viking_jump.Anim_Viking_jump");

	/** Pure clip-selection preference (caller maps to loaded assets). */
	enum class EClip : uint8
	{
		Idle,
		Walk,
		Run,
		Jump,
	};

	/**
	 * Choose idle / walk / run / jump from planar speed and flags.
	 * @param bPreferRun  Caller-computed (sprint, catch-up, Captain/Companion speed rules).
	 * @param bAllowJump  True when a jump clip exists and air poses should use it.
	 */
	inline EClip SelectClip(
		float PlanarSpeed,
		float WalkSpeedThreshold,
		bool bPreferRun,
		bool bInAir,
		bool bAllowJump)
	{
		if (bInAir && bAllowJump)
		{
			return EClip::Jump;
		}
		if (bPreferRun)
		{
			return EClip::Run;
		}
		if (PlanarSpeed >= WalkSpeedThreshold)
		{
			return EClip::Walk;
		}
		return EClip::Idle;
	}

	/** Load soft clip, else hard path. */
	UAnimSequence* LoadClip(TSoftObjectPtr<UAnimSequence>& Soft, const TCHAR* FallbackPath);

	/** Load soft mesh, else hard path. */
	USkeletalMesh* LoadMesh(TSoftObjectPtr<USkeletalMesh>& Soft, const TCHAR* FallbackPath);

	/**
	 * Assign mesh (if needed) and switch the component to AnimationSingleNode.
	 * @param bOnlyIfMeshUnset  When true (Companion default), skip mesh assign if one is already set.
	 */
	bool ApplyMeshAndSingleNodeMode(
		USkeletalMeshComponent* MeshComp,
		TSoftObjectPtr<USkeletalMesh>& SoftMesh,
		const TCHAR* FallbackMeshPath,
		const TCHAR* DebugLabel,
		bool bOnlyIfMeshUnset = false);

	/**
	 * Play a looping clip on the mesh's AnimSingleNodeInstance.
	 * Updates ActiveAnim when playback is established.
	 */
	bool PlayLoopingClip(
		USkeletalMeshComponent* MeshComp,
		UAnimSequence* Anim,
		TObjectPtr<UAnimSequence>& ActiveAnim,
		const TCHAR* DebugLabel);

	/** True if ActiveAnim is already the Desired clip and playing. */
	bool IsPlayingClip(
		const USkeletalMeshComponent* MeshComp,
		const UAnimSequence* ActiveAnim,
		const UAnimSequence* Desired);
}
