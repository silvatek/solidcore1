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
	TestTrue(TEXT("auto vegetation"), GameMode->bAutoSpawnVegetation);
	TestTrue(TEXT("forest tree density in range"),
		GameMode->ForestTreeDensity > 0.f && GameMode->ForestTreeDensity <= 1.f);
	TestTrue(TEXT("max forest trees positive"), GameMode->MaxForestTrees > 0);
	TestEqual(TEXT("default companion count Sam+Alex"), ASolidGameMode::DefaultCompanionCount, 2);
	TestNull(TEXT("no companion until Ensure"), GameMode->GetCompanion());
	TestEqual(TEXT("companions empty until Ensure"), GameMode->GetCompanions().Num(), 0);
	TestNotNull(TEXT("company created"), GameMode->GetCompany());
	TestNotNull(TEXT("party created"), GameMode->GetParty());
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
