#pragma once

#include "CoreMinimal.h"

/**
 * Full draw for a generated body, around the Viking bind pose.
 * 1 leaves a channel unchanged. A seed uses the whole band.
 * The deform clamps anything outside it.
 */
namespace SolidBody
{
	inline constexpr float ParamMin = 0.40f;
	inline constexpr float ParamMax = 1.95f;

	/**
	 * Average adult, measured down from the crown.
	 * The drawn head at HeadSize 1 is one head length.
	 * Chin 1, base of the neck and the shoulders 2, nipple line 3, navel 4,
	 * pubic bone 5, mid-thigh 6 (halfway from the hip to the knee),
	 * just below the knee 7, soles at 7.75 (the middle of 7.5 and 8).
	 */
	inline constexpr float HeadLengthCm = 22.f;
	inline constexpr float ProportionChin = 1.f;
	inline constexpr float ProportionNeck = 2.f;
	inline constexpr float ProportionChest = 3.f;
	inline constexpr float ProportionWaist = 4.f;
	inline constexpr float ProportionPubic = 5.f;
	/** Halfway from the hip to the knee. No joint sits here. */
	inline constexpr float ProportionMidThigh = 6.f;
	inline constexpr float ProportionKnee = 7.f;
	inline constexpr float ProportionSole = 7.75f;
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
 * The three sliders in the character editor. Each runs from 0 to 1.
 * Build 0 is short and wide, build 1 is tall and thin, and 0.5 is the Viking scale.
 * Gender 0 is feminine and gender 1 is masculine. 0.5 changes nothing.
 * Height 0.5 changes nothing. 0 shortens the figure and 1 lengthens it.
 */
struct FSolidBodyControls
{
	float Height = 0.5f;
	float Build = 0.5f;
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
	inline constexpr float Feminine = 0.f;
	inline constexpr float Masculine = 1.f;
	/** -1: a higher Build slider is the thin end, so bulk falls. */
	inline constexpr float BuildBulkDirection = -1.f;

	/**
	 * Build pulls each channel off 1.
	 * Positive means build 1 is larger. Negative means build 0 is larger.
	 * At 0.5 every channel is exactly 1, before gender and the height slider.
	 */
	inline constexpr float BuildHeightDelta = 0.22f;
	inline constexpr float BuildBulkDelta = -0.42f;
	inline constexpr float BuildTorsoDelta = -0.34f;
	inline constexpr float BuildShoulderDelta = -0.28f;
	inline constexpr float BuildHipDelta = -0.18f;
	inline constexpr float BuildArmDelta = 0.14f;
	inline constexpr float BuildLegDelta = 0.26f;
	inline constexpr float BuildHeadDelta = -0.24f;
	inline constexpr float BuildNeckDelta = 0.22f;

	/** Gender bias is -1 at feminine and +1 at masculine. Each spread is that fraction. */
	inline constexpr float GenderHeightSpread = 0.07f;
	inline constexpr float GenderShoulderSpread = 0.14f;
	inline constexpr float GenderHipSpread = 0.14f;
	inline constexpr float GenderChestSpread = 0.10f;
	inline constexpr float GenderBulkSpread = 0.06f;
	inline constexpr float GenderNeckSpread = 0.08f;
	inline constexpr float GenderHeadSpread = 0.04f;

	/** Height slider. 0.5 leaves the lengths alone. */
	inline constexpr float HeightScaleLow = 0.68f;
	inline constexpr float HeightScaleHigh = 1.36f;

	/** Neutral sliders. The seed does not choose the body. */
	FSolidBodyControls FromSeed(const FSolidBodyParams& Seed);

	/**
	 * Every proportion from the three sliders, in order: build, then gender, then height.
	 * The seed is ignored. 0.5, 0.5, 0.5 is the Viking scale on every channel.
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
