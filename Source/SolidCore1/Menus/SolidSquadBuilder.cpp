#include "SolidSquadBuilder.h"
#include "SolidHUD.h"
#include "Company/SolidBattlePlan.h"
#include "Company/SolidCompany.h"
#include "Company/SolidSquad.h"

namespace
{
	bool MatchesVocation(const ESolidVocation Vocation, const SolidSquadBuilder::EVocationFilter Filter)
	{
		switch (Filter)
		{
		case SolidSquadBuilder::EVocationFilter::All:
			return true;
		case SolidSquadBuilder::EVocationFilter::Warrior:
			return Vocation == ESolidVocation::Warrior;
		case SolidSquadBuilder::EVocationFilter::Rogue:
			return Vocation == ESolidVocation::Rogue;
		case SolidSquadBuilder::EVocationFilter::Mage:
			return Vocation == ESolidVocation::Mage;
		case SolidSquadBuilder::EVocationFilter::Cleric:
			return Vocation == ESolidVocation::Cleric;
		default:
			return false;
		}
	}

	bool MatchesFormation(
		const ESolidBattleFormation Formation,
		const SolidSquadBuilder::EFormationFilter Filter)
	{
		switch (Filter)
		{
		case SolidSquadBuilder::EFormationFilter::All:
			return true;
		case SolidSquadBuilder::EFormationFilter::Line:
			return Formation == ESolidBattleFormation::Line;
		case SolidSquadBuilder::EFormationFilter::Column:
			return Formation == ESolidBattleFormation::Column;
		case SolidSquadBuilder::EFormationFilter::Mob:
			return Formation == ESolidBattleFormation::Mob;
		case SolidSquadBuilder::EFormationFilter::Parade:
			return Formation == ESolidBattleFormation::Parade;
		default:
			return false;
		}
	}

	float ClampScroll(const float Scroll, const float ContentH, const float ViewH)
	{
		const float MaxScroll = FMath::Max(0.f, ContentH - ViewH);
		return FMath::Clamp(Scroll, 0.f, MaxScroll);
	}

	void PlaceThumb(
		const SolidSquadBuilder::FRect& Box,
		const float ContentH,
		const float Scroll,
		bool& bShow,
		SolidSquadBuilder::FRect& Thumb)
	{
		bShow = ContentH > Box.H + 0.5f;
		Thumb = SolidSquadBuilder::FRect();
		if (!bShow)
		{
			return;
		}
		const float ThumbH = FMath::Clamp(Box.H * (Box.H / ContentH), 12.f, Box.H);
		const float MaxScroll = FMath::Max(0.f, ContentH - Box.H);
		const float Travel = FMath::Max(0.f, Box.H - ThumbH);
		const float Along = MaxScroll > 0.f ? (Scroll / MaxScroll) * Travel : 0.f;
		Thumb.X = Box.X + Box.W - 8.f;
		Thumb.Y = Box.Y + Along;
		Thumb.W = 6.f;
		Thumb.H = ThumbH;
	}

	SolidSquadBuilder::FRect ChipRect(const float Left, const float Top, const int32 Index, const int32 Count)
	{
		const float Gap = 4.f;
		const float Width = Count > 0
			? (SolidSquadBuilder::ListW - Gap * static_cast<float>(Count - 1)) / static_cast<float>(Count)
			: 0.f;
		SolidSquadBuilder::FRect Chip;
		Chip.X = Left + static_cast<float>(Index) * (Width + Gap);
		Chip.Y = Top;
		Chip.W = Width;
		Chip.H = SolidSquadBuilder::ChipH;
		return Chip;
	}

	void ClipRow(
		const SolidSquadBuilder::FRect& Box,
		const float ContentY,
		const float Scroll,
		SolidSquadBuilder::FEntry& Row)
	{
		const float Top = Box.Y + ContentY - Scroll;
		const float Bottom = Top + SolidSquadBuilder::RowH;
		const float ClipTop = FMath::Max(Top, Box.Y);
		const float ClipBottom = FMath::Min(Bottom, Box.Y + Box.H);
		Row.Rect.X = Box.X + 4.f;
		Row.Rect.Y = ClipTop;
		Row.Rect.W = FMath::Max(0.f, Box.W - 16.f);
		Row.Rect.H = ClipBottom - ClipTop;
		Row.bVisible = Row.Rect.H > 0.5f && Row.Rect.W > 0.f;
	}

