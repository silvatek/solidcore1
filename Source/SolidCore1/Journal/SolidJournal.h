#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SolidWorldTime.h"
#include "SolidJournal.generated.h"

namespace SolidEvents
{
	struct FEvent;
}

/**
 * One journal line. Event lines point at the event catalog.
 * Stronghold lines and lair lines are the visit text, and the same trip can be written again.
 */
enum class EJournalType : uint8
{
	Stronghold,
	Event,
	Lair,
};

struct FJournalEntry
{
	EJournalType Type = EJournalType::Event;
	int32 Year = SolidWorldTime::StartYear;
	int32 Day = SolidWorldTime::StartDay;
	int32 MinuteOfSession = 0;
	/** Catalog index when Type is Event. */
	int32 EventIndex = INDEX_NONE;
	/** Visit text when Type is Stronghold or Lair. */
	FString Text;
};

/**
 * The play state's one journal for the session.
 * Holds the lines. The HUD paints the page from CollectRows.
 * OpenLevel keeps this object when the game state keeps it.
 */
UCLASS()
class SOLIDCORE1_API USolidJournal : public UObject
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* Title = TEXT("Journal");
	static constexpr const TCHAR* TimeHeading = TEXT("Time");
	static constexpr const TCHAR* SummaryHeading = TEXT("Summary");
	static constexpr const TCHAR* EmptyText = TEXT("No events yet.");
	static constexpr const TCHAR* CloseHint = TEXT("F10 to close");

	/** Gap between the time column and the entry column. */
	static constexpr float ColumnGap = 48.f;
	/** Oak inside the bronze binding. */
	static constexpr float BoardPad = 36.f;
	static constexpr float LineGap = 8.f;
	/** The page stays at least this large. */
	static constexpr float MinBoardWidth = 880.f;
	static constexpr float MinBoardHeight = 520.f;
	/** Share of the view, then raised to the minimum and kept on screen. */
	static constexpr float BoardWidthFraction = 0.55f;
	static constexpr float BoardHeightFraction = 0.62f;
	static constexpr float BoardMargin = 40.f;
	static constexpr float TitlePlatePadX = 18.f;
	static constexpr float TitlePlatePadY = 4.f;

	/** Oldest line drops off once the page is this full. */
	static constexpr int32 MaxEntries = 64;

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

	/** Lines in the order they were written. Not a UPROPERTY. */
	TArray<FJournalEntry> Entries;

	void Remember(
		const SolidWorldTime::FStamp& When,
		EJournalType Type,
		const FString& Text,
		int32 EventIndex = INDEX_NONE);

	/**
	 * Enter or exit. The line does not name the stronghold.
	 * An upgrade will be its own entry. The same trip can be noted again.
	 */
	void RememberStronghold(const SolidWorldTime::FStamp& When, bool bEnter);

	/** Enter or exit the Iglin tunnels. The same trip can be noted again. */
	void RememberLair(const SolidWorldTime::FStamp& When, bool bEnter);

	/** One line describing an event's configuration changes. Does not read Entries. */
	static FString Describe(const SolidEvents::FEvent& Event);

	/**
	 * Break one journal line into lines that fit MaxWidth.
	 * A break is at a space. A word wider than the column breaks on a character.
	 * Measure returns a width in the same units as MaxWidth.
	 */
	static void WrapText(
		const FString& Text,
		float MaxWidth,
		TFunctionRef<float(const FString&)> Measure,
		TArray<FString>& OutLines);

	/** Title, Time and Summary, the lines, detail under an event, and F10 to close. */
	void CollectRows(TArray<FRow>& OutRows) const;

	/**
	 * Board size in pixels. Content can widen it, and the view keeps it on screen.
	 * ContentW and ContentH are the measured lines, without the board padding.
	 */
	static void BoardSize(
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
	static FTitlePlate TitlePlate(
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
	static FColumns Columns(const float OriginX, const float StampWidth)
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
};
