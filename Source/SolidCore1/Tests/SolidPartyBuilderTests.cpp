#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Menus/SolidPartyBuilder.h"
#include "Party/SolidCompany.h"
#include "Party/SolidParty.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPartyBuilderToggleTest,
	"SolidCore1.PartyBuilder.Toggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPartyBuilderToggleTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("locked at the start"), SolidPartyBuilder::CanOpen(0));
	TestFalse(TEXT("locked with one companion allowed"), SolidPartyBuilder::CanOpen(1));
	TestTrue(TEXT("opens once two are allowed"), SolidPartyBuilder::CanOpen(2));
	TestTrue(TEXT("a selected row can be cleared at the cap"), SolidPartyBuilder::IsChoiceAvailable(true, 2, 2));
	TestFalse(TEXT("a new row is refused at the cap"), SolidPartyBuilder::IsChoiceAvailable(false, 2, 2));

	USolidCompany* Company = NewObject<USolidCompany>();
	USolidParty* Party = NewObject<USolidParty>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("party"), Party);
	Party->InitializeFromCompany(Company);

	TArray<SolidPartyBuilder::FRow> Rows;
	SolidPartyBuilder::CollectRows(Party, Rows);
	TestEqual(TEXT("lists the company"), Rows.Num(), 5);
	TestTrue(TEXT("Sam starts selected"), Rows[0].bSelected);
	TestTrue(TEXT("Alex starts selected"), Rows[1].bSelected);
	TestFalse(TEXT("Max starts out"), Rows[2].bSelected);
	TestTrue(TEXT("Sam is a rogue"), Rows[0].Label.Contains(TEXT("[x]  Sam (R)")));
	TestTrue(TEXT("Morgan is a mage"), Rows[3].Label.Contains(TEXT("[ ]  Morgan (M)")));

	const int32 Sam = Rows[0].CompanyIndex;
	const int32 Max = Rows[2].CompanyIndex;
	const int32 Morgan = Rows[3].CompanyIndex;
	TestTrue(TEXT("clear Sam"), SolidPartyBuilder::Toggle(Party, Sam, 2));
	TestEqual(TEXT("one remains"), Party->GetSelectedCompanionCount(), 1);
	TestTrue(TEXT("add Max"), SolidPartyBuilder::Toggle(Party, Max, 2));
	TestEqual(TEXT("back to two"), Party->GetSelectedCompanionCount(), 2);
	TestFalse(TEXT("Morgan is over the cap"), SolidPartyBuilder::Toggle(Party, Morgan, 2));
	TestEqual(TEXT("still two"), Party->GetSelectedCompanionCount(), 2);
	TestFalse(TEXT("unknown companion"), SolidPartyBuilder::Toggle(Party, 99, 2));
	TestFalse(TEXT("no party"), SolidPartyBuilder::Toggle(nullptr, Sam, 2));

	TArray<FSolidCompanion> Fielded;
	Party->CollectFieldedCompanions(2, Fielded);
	TestEqual(TEXT("fielded follows the new choice"), Fielded.Num(), 2);
	if (Fielded.Num() == 2)
	{
		TestEqual(TEXT("Alex stays first"), Fielded[0].Name, FString(TEXT("Alex")));
		TestEqual(TEXT("Max joined second"), Fielded[1].Name, FString(TEXT("Max")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