	bool InFirstSlots(const TArray<int32>& Slots, const int32 MaxSlots, const int32 Index)
	{
		if (Index == INDEX_NONE)
		{
			return false;
		}
		const int32 Count = FMath::Min(MaxSlots, Slots.Num());
		for (int32 Slot = 0; Slot < Count; ++Slot)
		{
			if (Slots[Slot] == Index)
			{
				return true;
			}
		}
		return false;
	}

	void FollowActivePlan(
		USolidSquad* Squad,
		const int32 ActivePlan,
		const TArray<int32>& After,
		const ESolidBattleFormation Formation,
		const ESolidBattleSpacing Spacing)
	{
		if (!Squad || ActivePlan == INDEX_NONE)
		{
			return;
		}
		const int32 Now = After.IndexOfByKey(ActivePlan);
		if (Now == INDEX_NONE)
		{
			Squad->SetHeldFormation(Formation, Spacing);
			Squad->ClearActiveBattlePlan();
			return;
		}
		Squad->SelectAssignedSlot(Now);
	}

	struct FPendingRow
	{
		int32 Index = INDEX_NONE;
		FString Label;
		float ContentY = 0.f;
	};

	void PlaceRows(
		const TArray<FPendingRow>& Pending,
		const float ContentH,
		const float RequestedScroll,
		const SolidSquadBuilder::FRect& Box,
		float& OutScroll,
		bool& bThumb,
		SolidSquadBuilder::FRect& Thumb,
		TArray<SolidSquadBuilder::FEntry>& OutRows)
	{
		OutScroll = ClampScroll(RequestedScroll, ContentH, SolidSquadBuilder::ViewH);
		PlaceThumb(Box, ContentH, OutScroll, bThumb, Thumb);
		OutRows.Reserve(Pending.Num());
		for (const FPendingRow& Item : Pending)
		{
			SolidSquadBuilder::FEntry Row;
			Row.Index = Item.Index;
			Row.Label = Item.Label;
			ClipRow(Box, Item.ContentY, OutScroll, Row);
			OutRows.Add(Row);
		}
	}
}

