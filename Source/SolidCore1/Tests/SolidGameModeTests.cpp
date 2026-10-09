#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGameModeDefaultsTest,
	"SolidCore1.GameMode.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGameModeDefaultsTest::RunTest(const FString& Parameters)
{
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);
	TestTrue(TEXT("auto terrain streamer"), GameMode->bAutoSpawnTerrainStreamer);
	TestTrue(TEXT("auto companion"), GameMode->bAutoSpawnCompanion);
	TestTrue(TEXT("auto starter trees"), GameMode->bAutoSpawnStarterTrees);
	TestEqual(TEXT("starter tree count"), GameMode->StarterTreeCount, 16);
	TestTrue(TEXT("tree spacing positive"), GameMode->StarterTreeSpacingCm > 0.f);
	TestNull(TEXT("no companion until Ensure"), GameMode->GetCompanion());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
