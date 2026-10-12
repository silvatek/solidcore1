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
	TestEqual(TEXT("seven towns"), SolidTowns::DefinitionCount, 7);

	const int32 ExpectedIds[] = { 0, 1, 2, 3, 4, 5, 6 };
	const TCHAR* Expected[] = {
		TEXT("Iglin"), TEXT("Relion"), TEXT("Kanfold"), TEXT("Visolar"), TEXT("Jethan"),
		TEXT("Dragongate"), TEXT("Rathen") };
	int32 WallGateCount = 0;
	for (int32 Index = 0; Index < SolidTowns::DefinitionCount; ++Index)
	{
		const SolidTowns::FTownInfo& Town = SolidTowns::Definitions[Index];
		TestEqual(TEXT("id"), Town.Id, ExpectedIds[Index]);
		TestEqual(TEXT("name"), SolidTowns::NameFor(Town.Id), FString(Expected[Index]));
		TestNotNull(TEXT("find"), SolidTowns::Find(Town.Id));
		if (Town.Type == SolidTowns::ETownType::WallGate)
		{
			++WallGateCount;
			TestEqual(TEXT("the wall gate is Dragongate"), FString(Town.Name), FString(TEXT("Dragongate")));
			TestTrue(TEXT("wall gate query"), SolidTowns::IsWallGate(Town.Id));
		}
		else
		{
			TestEqual(
				TEXT("ordinary town"),
				static_cast<uint8>(Town.Type),
				static_cast<uint8>(SolidTowns::ETownType::Town));
			TestFalse(TEXT("ordinary town is not a wall gate"), SolidTowns::IsWallGate(Town.Id));
		}
	}
	TestEqual(TEXT("one wall gate"), WallGateCount, 1);

	TestTrue(TEXT("index 7 has no name"), SolidTowns::NameFor(7).IsEmpty());
	TestNull(TEXT("index 7 is not a town"), SolidTowns::Find(7));
	TestTrue(TEXT("unknown index has no name"), SolidTowns::NameFor(9).IsEmpty());
	TestNull(TEXT("unknown index is not a town"), SolidTowns::Find(9));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
