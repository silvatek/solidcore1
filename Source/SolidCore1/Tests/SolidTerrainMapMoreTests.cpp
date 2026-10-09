#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMapSampleHeightMatchesGridTest,
	"SolidCore1.Map.SampleHeightMatchesGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMapSampleHeightMatchesGridTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map"), Map);

	const FSolidTerrainPoint& Grid = Map->GetPoint(32, 32);
	const float Sampled = Map->SampleHeight(Grid.X, Grid.Y);
	TestTrue(TEXT("sample at grid node matches point height"),
		FMath::IsNearlyEqual(Sampled, Grid.Height, 0.01f));

	const FSolidTerrainPoint& Nearest = Map->GetNearestPoint(Grid.X + 10.f, Grid.Y - 10.f);
	TestEqual(TEXT("nearest snaps nearby query"), Nearest.X, Grid.X);
	TestEqual(TEXT("nearest snaps nearby query Y"), Nearest.Y, Grid.Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMapBuildIdempotentTest,
	"SolidCore1.Map.BuildIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMapBuildIdempotentTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	const int32 CountBefore = Map->GetPointCount();
	const float HeightBefore = Map->GetPoint(10, 10).Height;

	Map->Build(9999, 0.00012f, 3000.f, 0.f, 65, 65, 200.f, /*bForceRebuild=*/false);
	TestEqual(TEXT("point count unchanged without force"), Map->GetPointCount(), CountBefore);
	TestTrue(TEXT("heights unchanged without force"),
		FMath::IsNearlyEqual(Map->GetPoint(10, 10).Height, HeightBefore));

	Map->Build(9999, 0.00012f, 3000.f, 0.f, 65, 65, 200.f, /*bForceRebuild=*/true);
	TestEqual(TEXT("forced rebuild keeps size"), Map->GetPointCount(), 65 * 65);
	TestEqual(TEXT("forced rebuild updates seed"), Map->Seed, 9999);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMapWorldBoundsTest,
	"SolidCore1.Map.WorldBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMapWorldBoundsTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	const FVector2D MinXY = Map->GetWorldMinXY();
	const FVector2D MaxXY = Map->GetWorldMaxXY();
	TestTrue(TEXT("min < max X"), MinXY.X < MaxXY.X);
	TestTrue(TEXT("min < max Y"), MinXY.Y < MaxXY.Y);
	TestTrue(TEXT("origin centered-ish (min negative)"), MinXY.X < 0.f && MinXY.Y < 0.f);
	TestTrue(TEXT("origin centered-ish (max positive)"), MaxXY.X > 0.f && MaxXY.Y > 0.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
