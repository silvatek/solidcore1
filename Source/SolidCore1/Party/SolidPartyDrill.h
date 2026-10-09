#pragma once

#include "CoreMinimal.h"

/**
 * F9 Party formation drill: walk a square while cycling assigned battle plans F1–F4.
 * Leg 0 (F1): walk only. Legs 1–3 (F2–F4): turn 90° clockwise, then walk.
 */
namespace SolidPartyDrill
{
	inline constexpr float LegDurationSeconds = 1.5f;
	inline constexpr int32 NumLegs = 4;
	inline constexpr float TurnYawDegrees = 90.f;

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
		float WalkSecondsRemaining = 0.f;
	};
}
