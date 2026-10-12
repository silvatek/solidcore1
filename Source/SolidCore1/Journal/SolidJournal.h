#pragma once

#include "CoreMinimal.h"
#include "Events/SolidEvents.h"

/**
 * Journal page opened from the F10 menu.
 * The name sits in the top border. Time and Summary head the two columns.
 * A longer page will scroll; the board stays this size either way.
 */
namespace SolidJournal
{
	inline constexpr const TCHAR* Title = TEXT("Journal");
	inline constexpr const TCHAR* TimeHeading = TEXT("Time");
	inline constexpr const TCHAR* SummaryHeading = TEXT("Summary");
	inline constexpr const TCHAR* EmptyText = TEXT("No events yet.");
	inline constexpr const TCHAR* CloseHint = TEXT("F10 to close");

	/** Gap between the time column and the entry column. */
	inline constexpr float ColumnGap = 48.f;
	/** Oak inside the bronze binding. */
	inline constexpr float BoardPad = 36.f;
	inline constexpr float LineGap = 8.f;
	/** The page stays at least this large. */
	inline constexpr float MinBoardWidth = 880.f;
	inline constexpr float MinBoardHeight = 520.f;
	/** Share of the view, then raised to the minimum and kept on screen. */
	inline constexpr float BoardWidthFraction = 0.55f;
	inline constexpr float BoardHeightFraction = 0.62f;
	inline constexpr float BoardMargin = 40.f;

	enum class ERowKind : uint8
	{
		/** Drawn on the top border, not in the column list. */
		Title,
		/** Column names. Stamp is Time, text is Summary. */
		Heading,
		Entry,
		/** Change under an entry. Stamp is empty so the words share the entry column. */
		Detail,
		Empty,
		Hint,
	};

	struct FRow
	{
		ERowKind Kind = ERowKind::Entry;
		/** World time. Set on an entry. Empty on a detail line. */
		FString Stamp;
		/** Journal text. */
		FString Text;
	};

	struct FColumns
	{
		float StampX = 0.f;
		float TextX = 0.f;
	};

	/** Nameplate on the top bronze band. */
	struct FTitlePlate
	{
		float X = 0.f;
		float Y = 0.f;
		float W = 0.f;
		float H = 0.f;
	};

	inline constexpr float TitlePlatePadX = 18.f;
	inline constexpr float TitlePlatePadY = 4.f;

	/** One line describing an event's configuration changes. */
	FString Describe(const SolidEvents::FEvent& Event);

	void CollectRows(const SolidEvents::FState& State, TArray<FRow>& OutRows);

	/**
	 * Board size in pixels. Content can widen it, and the view keeps it on screen.
	 * ContentW and ContentH are the measured lines, without the board padding.
	 */
	inline void BoardSize(
		const float ViewW,
		const float ViewH,
		const float ContentW,
		const float ContentH,
		float& OutW,
		float& OutH)
	{
		const float RoomW = FMath::Max(0.f, ViewW - BoardMargin * 2.f);
		const float RoomH = FMath::Max(0.f, ViewH - BoardMargin * 2.f);
		const float WantW = FMath::Max3(MinBoardWidth, ViewW * BoardWidthFraction, ContentW + BoardPad * 2.f);
		const float WantH = FMath::Max3(MinBoardHeight, ViewH * BoardHeightFraction, ContentH + BoardPad * 2.f);
		OutW = FMath::Min(WantW, RoomW);
		OutH = FMath::Min(WantH, RoomH);
	}

	/** The name sits on the top bronze band, centered along that edge. */
	inline FTitlePlate TitlePlate(
		const float BoardX,
		const float BoardY,
		const float BoardW,
		const float TitleW,
		const float TitleH,
		const float Band)
	{
		FTitlePlate Plate;
		Plate.W = TitleW + TitlePlatePadX * 2.f;
		Plate.H = FMath::Max(TitleH + TitlePlatePadY * 2.f, Band);
		Plate.X = BoardX + (BoardW - Plate.W) * 0.5f;
		Plate.Y = BoardY + Band * 0.5f - Plate.H * 0.5f;
		return Plate;
	}

	/** Time on the left. Entry text starts to the right of the widest stamp. */
	inline FColumns Columns(const float OriginX, const float StampWidth)
	{
		FColumns Out;
		Out.StampX = OriginX;
		Out.TextX = OriginX;
		if (StampWidth > 0.f)
		{
			Out.TextX = OriginX + StampWidth + ColumnGap;
		}
		return Out;
	}
}
