#pragma once

#include "CoreMinimal.h"

/**
 * Hit targets for the main menu and the portal list.
 * Choice rows fill the panel width so a click lands on the row, not only the glyphs.
 */
namespace SolidMenuPointer
{
	inline constexpr float LineGap = 6.f;
	inline constexpr float BoxPad = 22.f;
	inline constexpr float FallbackLineHeight = 18.f;
	/** Title, then a blank line, before the first choice. */
	inline constexpr int32 LeadingLines = 2;
	/** A blank line, then the key hint, after the last choice. */
	inline constexpr int32 TrailingLines = 2;

	struct FRow
	{
		float X = 0.f;
		float Y = 0.f;
		float W = 0.f;
		float H = 0.f;
	};

	struct FMetrics
	{
		float CanvasW = 0.f;
		float CanvasH = 0.f;
		/** Widest measured line, before the panel padding. */
		float ContentW = 0.f;
		float LineHeight = FallbackLineHeight;
		float LineGap = SolidMenuPointer::LineGap;
		float BoxPad = SolidMenuPointer::BoxPad;
		int32 ChoiceCount = 0;
	};

	inline int32 LineCount(const int32 ChoiceCount)
	{
		return LeadingLines + FMath::Max(ChoiceCount, 0) + TrailingLines;
	}

	inline void PanelBox(const FMetrics& Metrics, float& OutX, float& OutY, float& OutW, float& OutH)
	{
		const int32 Lines = LineCount(Metrics.ChoiceCount);
		const float BlockHeight = Lines * Metrics.LineHeight + (Lines - 1) * Metrics.LineGap;
		OutW = Metrics.ContentW + Metrics.BoxPad * 2.f;
		OutH = BlockHeight + Metrics.BoxPad * 2.f;
		OutX = (Metrics.CanvasW - OutW) * 0.5f;
		OutY = (Metrics.CanvasH - OutH) * 0.5f;
	}

	inline float LineY(const float BoxY, const FMetrics& Metrics, const int32 LineIndex)
	{
		return BoxY + Metrics.BoxPad + LineIndex * (Metrics.LineHeight + Metrics.LineGap);
	}

	inline void ChoiceRows(const FMetrics& Metrics, TArray<FRow>& OutRows)
	{
		OutRows.Reset();
		float BoxX = 0.f;
		float BoxY = 0.f;
		float BoxW = 0.f;
		float BoxH = 0.f;
		PanelBox(Metrics, BoxX, BoxY, BoxW, BoxH);
		const float Pitch = Metrics.LineHeight + Metrics.LineGap;
		for (int32 Index = 0; Index < Metrics.ChoiceCount; ++Index)
		{
			FRow Row;
			Row.X = BoxX;
			Row.Y = LineY(BoxY, Metrics, LeadingLines + Index);
			Row.W = BoxW;
			Row.H = Pitch;
			OutRows.Add(Row);
		}
	}

	inline bool Contains(const FRow& Row, const float X, const float Y)
	{
		return X >= Row.X && X < Row.X + Row.W && Y >= Row.Y && Y < Row.Y + Row.H;
	}

	/** Choice index under the pointer, or INDEX_NONE. */
	inline int32 HitChoice(const TArray<FRow>& Rows, const float X, const float Y)
	{
		for (int32 Index = 0; Index < Rows.Num(); ++Index)
		{
			if (Contains(Rows[Index], X, Y))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}
