#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Party/SolidBattlePlan.h"
#include "Party/SolidCompany.h"
#include "Party/SolidParty.h"
#include "SolidGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleFormationSlotsTest,
	"SolidCore1.BattlePlan.FormationSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleFormationSlotsTest::RunTest(const FString& Parameters)
{
	constexpr int32 Count = 2;

	const FVector2D Line0 = SolidBattleFormationSlots::SlotOffset(ESolidBattleFormation::Line, 0, Count);
	const FVector2D Line1 = SolidBattleFormationSlots::SlotOffset(ESolidBattleFormation::Line, 1, Count);
	TestTrue(TEXT("line: both slightly behind"), Line0.X < 0.f && Line1.X < 0.f);
	TestTrue(TEXT("line: opposite flanks"), Line0.Y * Line1.Y < 0.f);
	TestEqual(TEXT("line: same depth"), Line0.X, Line1.X);

	const FVector2D Col0 = SolidBattleFormationSlots::SlotOffset(ESolidBattleFormation::Column, 0, Count);
	const FVector2D Col1 = SolidBattleFormationSlots::SlotOffset(ESolidBattleFormation::Column, 1, Count);
	TestTrue(TEXT("column: alex further back than sam"), Col1.X < Col0.X);
	TestEqual(TEXT("column: centered"), Col0.Y, 0.f);
	TestEqual(TEXT("column: alex centered"), Col1.Y, 0.f);

	const FVector2D Mob0 = SolidBattleFormationSlots::SlotOffset(ESolidBattleFormation::Mob, 0, Count);
	const FVector2D Mob1 = SolidBattleFormationSlots::SlotOffset(ESolidBattleFormation::Mob, 1, Count);
	TestTrue(TEXT("mob: both behind"), Mob0.X < 0.f && Mob1.X < 0.f);
	TestEqual(TEXT("mob: same depth (triangle base)"), Mob0.X, Mob1.X);
	TestTrue(TEXT("mob: opposite flanks"), Mob0.Y * Mob1.Y < 0.f);
	TestTrue(TEXT("mob: tighter than line"), FMath::Abs(Mob0.Y) < FMath::Abs(Line0.Y));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanyDefaultPlansTest,
	"SolidCore1.BattlePlan.CompanyDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanyDefaultPlansTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	TestNotNull(TEXT("company"), Company);
	Company->InitializeDefaultBattlePlans();
	TestEqual(TEXT("three starter plans"), Company->GetBattlePlanCount(), 3);
	Company->InitializeDefaultBattlePlans();
	TestEqual(TEXT("idempotent init"), Company->GetBattlePlanCount(), 3);

	const FSolidBattlePlan* Line = Company->GetBattlePlan(0);
	const FSolidBattlePlan* Column = Company->GetBattlePlan(1);
	const FSolidBattlePlan* Mob = Company->GetBattlePlan(2);
	TestNotNull(TEXT("line plan"), Line);
	TestNotNull(TEXT("column plan"), Column);
	TestNotNull(TEXT("mob plan"), Mob);
	TestEqual(TEXT("line name"), Line->Name, FString(TEXT("Line")));
	TestEqual(TEXT("column name"), Column->Name, FString(TEXT("Column")));
	TestEqual(TEXT("mob name"), Mob->Name, FString(TEXT("Mob")));
	TestTrue(TEXT("line formation"), Line->Formation == ESolidBattleFormation::Line);
	TestTrue(TEXT("column formation"), Column->Formation == ESolidBattleFormation::Column);
	TestTrue(TEXT("mob formation"), Mob->Formation == ESolidBattleFormation::Mob);
	TestEqual(TEXT("find column"), Company->FindBattlePlanIndexByName(TEXT("Column")), 1);
	TestNull(TEXT("out of range"), Company->GetBattlePlan(99));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPartyAssignedPlansTest,
	"SolidCore1.BattlePlan.PartyAssigned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPartyAssignedPlansTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	USolidParty* Party = NewObject<USolidParty>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("party"), Party);

	Party->InitializeFromCompany(Company);
	TestEqual(TEXT("assigned starter count"), Party->GetAssignedCount(), 3);
	TestTrue(TEXT("active is column by default"),
		Party->GetActiveFormation() == ESolidBattleFormation::Column);
	TestEqual(TEXT("active slot is column index"), Party->GetActiveAssignedSlot(), 1);

	TestTrue(TEXT("select F1 line"), Party->SelectAssignedSlot(0));
	TestTrue(TEXT("active line"), Party->GetActiveFormation() == ESolidBattleFormation::Line);
	TestTrue(TEXT("select F3 mob"), Party->SelectAssignedSlot(2));
	TestTrue(TEXT("active mob"), Party->GetActiveFormation() == ESolidBattleFormation::Mob);
	TestFalse(TEXT("reject empty F4"), Party->SelectAssignedSlot(3));
	TestFalse(TEXT("reject F8 when only 3"), Party->SelectAssignedSlot(7));
	TestEqual(TEXT("slot unchanged after reject"), Party->GetActiveAssignedSlot(), 2);

	// Max 8 assigned — grow the Company catalog first so indices are unique.
	TArray<int32> TooMany;
	for (int32 i = 0; i < 12; ++i)
	{
		FSolidBattlePlan Extra;
		Extra.Name = FString::Printf(TEXT("Extra%d"), i);
		Extra.Formation = ESolidBattleFormation::Mob;
		TooMany.Add(Company->AddBattlePlan(Extra));
	}
	TestTrue(TEXT("set assigned"), Party->SetAssignedBattlePlans(TooMany));
	TestEqual(TEXT("capped at 8"), Party->GetAssignedCount(), SolidBattleFormationSlots::MaxAssignedBattlePlans);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGameModeBattlePlanSelectTest,
	"SolidCore1.BattlePlan.GameModeSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGameModeBattlePlanSelectTest::RunTest(const FString& Parameters)
{
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);
	TestNotNull(TEXT("company"), GameMode->GetCompany());
	TestNotNull(TEXT("party"), GameMode->GetParty());
	TestEqual(TEXT("company plans"), GameMode->GetCompany()->GetBattlePlanCount(), 3);
	TestTrue(TEXT("default column"),
		GameMode->GetParty()->GetActiveFormation() == ESolidBattleFormation::Column);

	TestTrue(TEXT("F1 line"), GameMode->SelectBattlePlanSlot(0));
	TestTrue(TEXT("formation line"),
		GameMode->GetParty()->GetActiveFormation() == ESolidBattleFormation::Line);
	TestTrue(TEXT("F2 column"), GameMode->SelectBattlePlanSlot(1));
	TestTrue(TEXT("F3 mob"), GameMode->SelectBattlePlanSlot(2));
	TestFalse(TEXT("F4 empty"), GameMode->SelectBattlePlanSlot(3));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
