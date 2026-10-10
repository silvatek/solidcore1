#pragma once

#include "CoreMinimal.h"

/**
 * Party formation drill (main menu: Test Drill): walk a square while cycling assigned battle plans F1–F4.
 * Leg 0 (F1): walk only. Legs 1–3 (F2–F4): blend +90° yaw over TurnDurationSeconds, then walk.
 */
namespace SolidPartyDrill
{
	inline constexpr float LegDurationSeconds = 1.5f;
	inline constexpr float TurnDurationSeconds = 0.25f;
	inline constexpr int32 NumLegs = 4;
	inline constexpr float TurnYawDegrees = 90.f;

	enum class EPhase : uint8
	{
		Turning,
		Walking,
	};

	/** Assigned battle-plan slot for a leg (0 = F1 … 3 = F4). */
	inline int32 PlanSlotForLeg(const int32 LegIndex)
	{
		return FMath::Clamp(LegIndex, 0, NumLegs - 1);
	}

	/** True for F2–F4 legs (turn before walking). */
	inline bool TurnsBeforeWalk(const int32 LegIndex)
	{
		return LegIndex >= 1 && LegIndex < NumLegs;
	}

	struct FState
	{
		bool bActive = false;
		int32 CurrentLeg = 0;
		EPhase Phase = EPhase::Walking;
		float PhaseSecondsRemaining = 0.f;
		float TurnStartYaw = 0.f;
		float TurnTargetYaw = 0.f;
		bool bSavedOrientRotationToMovement = true;
	};
}
