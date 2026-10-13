#pragma once

#include "CoreMinimal.h"

/**
 * Proportion range around the Viking bind pose.
 * 1 leaves a channel unchanged. The deform clamps anything outside this
 * so a seed cannot pull a joint apart.
 */
namespace SolidBody
{
	inline constexpr float ParamMin = 0.85f;
	inline constexpr float ParamMax = 1.15f;
}

/**
 * One generated humanoid. The mesh is rebuilt from these multipliers;
 * nothing here stores vertices.
 * FromSeed is pure: one seed is one body, for the captain and for every NPC.
 */
struct FSolidBodyParams
{
	float Height = 1.f;
	float ShoulderWidth = 1.f;
	float HipWidth = 1.f;
	float TorsoGirth = 1.f;
	float ArmLength = 1.f;
	float LegLength = 1.f;
	float HeadSize = 1.f;
	float NeckLength = 1.f;
	float Bulk = 1.f;

	/** Sample every channel inside SolidBody::ParamMin .. ParamMax. */
	static FSolidBodyParams FromSeed(uint32 Seed);

	/** Copy with each channel clamped to the proportion range. */
	FSolidBodyParams Clamped() const;

	/** Bit-identical comparison. Same seed must pass this. */
	bool ExactEquals(const FSolidBodyParams& Other) const;
};
