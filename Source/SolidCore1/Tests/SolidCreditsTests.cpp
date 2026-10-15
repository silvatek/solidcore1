#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCredits.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCreditsCopyTest,
	"SolidCore1.Credits.Copy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCreditsCopyTest::RunTest(const FString& Parameters)
{
	TArray<FString> Lines;
	SolidCredits::CollectLines(Lines);
	TestTrue(TEXT("has some lines"), Lines.Num() >= 6);

	FString Joined;
	for (const FString& Line : Lines)
	{
		Joined += Line;
		Joined += TEXT("\n");
	}

	TestTrue(TEXT("names Silvatek"), Joined.Contains(TEXT("Silvatek")));
	TestTrue(TEXT("names Cursor"), Joined.Contains(TEXT("Cursor")));
	TestTrue(TEXT("names Grok"), Joined.Contains(TEXT("Grok")));
	TestTrue(TEXT("credits Viking creator"), Joined.Contains(SolidCredits::VikingCreator));
	TestTrue(TEXT("credits grass creator"), Joined.Contains(SolidCredits::GrassCreator));
	TestTrue(TEXT("notes grass CC-BY"), Joined.Contains(TEXT("CC-BY")));
	TestTrue(TEXT("credits assassin creator"), Joined.Contains(SolidCredits::AssassinCreator));
	TestTrue(TEXT("credits assassin listing"), Joined.Contains(SolidCredits::AssassinListing));
	TestTrue(TEXT("credits blacksmith creator"), Joined.Contains(SolidCredits::BlacksmithCreator));
	TestTrue(TEXT("credits blacksmith listing"), Joined.Contains(SolidCredits::BlacksmithListing));
	TestTrue(TEXT("mentions F10"), Joined.Contains(TEXT("F10")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
