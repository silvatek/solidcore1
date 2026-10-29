#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCharacter.h"
#include "SolidSight.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainTestHelpers.h"
#include "Terrain/SolidTerrainStreamer.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCameraFogClearScaleTest,
	"SolidCore1.Fog.CameraClearScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCameraFogClearScaleTest::RunTest(const FString& Parameters)
{
	const auto Clear = [](float) { return 0.f; };
	TestEqual(TEXT("clear camera stays"), SolidSight::MaxClearScale(Clear), 1.f);

	const auto AnchorFogged = [](float Scale) { return Scale < 0.5f ? 1.f : 0.f; };
	TestEqual(TEXT("fogged anchor is not pulled"), SolidSight::MaxClearScale(AnchorFogged), 1.f);

	// Clear on [0, 0.25], fog beyond. The boom must stop at or before the boundary.
	const auto Boundary = [](float Scale) { return Scale > 0.25f ? 1.f : 0.f; };
	const float Scale = SolidSight::MaxClearScale(Boundary);
	TestTrue(TEXT("stops before fog"), Scale <= 0.25f);
	TestTrue(TEXT("reaches the clear side"), Scale > 0.24f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCameraGroundNeighbourTest,
	"SolidCore1.Fog.CameraGroundNeighbours",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCameraGroundNeighbourTest::RunTest(const FString& Parameters)
{
	const USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map"), Map);
	if (!Map)
	{
		return false;
	}

	const FVector2D Origin = Map->GetFogOriginXY();
	TestTrue(TEXT("the captain's point is clear"),
		FMath::IsNearlyEqual(SolidSight::CameraGroundFog(*Map, Origin.X, Origin.Y), 0.f));

	const float Edge = SolidTerrainFog::MetersToCm(SolidTerrainFog::ClearRadiusMeters) - Map->PointSpacing;
	const FVector2D Rim(Origin.X + Edge, Origin.Y);
	TestTrue(TEXT("the rim point itself is clear"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(Rim.X, Rim.Y).Fog, 0.f));
	TestTrue(TEXT("a fogged neighbour blocks the camera"),
		SolidSight::CameraGroundFog(*Map, Rim.X, Rim.Y) > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFogCurtainAboveCameraTest,
	"SolidCore1.Fog.CurtainAboveMaxZoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFogCurtainAboveCameraTest::RunTest(const FString& Parameters)
{
	const ASolidCharacter* Character = GetDefault<ASolidCharacter>();
	const ASolidTerrainStreamer* Streamer = GetDefault<ASolidTerrainStreamer>();
	TestNotNull(TEXT("character defaults"), Character);
	TestNotNull(TEXT("streamer defaults"), Streamer);

	TestTrue(TEXT("inner curtain clears the raven boom"),
		Streamer->FogVolumeHeightHalfCm > Character->GetCameraZoomMax());
	TestTrue(TEXT("inner curtain clears the falcon camera"),
		Streamer->FogVolumeHeightHalfCm > SolidSight::FalconMaxHeightCm);
	TestTrue(TEXT("inner curtain clears eagle sight"),
		Streamer->FogVolumeHeightHalfCm > SolidSight::EagleMaxHeightCm);
	TestTrue(TEXT("outer curtain clears eagle sight"),
		Streamer->FogVolumeHeightCm > SolidSight::EagleMaxHeightCm);
	TestTrue(TEXT("inner curtain is the fog height"),
		FMath::IsNearlyEqual(Streamer->FogVolumeHeightHalfCm, SolidTerrainFog::InnerCurtainHeightCm));
	TestTrue(TEXT("outer curtain is the fog height"),
		FMath::IsNearlyEqual(Streamer->FogVolumeHeightCm, SolidTerrainFog::OuterCurtainHeightCm));
	TestTrue(TEXT("both curtains are the same height"),
		FMath::IsNearlyEqual(Streamer->FogVolumeHeightCm, Streamer->FogVolumeHeightHalfCm));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
