#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Menus/SolidSquadBuilder.h"
#include "Squad/SolidBattlePlan.h"
#include "Squad/SolidCompany.h"
#include "Squad/SolidSquad.h"

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
	TestEqual(TEXT("F1 is Line"), Page.Keys[0].Label, FString(TEXT("Line")));
	TestEqual(TEXT("F2 is Column"), Page.Keys[1].Label, FString(TEXT("Column")));
	TestEqual(TEXT("F3 is Tight mob"), Page.Keys[2].Label, FString(TEXT("Tight mob")));
	TestEqual(TEXT("F4 is Loose mob"), Page.Keys[3].Label, FString(TEXT("Loose mob")));
	TestEqual(TEXT("F5 is Parade"), Page.Keys[4].Label, FString(TEXT("Parade")));
	TestTrue(TEXT("F6 is empty"), Page.Keys[5].Label.IsEmpty());
	TestTrue(TEXT("F7 is empty"), Page.Keys[6].Label.IsEmpty());
	TestTrue(TEXT("F8 is empty"), Page.Keys[7].Label.IsEmpty());
	TestEqual(TEXT("opens on All companions"), static_cast<uint8>(Page.Vocation), static_cast<uint8>(SolidSquadBuilder::EVocationFilter::All));
	TestEqual(TEXT("opens on All plans"), static_cast<uint8>(Page.Formation), static_cast<uint8>(SolidSquadBuilder::EFormationFilter::All));

	const int32 MaxIndex = Company->FindCompanionIndexByName(TEXT("Max"));
	const int32 Morgan = Company->FindCompanionIndexByName(TEXT("Morgan"));
	const int32 Sam = Company->FindCompanionIndexByName(TEXT("Sam"));
	const int32 Alex = Company->FindCompanionIndexByName(TEXT("Alex"));

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
	const int32 Line = 0;
	const int32 Column = 1;
	TestTrue(
		TEXT("the active plan moves to F4"),
		SolidSquadBuilder::Drop(Squad, 2, PlanDrag(Line, 0), Hit(SolidSquadBuilder::EHit::KeyBox, 3)));
	TestEqual(TEXT("the mark follows Line"), Squad->GetActiveAssignedSlot(), 3);
	TestTrue(TEXT("formation stays Line"), Squad->GetActiveFormation() == ESolidBattleFormation::Line);
	TestEqual(TEXT("F4 holds Line"), Squad->GetAssignedBattlePlan(3)->Name, FString(TEXT("Line")));
	TestEqual(TEXT("F1 now holds Loose mob"), Squad->GetAssignedBattlePlan(0)->Name, FString(TEXT("Loose mob")));
	int32 LineKeys = 0;
	for (const int32 PlanIndex : Squad->GetAssignedCompanyPlanIndices())
	{
		if (PlanIndex == Line)
		{
			++LineKeys;
		}
	}
	TestEqual(TEXT("Line is on one key"), LineKeys, 1);

	TestTrue(
		TEXT("Line returns to Plans"),
		SolidSquadBuilder::Drop(Squad, 2, PlanDrag(Line, 3), Hit(SolidSquadBuilder::EHit::PlansBox, INDEX_NONE)));
	TestEqual(TEXT("no plan is active"), Squad->GetActiveAssignedSlot(), INDEX_NONE);
	TestTrue(TEXT("companions keep Line"), Squad->GetActiveFormation() == ESolidBattleFormation::Line);
	TestTrue(TEXT("spacing stays standard"), Squad->GetActiveSpacing() == ESolidBattleSpacing::Standard);
	TestNull(TEXT("F4 is empty"), Squad->GetAssignedBattlePlan(3));
	TestFalse(TEXT("an empty key does nothing"), Squad->SelectAssignedSlot(3));
	TestEqual(TEXT("the empty key did not select itself"), Squad->GetActiveAssignedSlot(), INDEX_NONE);

	TestFalse(
		TEXT("a plan misses a squad box"),
		SolidSquadBuilder::Drop(Squad, 2, PlanDrag(Column, 1), Hit(SolidSquadBuilder::EHit::SquadBox, 0)));
	TestEqual(TEXT("Column stays on F2"), Squad->GetAssignedBattlePlan(1)->Name, FString(TEXT("Column")));
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

#endif // WITH_DEV_AUTOMATION_TESTS