void SolidSquadBuilder::Layout(
	const USolidSquad* Squad,
	const int32 MaxSquadSize,
	const FView& View,
	FPage& OutPage)
{
	OutPage = FPage();
	const int32 MaxSquad = FMath::Max(0, MaxSquadSize);
	OutPage.MaxSquad = MaxSquad;
	OutPage.Vocation = View.Vocation;
	OutPage.Formation = View.Formation;

	const USolidCompany* Company = Squad ? Squad->GetCompany() : nullptr;
	const TArray<int32> CompanionSlots = Squad
		? Squad->GetSelectedCompanionIndices()
		: TArray<int32>();
	const TArray<int32> PlanSlots = Squad
		? Squad->GetAssignedCompanyPlanIndices()
		: TArray<int32>();

	int32 Filled = 0;
	const int32 Boxed = FMath::Min(MaxSquad, CompanionSlots.Num());
	for (int32 Slot = 0; Slot < Boxed; ++Slot)
	{
		if (CompanionSlots[Slot] != INDEX_NONE)
		{
			++Filled;
		}
	}
	OutPage.FilledSquad = Filled;
	OutPage.Title = FString::Printf(TEXT("Build squad  %d/%d"), Filled, MaxSquad);

	const float CompanyBoxY = HeadingH + BlockGap + ChipH + BlockGap;
	OutPage.CompanyHeading = FRect{ 0.f, 0.f, ListW, HeadingH };
	OutPage.SquadHeading = FRect{ ListW + ColumnGap, 0.f, ColumnW, HeadingH };
	for (int32 Index = 0; Index < VocationFilterCount; ++Index)
	{
		OutPage.VocationChips.Add(ChipRect(
			0.f,
			HeadingH + BlockGap,
			Index,
			VocationFilterCount));
	}
	OutPage.CompanyBox = FRect{ 0.f, CompanyBoxY, ListW, ViewH };
	OutPage.SquadFrame = FRect{ ListW + ColumnGap, CompanyBoxY, ColumnW, ViewH };

	const float PlansTop = BlockH + SectionGap;
	const float PlansBoxY = PlansTop + HeadingH + BlockGap + ChipH + BlockGap;
	OutPage.PlansHeading = FRect{ 0.f, PlansTop, ListW, HeadingH };
	for (int32 Index = 0; Index < FormationFilterCount; ++Index)
	{
		OutPage.FormationChips.Add(ChipRect(
			0.f,
			PlansTop + HeadingH + BlockGap,
			Index,
			FormationFilterCount));
	}
	OutPage.PlansBox = FRect{ 0.f, PlansBoxY, ListW, ViewH };

	{
		TArray<FPendingRow> Pending;
		if (Company)
		{
			float ContentY = 0.f;
			for (int32 Index = 0; Index < Company->GetCompanionCount(); ++Index)
			{
				if (InFirstSlots(CompanionSlots, MaxSquad, Index))
				{
					continue;
				}
				const FSolidCompanion* Companion = Company->GetCompanion(Index);
				if (!Companion || !MatchesVocation(Companion->Vocation, View.Vocation))
				{
					continue;
				}
				FPendingRow Row;
				Row.Index = Index;
				Row.Label = SolidVocation::NameLabel(Companion->Name, Companion->Vocation);
				Row.ContentY = ContentY;
				Pending.Add(Row);
				ContentY += RowPitch;
			}
			OutPage.CompanyContentH = ContentY > 0.f ? ContentY - RowGap : 0.f;
		}
		PlaceRows(
			Pending,
			OutPage.CompanyContentH,
			View.CompanyScroll,
			OutPage.CompanyBox,
			OutPage.CompanyScroll,
			OutPage.bCompanyThumb,
			OutPage.CompanyThumb,
			OutPage.CompanyRows);
	}

	{
		TArray<FPendingRow> Pending;
		float ContentY = 0.f;
		for (int32 Slot = 0; Slot < MaxSquad; ++Slot)
		{
			FPendingRow Box;
			const int32 CompanyIndex = CompanionSlots.IsValidIndex(Slot) ? CompanionSlots[Slot] : INDEX_NONE;
			const FSolidCompanion* Companion = Company ? Company->GetCompanion(CompanyIndex) : nullptr;
			Box.Index = Companion ? CompanyIndex : INDEX_NONE;
			Box.ContentY = ContentY;
			if (Companion)
			{
				Box.Label = SolidVocation::NameLabel(Companion->Name, Companion->Vocation);
			}
			Pending.Add(Box);
			ContentY += RowPitch;
		}
		OutPage.SquadContentH = MaxSquad > 0 ? ContentY - RowGap : 0.f;
		PlaceRows(
			Pending,
			OutPage.SquadContentH,
			View.SquadScroll,
			OutPage.SquadFrame,
			OutPage.SquadScroll,
			OutPage.bSquadThumb,
			OutPage.SquadThumb,
			OutPage.SquadSlots);
	}

	{
		TArray<FPendingRow> Pending;
		if (Company)
		{
			float ContentY = 0.f;
			for (int32 Index = 0; Index < Company->GetBattlePlanCount(); ++Index)
			{
				if (InFirstSlots(PlanSlots, SolidBattleFormationSlots::MaxAssignedBattlePlans, Index))
				{
					continue;
				}
				const FSolidBattlePlan* Plan = Company->GetBattlePlan(Index);
				if (!Plan || !MatchesFormation(Plan->Formation, View.Formation))
				{
					continue;
				}
				FPendingRow Row;
				Row.Index = Index;
				Row.Label = Plan->Name;
				Row.ContentY = ContentY;
				Pending.Add(Row);
				ContentY += RowPitch;
			}
			OutPage.PlansContentH = ContentY > 0.f ? ContentY - RowGap : 0.f;
		}
		PlaceRows(
			Pending,
			OutPage.PlansContentH,
			View.PlansScroll,
			OutPage.PlansBox,
			OutPage.PlansScroll,
			OutPage.bPlansThumb,
			OutPage.PlansThumb,
			OutPage.PlanRows);
	}

	const float KeyW = (ColumnW - KeyGap) / static_cast<float>(KeyColumns);
	const float KeyH = (ViewH - KeyGap * static_cast<float>(KeyRows - 1)) / static_cast<float>(KeyRows);
	OutPage.Keys.Reserve(SolidBattleFormationSlots::MaxAssignedBattlePlans);
	for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
	{
		FEntry Key;
		const int32 PlanIndex = PlanSlots.IsValidIndex(Slot) ? PlanSlots[Slot] : INDEX_NONE;
		const FSolidBattlePlan* Plan = Company ? Company->GetBattlePlan(PlanIndex) : nullptr;
		Key.Index = Plan ? PlanIndex : INDEX_NONE;
		if (Plan)
		{
			Key.Label = Plan->Name;
		}
		const int32 Column = Slot % KeyColumns;
		const int32 Row = Slot / KeyColumns;
		Key.Rect.X = ListW + ColumnGap + static_cast<float>(Column) * (KeyW + KeyGap);
		Key.Rect.Y = PlansBoxY + static_cast<float>(Row) * (KeyH + KeyGap);
		Key.Rect.W = KeyW;
		Key.Rect.H = KeyH;
		Key.bVisible = true;
		OutPage.Keys.Add(Key);
	}

	float ContentW = 0.f;
	float ContentH = 0.f;
	ContentSize(ContentW, ContentH);
	OutPage.Confirm.W = ConfirmW;
	OutPage.Confirm.H = ConfirmH;
	OutPage.Confirm.X = (ContentW - ConfirmW) * 0.5f;
	OutPage.Confirm.Y = ContentH - ConfirmH;
}

