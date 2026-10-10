#pragma once

#include "CoreMinimal.h"

/**
 * Oak board in a bronze binding, used by the menu pages and the battle-plan list.
 * The debug readout stays a plain block.
 */
namespace SolidHudTheme
{
	struct FFrame
	{
		/** Outer bronze band (px). */
		float Outer = 5.f;
		/** Dark recess between the two bronze lines (px). */
		float Gap = 3.f;
		/** Inner bronze hairline (px). */
		float Inner = 1.f;
		/** Brighter boss drawn on each corner (px). */
		float Corner = 12.f;
	};

	struct FPalette
	{
		FLinearColor Fill = FLinearColor::Black;
		FLinearColor Recess = FLinearColor::Black;
		FLinearColor Bronze = FLinearColor::White;
		FLinearColor BronzeBright = FLinearColor::White;
		FLinearColor Title = FLinearColor::White;
		FLinearColor Ink = FLinearColor::White;
		FLinearColor Hint = FLinearColor::White;
	};

	/** Journal, credits, and the main menu. */
	inline FFrame MenuFrame()
	{
		return { 5.f, 3.f, 1.f, 12.f };
	}

	/** Battle-plan list. Thinner so the corner panel stays compact. */
	inline FFrame HudFrame()
	{
		return { 3.f, 2.f, 1.f, 8.f };
	}

	/** Pixels from the panel edge to the oak fill. */
	inline float BandThickness(const FFrame& Frame)
	{
		return Frame.Outer + Frame.Gap + Frame.Inner;
	}

	/** Inner oak size after the binding is inset. False when the box is too small. */
	inline bool InnerSize(const float Width, const float Height, const FFrame& Frame, float& OutWidth, float& OutHeight)
	{
		const float Band = BandThickness(Frame);
		OutWidth = Width - Band * 2.f;
		OutHeight = Height - Band * 2.f;
		return OutWidth > 0.f && OutHeight > 0.f;
	}

	inline FPalette OakAndBronze()
	{
		FPalette Palette;
		Palette.Fill = FLinearColor(0.10f, 0.06f, 0.035f, 1.f);
		Palette.Recess = FLinearColor(0.02f, 0.012f, 0.01f, 1.f);
		Palette.Bronze = FLinearColor(0.62f, 0.42f, 0.18f, 1.f);
		Palette.BronzeBright = FLinearColor(0.86f, 0.66f, 0.32f, 1.f);
		Palette.Title = FLinearColor(1.f, 0.84f, 0.47f, 1.f);
		Palette.Ink = FLinearColor(0.96f, 0.93f, 0.86f, 1.f);
		Palette.Hint = FLinearColor(0.70f, 0.60f, 0.46f, 1.f);
		return Palette;
	}
}
