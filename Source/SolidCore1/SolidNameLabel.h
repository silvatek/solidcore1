#pragma once

#include "CoreMinimal.h"

class UTextRenderComponent;

/**
 * Floating nameplates above Party members (TextRender, no UMG).
 * Captain style is larger / warmer so it reads as the highlighted label.
 */
namespace SolidNameLabel
{
	enum class EStyle : uint8
	{
		Captain,
		Companion,
	};

	inline constexpr float CaptainWorldSize = 42.f;
	inline constexpr float CompanionWorldSize = 32.f;

	/** Cm above capsule half-height (attached to root). */
	inline constexpr float HeightAboveCapsuleCm = 28.f;

	inline FColor ColorFor(EStyle Style)
	{
		return Style == EStyle::Captain
			? FColor(255, 214, 120)   // warm amber highlight
			: FColor(168, 176, 184);  // muted slate
	}

	inline float WorldSizeFor(EStyle Style)
	{
		return Style == EStyle::Captain ? CaptainWorldSize : CompanionWorldSize;
	}

	/** Apply text, color, size, alignment, and height for a style. */
	void Configure(
		UTextRenderComponent* Label,
		const FString& DisplayName,
		EStyle Style,
		float CapsuleHalfHeight);

	/** Keep the label upright and facing the local view camera. */
	void FaceViewCamera(UTextRenderComponent* Label, const UWorld* World);
}
