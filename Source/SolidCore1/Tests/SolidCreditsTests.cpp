#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCredits.h"
#include "SolidHUD.h"

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
	TestTrue(TEXT("mentions F10"), Joined.Contains(TEXT("F10")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCreditsHudToggleTest,
	"SolidCore1.Credits.HudToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCreditsHudToggleTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	TestFalse(TEXT("credits hidden by default"), HUD->IsCreditsVisible());
	HUD->ToggleCredits();
	TestTrue(TEXT("F10 shows credits"), HUD->IsCreditsVisible());
	HUD->ToggleCredits();
	TestFalse(TEXT("F10 hides credits"), HUD->IsCreditsVisible());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
