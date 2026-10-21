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
	TestTrue(TEXT("links the CC-BY deed"), Joined.Contains(SolidCredits::CcByLicense));
	TestTrue(TEXT("notes the grass adaptation"), Joined.Contains(SolidCredits::GrassChanges));
	TestTrue(TEXT("credits the Viking listing"), Joined.Contains(SolidCredits::VikingListing));
	TestTrue(TEXT("credits the grass listing"), Joined.Contains(SolidCredits::GrassListing));
	TestTrue(TEXT("mentions F10"), Joined.Contains(TEXT("F10")));
	TestTrue(TEXT("Unreal engine notice"), Joined.Contains(SolidCredits::EngineNotice));
	TestTrue(TEXT("Unreal trademark notice"), Joined.Contains(SolidCredits::EngineTrademark));
	TestTrue(TEXT("Unreal trademark place"), Joined.Contains(SolidCredits::EngineTrademarkPlace));
	TestTrue(TEXT("Unreal copyright notice"), Joined.Contains(SolidCredits::EngineCopyright));
	TestTrue(TEXT("Unreal warranty disclaimer"), Joined.Contains(SolidCredits::EngineDisclaimer));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
