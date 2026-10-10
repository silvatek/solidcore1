#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTowns.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownNamesTest,
	"SolidCore1.Towns.Names",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownNamesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("four towns"), SolidTowns::DefinitionCount, 4);

	const TCHAR* Expected[] = { TEXT("Iglin"), TEXT("Relion"), TEXT("Kanfold"), TEXT("Visolar") };
	for (int32 Index = 0; Index < SolidTowns::DefinitionCount; ++Index)
	{
		const SolidTowns::FDefinition& Town = SolidTowns::Definitions[Index];
		TestEqual(TEXT("id matches the table order"), Town.Id, Index);
		TestEqual(TEXT("name"), SolidTowns::NameFor(Town.Id), FString(Expected[Index]));
		TestNotNull(TEXT("find"), SolidTowns::Find(Town.Id));
	}

	TestTrue(TEXT("unknown index has no name"), SolidTowns::NameFor(9).IsEmpty());
	TestNull(TEXT("unknown index is not a town"), SolidTowns::Find(9));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
