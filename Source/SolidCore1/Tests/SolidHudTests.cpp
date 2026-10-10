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

	TestTrue(TEXT("debug readout starts visible"), HUD->IsDebugPanelVisible());
	HUD->ToggleDebugPanel();
	TestFalse(TEXT("F12 hides the debug readout"), HUD->IsDebugPanelVisible());
	HUD->ToggleDebugPanel();
	TestTrue(TEXT("F12 shows the debug readout again"), HUD->IsDebugPanelVisible());
	HUD->SetDebugPanelVisible(false);
	TestFalse(TEXT("set hidden"), HUD->IsDebugPanelVisible());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
