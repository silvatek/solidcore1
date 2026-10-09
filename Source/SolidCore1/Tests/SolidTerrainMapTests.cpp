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

	// Probe toward map interior so 60m/35m samples are not edge-clamped.
	const FVector2D Dir = SolidTerrainTestHelpers::InBoundsFogProbeDir(Map, 6000.f);
	const FVector2D FarXY = FogOrigin + Dir * 6000.f;
	const FVector2D MidXY = FogOrigin + Dir * 3500.f;

	const FSolidTerrainPoint Far = Map->SamplePoint(FarXY.X, FarXY.Y);
	TestTrue(TEXT("60m starts fully fogged"), FMath::IsNearlyEqual(Far.Fog, 1.f));

	const FSolidTerrainPoint Mid = Map->SamplePoint(MidXY.X, MidXY.Y);
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

	FVector2D TrailXY = FVector2D::ZeroVector;
	TestTrue(TEXT("found fully-fogged in-bounds trail point"),
		SolidTerrainTestHelpers::FindFullyFoggedTrailPoint(Map, TrailXY));

	FVector2D UntouchedXY = FVector2D::ZeroVector;
	TestTrue(TEXT("found second full-fog point outside trail apply radius"),
		SolidTerrainTestHelpers::FindUntouchedFullFogPoint(Map, TrailXY, UntouchedXY));

	const FVector2D FogOrigin = Map->GetFogOriginXY();
	FVector2D TowardOrigin = FogOrigin - TrailXY;
	if (!TowardOrigin.Normalize())
	{
		TowardOrigin = FVector2D(-1.f, 0.f);
	}

	const float UntouchedFogBefore = Map->SamplePoint(UntouchedXY.X, UntouchedXY.Y).Fog;
	TestTrue(TEXT("precondition: trail center fogged"),
		FMath::IsNearlyEqual(Map->SamplePoint(TrailXY.X, TrailXY.Y).Fog, 1.f));
	TestTrue(TEXT("precondition: untouched point fogged"),
		FMath::IsNearlyEqual(UntouchedFogBefore, 1.f));
	TestTrue(TEXT("precondition: untouched is outside 50m apply radius"),
		FVector2D::Distance(TrailXY, UntouchedXY)
			> SolidTerrainFog::FullFogStartMeters * 100.f);

	const int32 Changed = Map->ApplyExplorationFogAround(TrailXY.X, TrailXY.Y);
	TestTrue(TEXT("trail clear changed some points"), Changed > 0);

	TestTrue(TEXT("trail center cleared"),
		FMath::IsNearlyEqual(Map->SamplePoint(TrailXY.X, TrailXY.Y).Fog, 0.f));

	// 15m toward fog origin — still inside clear radius (25m).
	const FVector2D ClearXY = TrailXY + TowardOrigin * 1500.f;
	TestTrue(TEXT("15m from trail is clear"),
		FMath::IsNearlyEqual(Map->SamplePoint(ClearXY.X, ClearXY.Y).Fog, 0.f));

	// 35m toward fog origin — half ring (nearest grid fog, not bilinear blend).
	const FVector2D HalfXY = TrailXY + TowardOrigin * 3500.f;
	const float HalfFog = Map->GetNearestPoint(HalfXY.X, HalfXY.Y).Fog;
	TestTrue(TEXT("35m from trail is half fog"), FMath::IsNearlyEqual(HalfFog, 0.5f));

	const float UntouchedFogAfter = Map->SamplePoint(UntouchedXY.X, UntouchedXY.Y).Fog;
	TestTrue(TEXT("untouched fog unchanged"),
		FMath::IsNearlyEqual(UntouchedFogAfter, UntouchedFogBefore));
	TestTrue(TEXT("untouched still full fog"), FMath::IsNearlyEqual(UntouchedFogAfter, 1.f));

	const int32 Second = Map->ApplyExplorationFogAround(TrailXY.X, TrailXY.Y);
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
