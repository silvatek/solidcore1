#pragma once

#include "CoreMinimal.h"

class USolidSquad;

/**
 * Squad builder opened from the F10 menu.
 * Company and Plans are filtered scrollboxes. The squad is a column of boxes,
 * one per companion the events allow, and F1 through F8 are fixed keys.
 * A press picks up a name. A release on another box swaps, fills an empty box,
 * or, on the Company or Plans box, leaves the source empty.
 */
namespace SolidSquadBuilder
{
	/** The Build squad entry stays disabled until the events allow this many. */
	inline constexpr int32 MinSquadSizeToOpen = 2;

	inline bool CanOpen(const int32 MaxSquadSize)
	{
		return MaxSquadSize >= MinSquadSizeToOpen;
	}

	/** Rows that fit in Company, Plans, and the squad column before a thumb appears. */
	inline constexpr int32 WindowRows = 5;

	inline constexpr float RowH = 26.f;
	inline constexpr float RowGap = 4.f;
	inline constexpr float RowPitch = RowH + RowGap;
	inline constexpr float ListW = 340.f;
	inline constexpr float ColumnW = 180.f;
	inline constexpr float ColumnGap = 20.f;
	inline constexpr float HeadingH = 18.f;
	inline constexpr float BlockGap = 4.f;
	inline constexpr float ChipH = 22.f;
	inline constexpr float SectionGap = 14.f;
	inline constexpr float ViewH = WindowRows * RowPitch - RowGap;
	inline constexpr float KeyGap = 6.f;
	inline constexpr int32 KeyColumns = 2;
	inline constexpr int32 KeyRows = 4;
	inline constexpr int32 VocationFilterCount = 5;
	inline constexpr int32 FormationFilterCount = 5;

	inline constexpr float BlockH = HeadingH + BlockGap + ChipH + BlockGap + ViewH;

	inline void ContentSize(float& OutW, float& OutH)
	{
		OutW = ListW + ColumnGap + ColumnW;
		OutH = BlockH + SectionGap + BlockH;
	}

	enum class EVocationFilter : uint8
	{
		All,
		Warrior,
		Rogue,
		Mage,
		Cleric,
	};

	enum class EFormationFilter : uint8
	{
		All,
		Line,
		Column,
		Mob,
		Parade,
	};

	inline const TCHAR* VocationFilterName(const EVocationFilter Filter)
	{
		switch (Filter)
		{
		case EVocationFilter::All: return TEXT("All");
		case EVocationFilter::Warrior: return TEXT("Warrior");
		case EVocationFilter::Rogue: return TEXT("Rogue");
		case EVocationFilter::Mage: return TEXT("Mage");
		case EVocationFilter::Cleric: return TEXT("Cleric");
		default: return TEXT("");
		}
	}

	inline const TCHAR* FormationFilterName(const EFormationFilter Filter)
	{
		switch (Filter)
		{
		case EFormationFilter::All: return TEXT("All");
		case EFormationFilter::Line: return TEXT("Line");
		case EFormationFilter::Column: return TEXT("Column");
		case EFormationFilter::Mob: return TEXT("Mob");
		case EFormationFilter::Parade: return TEXT("Parade");
		default: return TEXT("");
		}
	}

	/** Filters and scroll offsets. Closing the page replaces this with a fresh one. */
	struct FView
	{
		EVocationFilter Vocation = EVocationFilter::All;
		EFormationFilter Formation = EFormationFilter::All;
		float CompanyScroll = 0.f;
		float PlansScroll = 0.f;
		float SquadScroll = 0.f;
	};

	inline void ChooseVocation(FView& View, const EVocationFilter Filter)
	{
		View.Vocation = Filter;
		View.CompanyScroll = 0.f;
	}

	inline void ChooseFormation(FView& View, const EFormationFilter Filter)
	{
		View.Formation = Filter;
		View.PlansScroll = 0.f;
	}

	struct FRect
	{
		float X = 0.f;
		float Y = 0.f;
		float W = 0.f;
		float H = 0.f;

		bool Contains(const float Px, const float Py) const
		{
			return W > 0.f && H > 0.f && Px >= X && Py >= Y && Px < X + W && Py < Y + H;
		}
	};

	/** One draggable row. Index is a company index or a catalog index. */
	struct FEntry
	{
		int32 Index = INDEX_NONE;
		FString Label;
		FRect Rect;
		bool bVisible = false;
	};

	enum class EHit : uint8
	{
		None,
		VocationFilter,
		FormationFilter,
		CompanyBox,
		CompanyRow,
		SquadBox,
		PlansBox,
		PlanRow,
		KeyBox,
	};

	struct FHit
	{
		EHit Kind = EHit::None;
		/** Filter, company index, squad slot, plan index, or key. Depends on Kind. */
		int32 Index = INDEX_NONE;
	};

	enum class EDrag : uint8
	{
		None,
		Companion,
		Plan,
	};

	struct FDrag
	{
		EDrag Kind = EDrag::None;
		/** Company index or catalog index. */
		int32 Index = INDEX_NONE;
		/** Squad slot or key. None when the name came from a list. */
		int32 Slot = INDEX_NONE;
		FString Label;
	};

	struct FPage
	{
		FString Title;
		int32 FilledSquad = 0;
		int32 MaxSquad = 0;
		EVocationFilter Vocation = EVocationFilter::All;
		EFormationFilter Formation = EFormationFilter::All;

		FRect CompanyHeading;
		FRect SquadHeading;
		FRect PlansHeading;
		TArray<FRect> VocationChips;
		TArray<FRect> FormationChips;
		FRect CompanyBox;
		FRect SquadFrame;
		FRect PlansBox;
		TArray<FEntry> CompanyRows;
		TArray<FEntry> SquadSlots;
		TArray<FEntry> PlanRows;
		TArray<FEntry> Keys;

		float CompanyContentH = 0.f;
		float PlansContentH = 0.f;
		float SquadContentH = 0.f;
		float CompanyScroll = 0.f;
		float PlansScroll = 0.f;
		float SquadScroll = 0.f;
		bool bCompanyThumb = false;
		bool bPlansThumb = false;
		bool bSquadThumb = false;
		FRect CompanyThumb;
		FRect PlansThumb;
		FRect SquadThumb;
	};

	void Layout(const USolidSquad* Squad, int32 MaxSquadSize, const FView& View, FPage& OutPage);

	FHit HitTest(const FPage& Page, float X, float Y);

	/**
	 * Wheel over Company, the squad column, or Plans moves that box.
	 * A key, a chip, and empty board space leave every offset where it was.
	 * Positive wheel moves toward the top of the list.
	 */
	bool Scroll(const FPage& Page, float X, float Y, float Wheel, FView& InOutView);

	/** A filled row or box. An empty box, a chip, and a miss do not pick anything up. */
	bool BeginDrag(const FPage& Page, const FHit& Hit, FDrag& OutDrag);

	/**
	 * Move or swap. The same box, a miss, or the wrong kind of box leaves the squad as it was.
	 * Dropping the active plan onto another key moves the mark with it.
	 * Dropping it back on Plans leaves no plan active and does not change the formation.
	 * A drop does not select the plan that landed.
	 */
	bool Drop(USolidSquad* Squad, int32 MaxSquadSize, const FDrag& Drag, const FHit& Hit);
}
