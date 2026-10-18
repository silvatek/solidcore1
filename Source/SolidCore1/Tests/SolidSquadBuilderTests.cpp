#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Menus/SolidSquadBuilder.h"
#include "Squad/SolidCompany.h"
#include "Squad/SolidSquad.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadBuilderToggleTest,
	"SolidCore1.SquadBuilder.Toggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadBuilderToggleTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("locked at the start"), SolidSquadBuilder::CanOpen(0));
	TestFalse(TEXT("locked with one companion allowed"), SolidSquadBuilder::CanOpen(1));
	TestTrue(TEXT("opens once two are allowed"), SolidSquadBuilder::CanOpen(2));
	TestTrue(TEXT("a selected row can be cleared at the cap"), SolidSquadBuilder::IsChoiceAvailable(true, 2, 2));
	TestFalse(TEXT("a new row is refused at the cap"), SolidSquadBuilder::IsChoiceAvailable(false, 2, 2));

	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);
	Squad->InitializeFromCompany(Company);

	TArray<SolidSquadBuilder::FRow> Rows;
	SolidSquadBuilder::CollectRows(Squad, Rows);
	TestEqual(TEXT("lists the company"), Rows.Num(), 5);
	TestTrue(TEXT("Sam starts selected"), Rows[0].bSelected);
	TestTrue(TEXT("Alex starts selected"), Rows[1].bSelected);
	TestFalse(TEXT("Max starts out"), Rows[2].bSelected);
	TestTrue(TEXT("Sam is a rogue"), Rows[0].Label.Contains(TEXT("[x]  Sam (R)")));
	TestTrue(TEXT("Morgan is a mage"), Rows[3].Label.Contains(TEXT("[ ]  Morgan (M)")));

	const int32 Sam = Rows[0].CompanyIndex;
	const int32 Max = Rows[2].CompanyIndex;
	const int32 Morgan = Rows[3].CompanyIndex;
	TestTrue(TEXT("clear Sam"), SolidSquadBuilder::Toggle(Squad, Sam, 2));
	TestEqual(TEXT("one remains"), Squad->GetSelectedCompanionCount(), 1);
	TestTrue(TEXT("add Max"), SolidSquadBuilder::Toggle(Squad, Max, 2));
	TestEqual(TEXT("back to two"), Squad->GetSelectedCompanionCount(), 2);
	TestFalse(TEXT("Morgan is over the cap"), SolidSquadBuilder::Toggle(Squad, Morgan, 2));
	TestEqual(TEXT("still two"), Squad->GetSelectedCompanionCount(), 2);
	TestFalse(TEXT("unknown companion"), SolidSquadBuilder::Toggle(Squad, 99, 2));
	TestFalse(TEXT("no squad"), SolidSquadBuilder::Toggle(nullptr, Sam, 2));

	TArray<FSolidCompanion> Fielded;
	Squad->CollectFieldedCompanions(2, Fielded);
	TestEqual(TEXT("fielded follows the new choice"), Fielded.Num(), 2);
	if (Fielded.Num() == 2)
	{
		TestEqual(TEXT("Alex stays first"), Fielded[0].Name, FString(TEXT("Alex")));
		TestEqual(TEXT("Max joined second"), Fielded[1].Name, FString(TEXT("Max")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
