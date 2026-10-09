#pragma once

#include "CoreMinimal.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UObject;

/**
 * Floating nameplates above Party members (TextRender + cube plates, no UMG).
 * Captain style is larger / warmer so it reads as the highlighted label.
 * Plates: outer border + contrasting background behind the text.
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

	/** Cm above capsule half-height (label root attached to capsule root). */
	inline constexpr float HeightAboveCapsuleCm = 28.f;

	inline FColor TextColorFor(EStyle Style)
	{
		return Style == EStyle::Captain
			? FColor(255, 214, 120)   // warm amber highlight
			: FColor(210, 216, 222);  // light slate on dark plate
	}

	inline FLinearColor BackgroundColorFor(EStyle Style)
	{
		return Style == EStyle::Captain
			? FLinearColor(0.07f, 0.05f, 0.03f)   // near-black brown
			: FLinearColor(0.06f, 0.08f, 0.10f);  // dark slate
	}

	inline FLinearColor BorderColorFor(EStyle Style)
	{
		return Style == EStyle::Captain
			? FLinearColor(0.85f, 0.68f, 0.32f)   // amber border
			: FLinearColor(0.42f, 0.46f, 0.52f);  // mid slate border
	}

	inline float WorldSizeFor(EStyle Style)
	{
		return Style == EStyle::Captain ? CaptainWorldSize : CompanionWorldSize;
	}

	/** Legacy alias used by tests / callers that only need the text color. */
	inline FColor ColorFor(EStyle Style)
	{
		return TextColorFor(Style);
	}

	/**
	 * Size / materials / text for a nameplate group.
	 * Root is placed above the capsule; Border + Background are thin cubes facing +X;
	 * Text sits in front. FaceViewCamera should rotate Root.
	 */
	void Configure(
		USceneComponent* Root,
		UStaticMeshComponent* Border,
		UStaticMeshComponent* Background,
		UTextRenderComponent* Text,
		const FString& DisplayName,
		EStyle Style,
		float CapsuleHalfHeight,
		UObject* MaterialOuter);

	/** Keep the label upright and facing the local view camera. */
	void FaceViewCamera(USceneComponent* LabelRoot, const UWorld* World);
}