SolidSquadBuilder::FHit SolidSquadBuilder::HitTest(const FPage& Page, const float X, const float Y)
{
	FHit Hit;
	for (int32 Index = 0; Index < Page.VocationChips.Num(); ++Index)
	{
		if (Page.VocationChips[Index].Contains(X, Y))
		{
			Hit.Kind = EHit::VocationFilter;
			Hit.Index = Index;
			return Hit;
		}
	}
	for (int32 Index = 0; Index < Page.FormationChips.Num(); ++Index)
	{
		if (Page.FormationChips[Index].Contains(X, Y))
		{
			Hit.Kind = EHit::FormationFilter;
			Hit.Index = Index;
			return Hit;
		}
	}
	for (const FEntry& Row : Page.CompanyRows)
	{
		if (Row.bVisible && Row.Rect.Contains(X, Y))
		{
			Hit.Kind = EHit::CompanyRow;
			Hit.Index = Row.Index;
			return Hit;
		}
	}
	if (Page.CompanyBox.Contains(X, Y))
	{
		Hit.Kind = EHit::CompanyBox;
		return Hit;
	}
	for (int32 Slot = 0; Slot < Page.SquadSlots.Num(); ++Slot)
	{
		const FEntry& Box = Page.SquadSlots[Slot];
		if (Box.bVisible && Box.Rect.Contains(X, Y))
		{
			Hit.Kind = EHit::SquadBox;
			Hit.Index = Slot;
			return Hit;
		}
	}
	for (const FEntry& Row : Page.PlanRows)
	{
		if (Row.bVisible && Row.Rect.Contains(X, Y))
		{
			Hit.Kind = EHit::PlanRow;
			Hit.Index = Row.Index;
			return Hit;
		}
	}
	if (Page.PlansBox.Contains(X, Y))
	{
		Hit.Kind = EHit::PlansBox;
		return Hit;
	}
	for (int32 Slot = 0; Slot < Page.Keys.Num(); ++Slot)
	{
		if (Page.Keys[Slot].Rect.Contains(X, Y))
		{
			Hit.Kind = EHit::KeyBox;
			Hit.Index = Slot;
			return Hit;
		}
	}
	if (Page.Confirm.Contains(X, Y))
	{
		Hit.Kind = EHit::Confirm;
	}
	return Hit;
}

