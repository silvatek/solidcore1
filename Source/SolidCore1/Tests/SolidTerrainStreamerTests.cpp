#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStreamerNullWorldTest,
	"SolidCore1.Streamer.NullWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStreamerNullWorldTest::RunTest(const FString& Parameters)
{
	TestNull(TEXT("FindExisting null world"), ASolidTerrainStreamer::FindExisting(nullptr));
	TestNull(TEXT("EnsureExists null world"), ASolidTerrainStreamer::EnsureExists(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStreamerEnsureExistsIdempotentTest,
	"SolidCore1.Streamer.EnsureExistsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStreamerEnsureExistsIdempotentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("SolidStreamerTestWorld"));
	TestNotNull(TEXT("created world"), World);
	if (!World)
	{
		return false;
	}

	TestNull(TEXT("no streamer before EnsureExists"), ASolidTerrainStreamer::FindExisting(World));

	ASolidTerrainStreamer* First = ASolidTerrainStreamer::EnsureExists(World);
	TestNotNull(TEXT("EnsureExists spawns"), First);
	TestEqual(TEXT("FindExisting returns spawned"), ASolidTerrainStreamer::FindExisting(World), First);

	ASolidTerrainStreamer* Second = ASolidTerrainStreamer::EnsureExists(World);
	TestEqual(TEXT("second EnsureExists is idempotent"), Second, First);

	World->DestroyWorld(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
