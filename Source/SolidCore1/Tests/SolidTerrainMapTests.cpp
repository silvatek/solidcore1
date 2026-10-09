#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMapBuildSmokeTest,
	"SolidCore1.Map.BuildSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMapBuildSmokeTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map object"), Map);
	TestTrue(TEXT("map reports built"), Map->IsBuilt());
	TestEqual(TEXT("point count"), Map->GetPointCount(), 65 * 65);

	const FVector2D FogOrigin = Map->GetFogOriginXY();
	const FSolidTerrainPoint Origin = Map->SamplePoint(FogOrigin.X, FogOrigin.Y);
	TestTrue(TEXT("fog origin starts clear"), FMath::IsNearlyEqual(Origin.Fog, 0.f));

	const FSolidTerrainPoint Far = Map->SamplePoint(FogOrigin.X + 6000.f, FogOrigin.Y); // 60m
	TestTrue(TEXT("60m starts fully fogged"), FMath::IsNearlyEqual(Far.Fog, 1.f));

	const FSolidTerrainPoint Mid = Map->SamplePoint(FogOrigin.X + 3500.f, FogOrigin.Y); // 35m
	TestTrue(TEXT("35m starts at half fog"), FMath::IsNearlyEqual(Mid.Fog, 0.5f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMapTrailClearTest,
	"SolidCore1.Map.TrailClearsFog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMapTrailClearTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map object"), Map);

	// ~70.7m from origin → starts at full fog; stays inside ±6400cm map bounds for probes.
	constexpr float TrailX = 5000.f;
	constexpr float TrailY = 5000.f;
	TestTrue(TEXT("precondition: trail center fogged"),
		FMath::IsNearlyEqual(Map->SamplePoint(TrailX, TrailY).Fog, 1.f));

	const int32 Changed = Map->ApplyExplorationFogAround(TrailX, TrailY);
	TestTrue(TEXT("trail clear changed some points"), Changed > 0);

	TestTrue(TEXT("trail center cleared"),
		FMath::IsNearlyEqual(Map->SamplePoint(TrailX, TrailY).Fog, 0.f));

	// 15m toward origin — still inside clear radius (25m), was full fog before.
	TestTrue(TEXT("15m from trail is clear"),
		FMath::IsNearlyEqual(Map->SamplePoint(TrailX - 1500.f, TrailY).Fog, 0.f));

	// 35m toward origin — half ring; was full fog before trail.
	const float HalfFog = Map->SamplePoint(TrailX - 3500.f, TrailY).Fog;
	TestTrue(TEXT("35m from trail is half fog"), FMath::IsNearlyEqual(HalfFog, 0.5f));

	// Far opposite corner — outside apply radius, still full fog.
	const float Untouched = Map->SamplePoint(-5000.f, -5000.f).Fog;
	TestTrue(TEXT("opposite far side still full fog"), FMath::IsNearlyEqual(Untouched, 1.f));

	const int32 Second = Map->ApplyExplorationFogAround(TrailX, TrailY);
	TestEqual(TEXT("second apply is idempotent"), Second, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMapTrailNeverIncreasesFogTest,
	"SolidCore1.Map.TrailNeverIncreasesFog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMapTrailNeverIncreasesFogTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map object"), Map);

	TArray<float> Before;
	Before.Reserve(Map->GetPointCount());
	for (const FSolidTerrainPoint& P : Map->GetPoints())
	{
		Before.Add(P.Fog);
	}

	Map->ApplyExplorationFogAround(0.f, 0.f);
	Map->ApplyExplorationFogAround(4000.f, 2000.f);

	const TArray<FSolidTerrainPoint>& After = Map->GetPoints();
	TestEqual(TEXT("point count stable"), After.Num(), Before.Num());
	for (int32 Index = 0; Index < After.Num(); ++Index)
	{
		if (After[Index].Fog > Before[Index] + KINDA_SMALL_NUMBER)
		{
			AddError(FString::Printf(
				TEXT("Fog increased at index %d: %.3f -> %.3f"),
				Index, Before[Index], After[Index].Fog));
			return false;
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