bool SolidSquadBuilder::Scroll(
	const FPage& Page,
	const float X,
	const float Y,
	const float Wheel,
	FView& InOutView)
{
	if (FMath::IsNearlyZero(Wheel))
	{
		return false;
	}
	if (Page.Confirm.Contains(X, Y))
	{
		return false;
	}
	for (const FEntry& Key : Page.Keys)
	{
		if (Key.Rect.Contains(X, Y))
		{
			return false;
		}
	}

	const float Delta = -Wheel * RowPitch;
	if (Page.CompanyBox.Contains(X, Y))
	{
		InOutView.CompanyScroll = ClampScroll(
			InOutView.CompanyScroll + Delta,
			Page.CompanyContentH,
			Page.CompanyBox.H);
		return true;
	}
	if (Page.SquadFrame.Contains(X, Y))
	{
		InOutView.SquadScroll = ClampScroll(
			InOutView.SquadScroll + Delta,
			Page.SquadContentH,
			Page.SquadFrame.H);
		return true;
	}
	if (Page.PlansBox.Contains(X, Y))
	{
		InOutView.PlansScroll = ClampScroll(
			InOutView.PlansScroll + Delta,
			Page.PlansContentH,
			Page.PlansBox.H);
		return true;
	}
	return false;
}

bool SolidSquadBuilder::BeginDrag(const FPage& Page, const FHit& Hit, FDrag& OutDrag)
{
	OutDrag = FDrag();
	if (Hit.Kind == EHit::Confirm)
	{
		return false;
	}
	if (Hit.Kind == EHit::CompanyRow)
	{
		for (const FEntry& Row : Page.CompanyRows)
		{
			if (Row.Index == Hit.Index)
			{
				OutDrag.Kind = EDrag::Companion;
				OutDrag.Index = Row.Index;
				OutDrag.Label = Row.Label;
				return true;
			}
		}
		return false;
	}
	if (Hit.Kind == EHit::SquadBox && Page.SquadSlots.IsValidIndex(Hit.Index))
	{
		const FEntry& Box = Page.SquadSlots[Hit.Index];
		if (Box.Index == INDEX_NONE)
		{
			return false;
		}
		OutDrag.Kind = EDrag::Companion;
		OutDrag.Index = Box.Index;
		OutDrag.Slot = Hit.Index;
		OutDrag.Label = Box.Label;
		return true;
	}
	if (Hit.Kind == EHit::PlanRow)
	{
		for (const FEntry& Row : Page.PlanRows)
		{
			if (Row.Index == Hit.Index)
			{
				OutDrag.Kind = EDrag::Plan;
				OutDrag.Index = Row.Index;
				OutDrag.Label = Row.Label;
				return true;
			}
		}
		return false;
	}
	if (Hit.Kind == EHit::KeyBox && Page.Keys.IsValidIndex(Hit.Index))
	{
		const FEntry& Key = Page.Keys[Hit.Index];
		if (Key.Index == INDEX_NONE)
		{
			return false;
		}
		OutDrag.Kind = EDrag::Plan;
		OutDrag.Index = Key.Index;
		OutDrag.Slot = Hit.Index;
		OutDrag.Label = Key.Label;
		return true;
	}
	return false;
}

