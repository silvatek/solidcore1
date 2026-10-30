#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidHudDebugToggleTest,
	"SolidCore1.HUD.DebugToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidHudDebugToggleTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}

	HUD->ToggleDebugPanel();
	HUD->ToggleDebugPanel();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
