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
	TestEqual(TEXT("six towns"), SolidTowns::DefinitionCount, 6);

	const int32 ExpectedIds[] = { 0, 1, 2, 3, 4, 6 };
	const TCHAR* Expected[] = {
		TEXT("Iglin"), TEXT("Relion"), TEXT("Kanfold"), TEXT("Visolar"), TEXT("Jethan"), TEXT("Rathen") };
	for (int32 Index = 0; Index < SolidTowns::DefinitionCount; ++Index)
	{
		const SolidTowns::FTownInfo& Town = SolidTowns::Definitions[Index];
		TestEqual(TEXT("id"), Town.Id, ExpectedIds[Index]);
		TestEqual(TEXT("name"), SolidTowns::NameFor(Town.Id), FString(Expected[Index]));
		TestNotNull(TEXT("find"), SolidTowns::Find(Town.Id));
	}

	TestTrue(TEXT("index 5 is unused"), SolidTowns::NameFor(5).IsEmpty());
	TestNull(TEXT("index 5 is not a town"), SolidTowns::Find(5));
	TestTrue(TEXT("unknown index has no name"), SolidTowns::NameFor(9).IsEmpty());
	TestNull(TEXT("unknown index is not a town"), SolidTowns::Find(9));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
