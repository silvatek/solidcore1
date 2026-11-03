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
	TestTrue(TEXT("credits the Velociraptor creator"), Joined.Contains(SolidCredits::VelociraptorCreator));
	TestTrue(TEXT("credits the Velociraptor license"), Joined.Contains(SolidCredits::VelociraptorLicense));
	TestTrue(TEXT("credits the Velociraptor listing"), Joined.Contains(SolidCredits::VelociraptorListing));
	TestTrue(TEXT("credits the Stylized Environment creator"), Joined.Contains(SolidCredits::StylizedEnvironmentCreator));
	TestTrue(TEXT("credits the Stylized Environment license"), Joined.Contains(SolidCredits::StylizedEnvironmentLicense));
	TestTrue(TEXT("credits the Stylized Environment listing"), Joined.Contains(SolidCredits::StylizedEnvironmentListing));
	const int32 GrassAt = Joined.Find(SolidCredits::GrassListing);
	const int32 RaptorAt = Joined.Find(SolidCredits::VelociraptorListing);
	const int32 EnvironmentCreatorAt = Joined.Find(SolidCredits::StylizedEnvironmentCreator);
	const int32 EnvironmentDeedAt = Joined.Find(
		SolidCredits::CcByLicense, ESearchCase::CaseSensitive, ESearchDir::FromStart, EnvironmentCreatorAt);
	const int32 EnvironmentAt = Joined.Find(SolidCredits::StylizedEnvironmentListing);
	TestTrue(TEXT("the Velociraptor block follows the grass listing"), GrassAt != INDEX_NONE && RaptorAt > GrassAt);
	TestTrue(TEXT("the Stylized Environment block follows the Velociraptor listing"), RaptorAt != INDEX_NONE && EnvironmentAt > RaptorAt);
	TestTrue(
		TEXT("the Stylized Environment block links the CC-BY deed"),
		EnvironmentCreatorAt != INDEX_NONE && EnvironmentDeedAt > EnvironmentCreatorAt && EnvironmentAt > EnvironmentDeedAt);
	TestTrue(TEXT("mentions F10"), Joined.Contains(TEXT("F10")));
	TestTrue(TEXT("Unreal engine notice"), Joined.Contains(SolidCredits::EngineNotice));
	TestTrue(TEXT("Unreal trademark notice"), Joined.Contains(SolidCredits::EngineTrademark));
	TestTrue(TEXT("Unreal trademark place"), Joined.Contains(SolidCredits::EngineTrademarkPlace));
	TestTrue(TEXT("Unreal copyright notice"), Joined.Contains(SolidCredits::EngineCopyright));
	TestTrue(TEXT("Unreal warranty disclaimer"), Joined.Contains(SolidCredits::EngineDisclaimer));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
