#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBuildId.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBuildIdPresentTest,
	"SolidCore1.Build.IdPresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBuildIdPresentTest::RunTest(const FString& Parameters)
{
	const FString Id(SOLID_BUILD_ID);
	const FString Note(SOLID_BUILD_NOTE);
	TestTrue(TEXT("build id non-empty"), !Id.IsEmpty());
	TestTrue(TEXT("build id starts with SC1-"), Id.StartsWith(TEXT("SC1-")));
	TestTrue(TEXT("build note non-empty"), !Note.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
