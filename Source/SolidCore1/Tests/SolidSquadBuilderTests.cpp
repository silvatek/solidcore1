#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Journal/SolidJournal.h"
#include "Menus/SolidSquadBuilder.h"
#include "SolidHUD.h"
#include "BattlePlans/SolidBattlePlan.h"
#include "Company/SolidCompany.h"
#include "Company/SolidSquad.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidSquadBuilderTestPrivate
{
	int32 VisibleCount(const TArray<SolidSquadBuilder::FEntry>& Rows)
	{
		int32 Count = 0;
		for (const SolidSquadBuilder::FEntry& Row : Rows)
		{
			if (Row.bVisible)
			{
				++Count;
			}
		}
		return Count;
	}

	bool HasLabel(const TArray<SolidSquadBuilder::FEntry>& Rows, const TCHAR* Label)
	{
		for (const SolidSquadBuilder::FEntry& Row : Rows)
		{
			if (Row.Label == Label)
			{
				return true;
			}
		}
		return false;
	}

	SolidSquadBuilder::FDrag CompanionDrag(const int32 CompanyIndex, const int32 Slot)
	{
		SolidSquadBuilder::FDrag Drag;
		Drag.Kind = SolidSquadBuilder::EDrag::Companion;
		Drag.Index = CompanyIndex;
		Drag.Slot = Slot;
		return Drag;
	}

	SolidSquadBuilder::FDrag PlanDrag(const int32 PlanIndex, const int32 Slot)
	{
		SolidSquadBuilder::FDrag Drag;
		Drag.Kind = SolidSquadBuilder::EDrag::Plan;
		Drag.Index = PlanIndex;
		Drag.Slot = Slot;
		return Drag;
	}

	SolidSquadBuilder::FHit Hit(const SolidSquadBuilder::EHit Kind, const int32 Index)
	{
		SolidSquadBuilder::FHit Result;
		Result.Kind = Kind;
		Result.Index = Index;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadBuilderDragTest,
	"SolidCore1.SquadBuilder.Drag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadBuilderDragTest::RunTest(const FString& Parameters)
{
	using namespace SolidSquadBuilderTestPrivate;

	TestFalse(TEXT("locked at the start"), SolidSquadBuilder::CanOpen(0));
	TestFalse(TEXT("locked with one companion allowed"), SolidSquadBuilder::CanOpen(1));
	TestTrue(TEXT("opens once two are allowed"), SolidSquadBuilder::CanOpen(2));

	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);
	Squad->InitializeFromCompany(Company);
	const int32 Sam = Company->FindCompanionIndexByName(TEXT("Sam"));
	const int32 Alex = Company->FindCompanionIndexByName(TEXT("Alex"));
	TestTrue(TEXT("the company has Sam and Alex"), Sam != INDEX_NONE && Alex != INDEX_NONE);
	TestTrue(
		TEXT("the page starts with Sam and Alex in the boxes"),
		Squad->SetSelectedCompanions({ Sam, Alex }));

	SolidSquadBuilder::FView View;
	SolidSquadBuilder::FPage Page;
	SolidSquadBuilder::Layout(Squad, 2, View, Page);
	TestEqual(TEXT("title counts filled boxes"), Page.Title, FString(TEXT("Build squad  2/2")));
	TestEqual(TEXT("two squad boxes"), Page.SquadSlots.Num(), 2);
	TestEqual(TEXT("Sam is first"), Page.SquadSlots[0].Label, FString(TEXT("Sam (R)")));
	TestEqual(TEXT("Alex is second"), Page.SquadSlots[1].Label, FString(TEXT("Alex (W)")));
	TestEqual(TEXT("three stay in the company"), Page.CompanyRows.Num(), 3);
	TestEqual(TEXT("Max stays next"), Page.CompanyRows[0].Label, FString(TEXT("Max (C)")));
	TestEqual(TEXT("Morgan stays next"), Page.CompanyRows[1].Label, FString(TEXT("Morgan (M)")));
	TestEqual(TEXT("Quinn stays last"), Page.CompanyRows[2].Label, FString(TEXT("Quinn (W)")));
	TestFalse(TEXT("company thumb stays hidden"), Page.bCompanyThumb);
	TestFalse(TEXT("plans thumb stays hidden"), Page.bPlansThumb);
	TestFalse(TEXT("squad thumb stays hidden"), Page.bSquadThumb);
	TestEqual(TEXT("plans list is empty"), Page.PlanRows.Num(), 0);
	TestEqual(TEXT("eight keys"), Page.Keys.Num(), 8);
	int32 FilledKeys = 0;
	for (int32 Slot = 0; Slot < Page.Keys.Num(); ++Slot)
	{
		const FSolidBattlePlan* Plan = Squad->GetAssignedBattlePlan(Slot);
		if (Plan)
		{
			TestEqual(
				*FString::Printf(TEXT("F%d shows its plan"), Slot + 1),
				Page.Keys[Slot].Label,
				Plan->Name);
			++FilledKeys;
		}
		else
		{
			TestTrue(
				*FString::Printf(TEXT("F%d is empty"), Slot + 1),
				Page.Keys[Slot].Label.IsEmpty());
		}
	}
	TestEqual(TEXT("every assigned plan is on a key"), FilledKeys, Squad->GetAssignedCount());
	TestTrue(TEXT("F1 holds a plan"), Squad->GetAssignedBattlePlan(0) != nullptr);
	TestEqual(TEXT("opens on All companions"), static_cast<uint8>(Page.Vocation), static_cast<uint8>(SolidSquadBuilder::EVocationFilter::All));
	TestEqual(TEXT("opens on All plans"), static_cast<uint8>(Page.Formation), static_cast<uint8>(SolidSquadBuilder::EFormationFilter::All));

	const int32 MaxIndex = Company->FindCompanionIndexByName(TEXT("Max"));
	const int32 Morgan = Company->FindCompanionIndexByName(TEXT("Morgan"));

	SolidSquadBuilder::Layout(Squad, 4, View, Page);
	TestTrue(
		TEXT("empty squad box takes Max"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(MaxIndex, INDEX_NONE), Hit(SolidSquadBuilder::EHit::SquadBox, 2)));
	TArray<FSolidCompanion> Fielded;
	Squad->CollectFieldedCompanions(4, Fielded);
	TestEqual(TEXT("three walk"), Fielded.Num(), 3);
	if (Fielded.Num() == 3)
	{
		TestEqual(TEXT("Sam still leads"), Fielded[0].Name, FString(TEXT("Sam")));
		TestEqual(TEXT("Alex stays second"), Fielded[1].Name, FString(TEXT("Alex")));
		TestEqual(TEXT("Max is third"), Fielded[2].Name, FString(TEXT("Max")));
	}

	TestTrue(
		TEXT("occupied box swaps"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(Morgan, INDEX_NONE), Hit(SolidSquadBuilder::EHit::SquadBox, 0)));
	Squad->CollectFieldedCompanions(4, Fielded);
	TestEqual(TEXT("still three"), Fielded.Num(), 3);
	if (Fielded.Num() == 3)
	{
		TestEqual(TEXT("Morgan took the first box"), Fielded[0].Name, FString(TEXT("Morgan")));
		TestEqual(TEXT("Alex stayed"), Fielded[1].Name, FString(TEXT("Alex")));
		TestEqual(TEXT("Max stayed"), Fielded[2].Name, FString(TEXT("Max")));
	}
	SolidSquadBuilder::Layout(Squad, 4, View, Page);
	TestTrue(TEXT("Sam returned to the company"), HasLabel(Page.CompanyRows, TEXT("Sam (R)")));
	TestFalse(TEXT("Morgan left the company"), HasLabel(Page.CompanyRows, TEXT("Morgan (M)")));

	TestFalse(
		TEXT("same box does nothing"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(Morgan, 0), Hit(SolidSquadBuilder::EHit::SquadBox, 0)));
	TestFalse(
		TEXT("a miss does nothing"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(Morgan, 0), Hit(SolidSquadBuilder::EHit::None, INDEX_NONE)));
	TestFalse(
		TEXT("a companion misses a key"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(Morgan, 0), Hit(SolidSquadBuilder::EHit::KeyBox, 0)));
	TestEqual(TEXT("Morgan is still selected"), Squad->GetSelectedCompanionIndices()[0], Morgan);

	TestTrue(
		TEXT("company box clears the squad box"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(Alex, 1), Hit(SolidSquadBuilder::EHit::CompanyBox, INDEX_NONE)));
	const TArray<int32>& Gapped = Squad->GetSelectedCompanionIndices();
	TestEqual(TEXT("hole stays between Morgan and Max"), Gapped.Num(), 3);
	TestEqual(TEXT("first box is Morgan"), Gapped[0], Morgan);
	TestEqual(TEXT("second box is empty"), Gapped[1], INDEX_NONE);
	TestEqual(TEXT("third box is Max"), Gapped[2], MaxIndex);
	Squad->CollectFieldedCompanions(4, Fielded);
	TestEqual(TEXT("the gap is skipped"), Fielded.Num(), 2);
	if (Fielded.Num() == 2)
	{
		TestEqual(TEXT("Morgan still leads"), Fielded[0].Name, FString(TEXT("Morgan")));
		TestEqual(TEXT("Max follows the gap"), Fielded[1].Name, FString(TEXT("Max")));
	}
	Squad->CollectFieldedCompanions(2, Fielded);
	TestEqual(TEXT("the cap stops at the second box"), Fielded.Num(), 1);
	if (Fielded.Num() == 1)
	{
		TestEqual(TEXT("only Morgan is inside the cap"), Fielded[0].Name, FString(TEXT("Morgan")));
	}

	View.CompanyScroll = 80.f;
	SolidSquadBuilder::ChooseVocation(View, SolidSquadBuilder::EVocationFilter::Rogue);
	TestEqual(TEXT("a new filter starts at the top"), View.CompanyScroll, 0.f);
	SolidSquadBuilder::Layout(Squad, 4, View, Page);
	TestTrue(TEXT("Morgan stays in the squad under Rogue"), Page.SquadSlots[0].Label.Contains(TEXT("Morgan (M)")));
	TestTrue(TEXT("Sam is the rogue row"), HasLabel(Page.CompanyRows, TEXT("Sam (R)")));
	TestFalse(TEXT("Alex is hidden by Rogue"), HasLabel(Page.CompanyRows, TEXT("Alex (W)")));
	SolidSquadBuilder::FDrag Began;
	TestTrue(TEXT("a shown row can be pressed"), SolidSquadBuilder::BeginDrag(
		Page,
		SolidSquadBuilder::HitTest(Page, Page.CompanyRows[0].Rect.X + 1.f, Page.CompanyRows[0].Rect.Y + 1.f),
		Began));
	TestEqual(TEXT("the shown row is Sam"), Began.Index, Sam);
	TestFalse(
		TEXT("an empty squad box cannot be pressed"),
		SolidSquadBuilder::BeginDrag(Page, Hit(SolidSquadBuilder::EHit::SquadBox, 1), Began));

	SolidSquadBuilder::ChooseVocation(View, SolidSquadBuilder::EVocationFilter::Warrior);
	TestTrue(
		TEXT("Sam takes the empty box while Warrior hides him"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(Sam, INDEX_NONE), Hit(SolidSquadBuilder::EHit::SquadBox, 1)));
	SolidSquadBuilder::Layout(Squad, 4, View, Page);
	TestFalse(TEXT("Warrior still hides Sam"), HasLabel(Page.CompanyRows, TEXT("Sam (R)")));
	TestEqual(TEXT("Sam is in the second box"), Page.SquadSlots[1].Label, FString(TEXT("Sam (R)")));
	TestTrue(
		TEXT("Sam returns to the company while Warrior hides him"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(Sam, 1), Hit(SolidSquadBuilder::EHit::CompanyBox, INDEX_NONE)));
	SolidSquadBuilder::ChooseVocation(View, SolidSquadBuilder::EVocationFilter::All);
	SolidSquadBuilder::Layout(Squad, 4, View, Page);
	TestTrue(TEXT("All lists Sam again"), HasLabel(Page.CompanyRows, TEXT("Sam (R)")));

	USolidSquad* Bare = NewObject<USolidSquad>();
	TestFalse(
		TEXT("no squad"),
		SolidSquadBuilder::Drop(nullptr, 2, CompanionDrag(Sam, INDEX_NONE), Hit(SolidSquadBuilder::EHit::SquadBox, 0)));
	TestFalse(
		TEXT("unbound squad"),
		SolidSquadBuilder::Drop(Bare, 2, CompanionDrag(Sam, INDEX_NONE), Hit(SolidSquadBuilder::EHit::SquadBox, 0)));
	TestFalse(
		TEXT("unknown companion"),
		SolidSquadBuilder::Drop(Squad, 4, CompanionDrag(99, INDEX_NONE), Hit(SolidSquadBuilder::EHit::SquadBox, 3)));

	TArray<int32> Full;
	Full.Add(Sam);
	Full.Add(Alex);
	TestTrue(TEXT("fill the two boxes"), Squad->SetSelectedCompanions(Full));
	const int32 Before = Squad->GetSelectedCompanionCount();
	TestTrue(
		TEXT("a full column still swaps"),
		SolidSquadBuilder::Drop(Squad, 2, CompanionDrag(MaxIndex, INDEX_NONE), Hit(SolidSquadBuilder::EHit::SquadBox, 0)));
	TestEqual(TEXT("the cap still holds"), Squad->GetSelectedCompanionCount(), Before);
	TestTrue(
		TEXT("a full column can drop back on Company"),
		SolidSquadBuilder::Drop(Squad, 2, CompanionDrag(MaxIndex, 0), Hit(SolidSquadBuilder::EHit::CompanyBox, INDEX_NONE)));
	TestTrue(TEXT("the column is no longer full"), Squad->GetSelectedCompanionCount() < 2);

	TestTrue(TEXT("F1 starts active"), Squad->SelectAssignedSlot(0));
	const FSolidBattlePlan* FromPlan = Squad->GetAssignedBattlePlan(0);
	const FSolidBattlePlan* ToPlan = Squad->GetAssignedBattlePlan(3);
	const FSolidBattlePlan* StayPlan = Squad->GetAssignedBattlePlan(1);
	TestNotNull(TEXT("F1 has a plan"), FromPlan);
	TestNotNull(TEXT("F4 has a plan"), ToPlan);
	TestNotNull(TEXT("F2 has a plan"), StayPlan);
	if (!FromPlan || !ToPlan || !StayPlan)
	{
		return false;
	}
	const int32 FromIndex = Squad->GetAssignedCompanyPlanIndices()[0];
	const int32 StayIndex = Squad->GetAssignedCompanyPlanIndices()[1];
	const FString FromName = FromPlan->Name;
	const FString ToName = ToPlan->Name;
	const FString StayName = StayPlan->Name;
	const ESolidBattleFormation FromFormation = FromPlan->Formation;
	const ESolidBattleSpacing FromSpacing = FromPlan->Spacing;
	TestTrue(
		TEXT("the active plan moves to F4"),
		SolidSquadBuilder::Drop(Squad, 2, PlanDrag(FromIndex, 0), Hit(SolidSquadBuilder::EHit::KeyBox, 3)));
	TestEqual(TEXT("the mark follows that plan"), Squad->GetActiveAssignedSlot(), 3);
	TestTrue(TEXT("formation stays"), Squad->GetActiveFormation() == FromFormation);
	TestEqual(TEXT("F4 holds the plan from F1"), Squad->GetAssignedBattlePlan(3)->Name, FromName);
	TestEqual(TEXT("F1 holds the plan from F4"), Squad->GetAssignedBattlePlan(0)->Name, ToName);
	int32 FromKeys = 0;
	for (const int32 PlanIndex : Squad->GetAssignedCompanyPlanIndices())
	{
		if (PlanIndex == FromIndex)
		{
			++FromKeys;
		}
	}
	TestEqual(TEXT("that plan is on one key"), FromKeys, 1);

	TestTrue(
		TEXT("that plan returns to Plans"),
		SolidSquadBuilder::Drop(Squad, 2, PlanDrag(FromIndex, 3), Hit(SolidSquadBuilder::EHit::PlansBox, INDEX_NONE)));
	TestEqual(TEXT("no plan is active"), Squad->GetActiveAssignedSlot(), INDEX_NONE);
	TestTrue(TEXT("companions keep the formation"), Squad->GetActiveFormation() == FromFormation);
	TestTrue(TEXT("spacing stays"), Squad->GetActiveSpacing() == FromSpacing);
	TestNull(TEXT("F4 is empty"), Squad->GetAssignedBattlePlan(3));
	TestFalse(TEXT("an empty key does nothing"), Squad->SelectAssignedSlot(3));
	TestEqual(TEXT("the empty key did not select itself"), Squad->GetActiveAssignedSlot(), INDEX_NONE);

	TestFalse(
		TEXT("a plan misses a squad box"),
		SolidSquadBuilder::Drop(Squad, 2, PlanDrag(StayIndex, 1), Hit(SolidSquadBuilder::EHit::SquadBox, 0)));
	TestEqual(TEXT("F2 stays"), Squad->GetAssignedBattlePlan(1)->Name, StayName);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadBuilderScrollTest,
	"SolidCore1.SquadBuilder.Scroll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadBuilderScrollTest::RunTest(const FString& Parameters)
{
	using namespace SolidSquadBuilderTestPrivate;

	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);
	Squad->InitializeFromCompany(Company);

	constexpr int32 ExtraCompanions = 24;
	for (int32 Index = 0; Index < ExtraCompanions; ++Index)
	{
		FSolidCompanion Companion;
		Companion.Name = FString::Printf(TEXT("Extra%d"), Index);
		Companion.Vocation = (Index % 2 == 0) ? ESolidVocation::Mage : ESolidVocation::Warrior;
		Company->AddCompanion(Companion);
	}
	constexpr int32 ExtraPlans = 12;
	for (int32 Index = 0; Index < ExtraPlans; ++Index)
	{
		FSolidBattlePlan Plan;
		Plan.Name = FString::Printf(TEXT("ExtraPlan%d"), Index);
		Plan.Formation = (Index % 2 == 0) ? ESolidBattleFormation::Mob : ESolidBattleFormation::Line;
		Plan.Spacing = ESolidBattleSpacing::Standard;
		Company->AddBattlePlan(Plan);
	}

	const int32 MaxSquad = Company->GetCompanionCount();
	SolidSquadBuilder::FView View;
	SolidSquadBuilder::FPage Page;
	SolidSquadBuilder::Layout(Squad, MaxSquad, View, Page);
	TestTrue(TEXT("a long company shows a thumb"), Page.bCompanyThumb);
	TestTrue(TEXT("a long catalog shows a thumb"), Page.bPlansThumb);
	TestTrue(TEXT("a tall squad column shows a thumb"), Page.bSquadThumb);
	TestEqual(TEXT("company window"), VisibleCount(Page.CompanyRows), SolidSquadBuilder::WindowRows);
	TestTrue(TEXT("the company window is shorter than the list"), Page.CompanyRows.Num() > SolidSquadBuilder::WindowRows);
	TestTrue(TEXT("the first company row is in the window"), Page.CompanyRows[0].bVisible);
	TestFalse(TEXT("the last company row waits below"), Page.CompanyRows.Last().bVisible);
	TestEqual(TEXT("plans window"), VisibleCount(Page.PlanRows), SolidSquadBuilder::WindowRows);
	TestTrue(TEXT("the catalog window is shorter than the list"), Page.PlanRows.Num() > SolidSquadBuilder::WindowRows);
	TestFalse(TEXT("the last plan waits below"), Page.PlanRows.Last().bVisible);

	const float CompanyX = Page.CompanyBox.X + 8.f;
	const float CompanyY = Page.CompanyBox.Y + 8.f;
	TestTrue(TEXT("the company wheel scrolls"), SolidSquadBuilder::Scroll(Page, CompanyX, CompanyY, -1.f, View));
	TestTrue(TEXT("company moved down"), View.CompanyScroll > 0.f);
	TestEqual(TEXT("plans stayed"), View.PlansScroll, 0.f);
	TestEqual(TEXT("the squad column stayed"), View.SquadScroll, 0.f);
	const float CompanyScroll = View.CompanyScroll;

	const float KeyX = Page.Keys[0].Rect.X + 2.f;
	const float KeyY = Page.Keys[0].Rect.Y + 2.f;
	TestFalse(TEXT("a key ignores the wheel"), SolidSquadBuilder::Scroll(Page, KeyX, KeyY, -1.f, View));
	TestEqual(TEXT("the key left the company where it was"), View.CompanyScroll, CompanyScroll);

	const float SquadX = Page.SquadFrame.X + 8.f;
	const float SquadY = Page.SquadFrame.Y + 8.f;
	TestTrue(TEXT("the squad wheel scrolls"), SolidSquadBuilder::Scroll(Page, SquadX, SquadY, -1.f, View));
	TestTrue(TEXT("the column moved"), View.SquadScroll > 0.f);
	TestEqual(TEXT("company did not move with the column"), View.CompanyScroll, CompanyScroll);

	View.CompanyScroll = 400.f;
	SolidSquadBuilder::ChooseVocation(View, SolidSquadBuilder::EVocationFilter::Mage);
	TestEqual(TEXT("Mage starts at the top"), View.CompanyScroll, 0.f);
	SolidSquadBuilder::Layout(Squad, MaxSquad, View, Page);
	TestTrue(TEXT("Mage still needs a thumb"), Page.bCompanyThumb);
	for (const SolidSquadBuilder::FEntry& Row : Page.CompanyRows)
	{
		const FSolidCompanion* Companion = Company->GetCompanion(Row.Index);
		TestTrue(TEXT("Mage hides the other vocations"), Companion && Companion->Vocation == ESolidVocation::Mage);
	}
	TestFalse(TEXT("a warrior extra is hidden"), HasLabel(Page.CompanyRows, TEXT("Extra1 (W)")));
	TestTrue(TEXT("the first mage is back in the window"), Page.CompanyRows.Num() > 0 && Page.CompanyRows[0].bVisible);

	View.PlansScroll = 400.f;
	SolidSquadBuilder::ChooseFormation(View, SolidSquadBuilder::EFormationFilter::Mob);
	TestEqual(TEXT("Mob starts at the top"), View.PlansScroll, 0.f);
	SolidSquadBuilder::Layout(Squad, MaxSquad, View, Page);
	for (const SolidSquadBuilder::FEntry& Row : Page.PlanRows)
	{
		const FSolidBattlePlan* Plan = Company->GetBattlePlan(Row.Index);
		TestTrue(TEXT("Mob hides the other formations"), Plan && Plan->Formation == ESolidBattleFormation::Mob);
	}
	TestTrue(TEXT("Mob lists a tight or loose extra"), Page.PlanRows.Num() > 0);
	TestFalse(TEXT("a line extra is hidden"), HasLabel(Page.PlanRows, TEXT("ExtraPlan1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadBuilderConfirmTest,
	"SolidCore1.SquadBuilder.Confirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadBuilderConfirmTest::RunTest(const FString& Parameters)
{
	using namespace SolidSquadBuilderTestPrivate;

	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);
	Squad->InitializeFromCompany(Company);

	SolidSquadBuilder::FView View;
	SolidSquadBuilder::FPage Page;
	SolidSquadBuilder::Layout(Squad, 2, View, Page);

	float ContentW = 0.f;
	float ContentH = 0.f;
	SolidSquadBuilder::ContentSize(ContentW, ContentH);
	TestEqual(TEXT("Confirm sits on the bottom of the page"), Page.Confirm.Y + Page.Confirm.H, ContentH);
	TestTrue(TEXT("Confirm is below the plans box"), Page.Confirm.Y >= Page.PlansBox.Y + Page.PlansBox.H);
	TestTrue(TEXT("Confirm is below the keys"), Page.Keys.Num() == 8 && Page.Confirm.Y >= Page.Keys[7].Rect.Y + Page.Keys[7].Rect.H);
	TestEqual(TEXT("Confirm label"), FString(SolidSquadBuilder::ConfirmLabel), FString(TEXT("Confirm")));

	TestTrue(
		TEXT("Confirm fits the journal board"),
		ContentW + USolidJournal::BoardPad * 2.f <= USolidJournal::MinBoardWidth
		&& ContentH + USolidJournal::BoardPad * 2.f <= USolidJournal::MinBoardHeight);
	float BoardW = 0.f;
	float BoardH = 0.f;
	USolidJournal::BoardSize(1920.f, 1080.f, ContentW, ContentH, BoardW, BoardH);
	TestTrue(
		TEXT("a roomy view stays the journal share"),
		FMath::IsNearlyEqual(BoardW, 1920.f * USolidJournal::BoardWidthFraction, 0.5f)
		&& FMath::IsNearlyEqual(BoardH, 1080.f * USolidJournal::BoardHeightFraction, 0.5f));

	const SolidSquadBuilder::FHit Button = SolidSquadBuilder::HitTest(
		Page,
		Page.Confirm.X + 1.f,
		Page.Confirm.Y + 1.f);
	TestEqual(TEXT("the button is Confirm"), static_cast<uint8>(Button.Kind), static_cast<uint8>(SolidSquadBuilder::EHit::Confirm));
	SolidSquadBuilder::FDrag Began;
	TestFalse(TEXT("Confirm is not a drag"), SolidSquadBuilder::BeginDrag(Page, Button, Began));
	TestFalse(TEXT("the wheel ignores Confirm"), SolidSquadBuilder::Scroll(
		Page,
		Page.Confirm.X + 1.f,
		Page.Confirm.Y + 1.f,
		-1.f,
		View));
	TestEqual(TEXT("Confirm did not scroll the company"), View.CompanyScroll, 0.f);

	const TArray<int32> CompanionsBefore = Squad->GetSelectedCompanionIndices();
	const TArray<int32> PlansBefore = Squad->GetAssignedCompanyPlanIndices();
	const int32 ActiveBefore = Squad->GetActiveAssignedSlot();
	TestFalse(
		TEXT("a companion dropped on Confirm stays put"),
		SolidSquadBuilder::Drop(Squad, 2, CompanionDrag(0, 0), Hit(SolidSquadBuilder::EHit::Confirm, INDEX_NONE)));
	TestFalse(
		TEXT("a plan dropped on Confirm stays put"),
		SolidSquadBuilder::Drop(Squad, 2, PlanDrag(0, 0), Hit(SolidSquadBuilder::EHit::Confirm, INDEX_NONE)));
	TestTrue(TEXT("the squad is unchanged"), Squad->GetSelectedCompanionIndices() == CompanionsBefore);
	TestTrue(TEXT("the keys are unchanged"), Squad->GetAssignedCompanyPlanIndices() == PlansBefore);
	TestEqual(TEXT("the active plan is unchanged"), Squad->GetActiveAssignedSlot(), ActiveBefore);

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	HUD->ConfirmSquadBuilder();
	TestFalse(TEXT("Confirm on a closed page stays closed"), HUD->IsSquadBuilderOpen());
	HUD->OpenSquadBuilder();
	TestTrue(TEXT("the page opens"), HUD->IsSquadBuilderOpen());
	HUD->ConfirmSquadBuilder();
	TestFalse(TEXT("Confirm closes the page"), HUD->IsSquadBuilderOpen());
	TestFalse(TEXT("Confirm does not open the menu"), HUD->IsMainMenuOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadBuilderStrongholdLeaveTest,
	"SolidCore1.SquadBuilder.StrongholdLeave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadBuilderStrongholdLeaveTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);
	Squad->InitializeFromCompany(Company);
	TestTrue(TEXT("the company starts with the five plans"), Company->HasOnlyDefaultBattlePlans());
	TestEqual(TEXT("the squad starts empty"), Squad->GetSelectedCompanionCount(), 0);
	TestFalse(TEXT("the menu stays locked below two"), SolidSquadBuilder::CanOpen(1));
	TestFalse(
		TEXT("no companion and the starter plans have nothing to arrange"),
		SolidSquadBuilder::HasAnythingToArrange(Squad, 0));

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	const TArray<int32> Selected = Squad->GetSelectedCompanionIndices();
	TestFalse(
		TEXT("that leave leaves the page closed"),
		SolidSquadBuilder::OpenAfterStrongholdLeave(HUD, Squad, 0));
	TestFalse(TEXT("the page stayed closed"), HUD->IsSquadBuilderOpen());
	TestTrue(TEXT("the squad was not changed"), Squad->GetSelectedCompanionIndices() == Selected);

	TestTrue(
		TEXT("one companion is something to arrange"),
		SolidSquadBuilder::HasAnythingToArrange(Squad, 1));
	TestTrue(
		TEXT("one companion opens the page"),
		SolidSquadBuilder::OpenAfterStrongholdLeave(HUD, Squad, 1));
	TestTrue(TEXT("Build squad is open"), HUD->IsSquadBuilderOpen());
	TestTrue(TEXT("the leave did not fill the squad"), Squad->GetSelectedCompanionIndices() == Selected);
	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes the page"), HUD->IsSquadBuilderOpen());
	TestFalse(TEXT("F10 does not open the menu"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("closing does not open it again"), HUD->IsSquadBuilderOpen());

	TestTrue(
		TEXT("the next leave opens it again"),
		SolidSquadBuilder::OpenAfterStrongholdLeave(HUD, Squad, 2));
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes the page"), HUD->IsSquadBuilderOpen());
	TestFalse(TEXT("Esc does not open the menu"), HUD->IsMainMenuOpen());
	HUD->OpenSquadBuilder();
	TestTrue(TEXT("the menu can still open Build squad"), HUD->IsSquadBuilderOpen());
	HUD->CloseMenuOverlay();

	FSolidBattlePlan Extra;
	Extra.Name = TEXT("Hold");
	Extra.Formation = ESolidBattleFormation::Line;
	Extra.Spacing = ESolidBattleSpacing::Standard;
	Company->AddBattlePlan(Extra);
	TestFalse(TEXT("a further plan is not the starter catalog"), Company->HasOnlyDefaultBattlePlans());
	TestTrue(
		TEXT("a further plan is something to arrange with no companion allowed"),
		SolidSquadBuilder::HasAnythingToArrange(Squad, 0));
	TestTrue(
		TEXT("that plan opens the page"),
		SolidSquadBuilder::OpenAfterStrongholdLeave(HUD, Squad, 0));
	TestTrue(TEXT("Build squad opened for the plan"), HUD->IsSquadBuilderOpen());
	TestTrue(TEXT("the squad is still unchanged"), Squad->GetSelectedCompanionIndices() == Selected);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
