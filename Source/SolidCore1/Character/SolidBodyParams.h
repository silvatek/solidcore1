#pragma once

#include "CoreMinimal.h"

/**
 * Full draw for a generated body, around the Viking bind pose.
 * 1 leaves a channel unchanged. A seed uses the whole band.
 * The deform clamps anything outside it.
 */
namespace SolidBody
{
	inline constexpr float ParamMin = 0.75f;
	inline constexpr float ParamMax = 1.35f;
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

/**
 * The three sliders in the character editor.
 * Height and build sit on SolidBody::ParamMin .. ParamMax.
 * Gender runs from 0 (broad shoulders, narrow hips) to 1 (the opposite). 0.5 leaves the seed.
 */
struct FSolidBodyControls
{
	float Height = 1.f;
	float Build = 1.f;
	float Gender = 0.5f;
};

namespace SolidBodyControls
{
	inline constexpr int32 Height = 0;
	inline constexpr int32 Build = 1;
	inline constexpr int32 Gender = 2;
	inline constexpr int32 Count = 3;
	inline constexpr float KeyStep = 0.05f;
	inline constexpr float MouseStep = 0.01f;
	inline constexpr float GenderNeutral = 0.5f;
	/** Feminine end of the gender slider. Masculine is the other end. */
	inline constexpr float Feminine = 1.f;
	inline constexpr float Masculine = 0.f;
	/** +1 when a higher Build slider increases bulk. */
	inline constexpr float BuildBulkDirection = 1.f;
	/** How far gender 0 or 1 pulls shoulders and hips off the seed. */
	inline constexpr float GenderSpread = 0.18f;

	/** Height and build copied from the seed. Gender stays neutral, so Resolve matches the seed. */
	FSolidBodyControls FromSeed(const FSolidBodyParams& Seed);

	/**
	 * Seed with the sliders applied.
	 * Neutral gender and a build equal to the seed bulk leave every channel bit-identical.
	 */
	FSolidBodyParams Resolve(const FSolidBodyParams& Seed, const FSolidBodyControls& Controls);

	/** Wrap a slider index by Delta inside Count. */
	int32 WrapSlider(int32 Index, int32 Delta);

	/** Move one slider by one key step. Direction above zero increases it. */
	void Nudge(FSolidBodyControls& Controls, int32 Slider, int32 Direction);

	/** Set one slider from a 0..1 bar position, quantized to MouseStep. */
	void SetFraction(FSolidBodyControls& Controls, int32 Slider, float Fraction);

	/** 0 at the low end of that slider, 1 at the high end. */
	float Fraction(int32 Slider, float Value);

	/** Slider value for a 0..1 bar position, quantized to MouseStep and clamped. */
	float ValueAtFraction(int32 Slider, float Fraction);

	const TCHAR* Label(int32 Slider);
}
