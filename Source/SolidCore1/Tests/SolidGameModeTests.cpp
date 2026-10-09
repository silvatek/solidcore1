#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCharacter.h"
#include "SolidContentPaths.h"
#include "SolidGameMode.h"
#include "GameFramework/Pawn.h"

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
	TestEqual(TEXT("default companion count Sam+Alex"), ASolidGameMode::DefaultCompanionCount, 2);
	TestNull(TEXT("no companion until Ensure"), GameMode->GetCompanion());
	TestEqual(TEXT("companions empty until Ensure"), GameMode->GetCompanions().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGameModePawnResolutionTest,
	"SolidCore1.GameMode.PawnResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGameModePawnResolutionTest::RunTest(const FString& Parameters)
{
	UClass* PreferredBp = SolidContentPaths::LoadFirstClass<APawn>(SolidContentPaths::PawnBlueprintClasses());
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);
	TestNotNull(TEXT("DefaultPawnClass set"), GameMode->DefaultPawnClass.Get());

	if (PreferredBp)
	{
		TestEqual(TEXT("DefaultPawnClass is preferred BP_SolidCharacter"),
			GameMode->DefaultPawnClass.Get(), PreferredBp);
	}
	else
	{
		TestEqual(TEXT("DefaultPawnClass falls back to C++ SolidCharacter"),
			GameMode->DefaultPawnClass.Get(), ASolidCharacter::StaticClass());
	}

	TestTrue(TEXT("pawn is SolidCharacter or subclass"),
		GameMode->DefaultPawnClass->IsChildOf(ASolidCharacter::StaticClass()));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