bool SolidSquadBuilder::Drop(
	USolidSquad* Squad,
	const int32 MaxSquadSize,
	const FDrag& Drag,
	const FHit& Hit)
{
	if (!Squad || !Squad->GetCompany() || Drag.Kind == EDrag::None || Hit.Kind == EHit::None
		|| Hit.Kind == EHit::Confirm)
	{
		return false;
	}

	if (Drag.Kind == EDrag::Companion)
	{
		const bool bOntoSquad = Hit.Kind == EHit::SquadBox;
		const bool bOntoCompany = Hit.Kind == EHit::CompanyBox || Hit.Kind == EHit::CompanyRow;
		if (!bOntoSquad && !bOntoCompany)
		{
			return false;
		}
		if (!Squad->GetCompany()->GetCompanion(Drag.Index))
		{
			return false;
		}

		const int32 MaxSquad = FMath::Max(0, MaxSquadSize);
		TArray<int32> Slots = Squad->GetSelectedCompanionIndices();
		if (Slots.Num() > MaxSquad)
		{
			Slots.SetNum(MaxSquad);
		}
		while (Slots.Num() < MaxSquad)
		{
			Slots.Add(INDEX_NONE);
		}
		if (MaxSquad <= 0)
		{
			return false;
		}

		if (Drag.Slot == INDEX_NONE)
		{
			if (!bOntoSquad || !Slots.IsValidIndex(Hit.Index) || Slots[Hit.Index] == Drag.Index)
			{
				return false;
			}
			Slots[Hit.Index] = Drag.Index;
		}
		else
		{
			if (!Slots.IsValidIndex(Drag.Slot))
			{
				return false;
			}
			if (bOntoCompany)
			{
				Slots[Drag.Slot] = INDEX_NONE;
			}
			else
			{
				if (!Slots.IsValidIndex(Hit.Index) || Hit.Index == Drag.Slot)
				{
					return false;
				}
				const int32 Displaced = Slots[Hit.Index];
				Slots[Hit.Index] = Slots[Drag.Slot];
				Slots[Drag.Slot] = Displaced;
			}
		}
		return Squad->SetSelectedCompanions(Slots);
	}

	const bool bOntoKey = Hit.Kind == EHit::KeyBox;
	const bool bOntoPlans = Hit.Kind == EHit::PlansBox || Hit.Kind == EHit::PlanRow;
	if (!bOntoKey && !bOntoPlans)
	{
		return false;
	}
	if (!Squad->GetCompany()->GetBattlePlan(Drag.Index))
	{
		return false;
	}

	TArray<int32> Slots = Squad->GetAssignedCompanyPlanIndices();
	Slots.SetNum(SolidBattleFormationSlots::MaxAssignedBattlePlans);
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (!Squad->GetCompany()->GetBattlePlan(Slots[Slot]))
		{
			Slots[Slot] = INDEX_NONE;
		}
	}

	const int32 Active = Squad->GetActiveAssignedSlot();
	const int32 ActivePlan = Slots.IsValidIndex(Active) ? Slots[Active] : INDEX_NONE;
	const ESolidBattleFormation Formation = Squad->GetActiveFormation();
	const ESolidBattleSpacing Spacing = Squad->GetActiveSpacing();

	if (Drag.Slot == INDEX_NONE)
	{
		if (!bOntoKey || !Slots.IsValidIndex(Hit.Index) || Slots[Hit.Index] == Drag.Index)
		{
			return false;
		}
	}
	else if (!Slots.IsValidIndex(Drag.Slot))
	{
		return false;
	}
	else if (!bOntoPlans && (!Slots.IsValidIndex(Hit.Index) || Hit.Index == Drag.Slot))
	{
		return false;
	}

	const int32 Displaced = (Drag.Slot != INDEX_NONE && bOntoKey) ? Slots[Hit.Index] : INDEX_NONE;
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		if (Slots[Slot] == Drag.Index)
		{
			Slots[Slot] = INDEX_NONE;
		}
	}
	if (bOntoKey)
	{
		Slots[Hit.Index] = Drag.Index;
		if (Drag.Slot != INDEX_NONE)
		{
			Slots[Drag.Slot] = Displaced;
		}
	}

	Squad->SetHeldFormation(Formation, Spacing);
	Squad->SetAssignedBattlePlans(Slots);
	FollowActivePlan(Squad, ActivePlan, Slots, Formation, Spacing);
	return true;
}

bool SolidSquadBuilder::HasAnythingToArrange(const USolidSquad* Squad, const int32 MaxSquadSize)
{
	const USolidCompany* Company = Squad ? Squad->GetCompany() : nullptr;
	const bool bNoCompanions = MaxSquadSize <= 0 || !Company || Company->GetCompanionCount() <= 0;
	if (!bNoCompanions)
	{
		return true;
	}
	if (!Company || Company->GetBattlePlanCount() == 0 || Company->HasOnlyDefaultBattlePlans())
	{
		return false;
	}
	return true;
}

bool SolidSquadBuilder::OpenAfterStrongholdLeave(
	ASolidHUD* HUD,
	const USolidSquad* Squad,
	const int32 MaxSquadSize)
{
	if (!HUD || !HasAnythingToArrange(Squad, MaxSquadSize))
	{
		return false;
	}
	HUD->OpenSquadBuilder();
	return true;
}
