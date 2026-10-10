#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCameraFog.h"
#include "SolidCharacter.h"
#include "Terrain/SolidTerrainStreamer.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCameraFogClearScaleTest,
	"SolidCore1.Fog.CameraClearScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCameraFogClearScaleTest::RunTest(const FString& Parameters)
{
	const auto Clear = [](float) { return 0.f; };
	TestEqual(TEXT("clear camera stays"), SolidCameraFog::MaxClearScale(Clear), 1.f);

	const auto AnchorFogged = [](float Scale) { return Scale < 0.5f ? 1.f : 0.f; };
	TestEqual(TEXT("fogged anchor is not pulled"), SolidCameraFog::MaxClearScale(AnchorFogged), 1.f);

	// Clear on [0, 0.25], fog beyond. The boom must stop at or before the boundary.
	const auto Boundary = [](float Scale) { return Scale > 0.25f ? 1.f : 0.f; };
	const float Scale = SolidCameraFog::MaxClearScale(Boundary);
	TestTrue(TEXT("stops before fog"), Scale <= 0.25f);
	TestTrue(TEXT("reaches the clear side"), Scale > 0.24f);
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

	const float MaxZoom = Character->GetCameraZoomMax();
	TestTrue(TEXT("full curtain is well above max zoom"),
		Streamer->FogVolumeHeightCm > MaxZoom + 5000.f);
	TestTrue(TEXT("half curtain is above max zoom"),
		Streamer->FogVolumeHeightHalfCm > MaxZoom);
	TestTrue(TEXT("full curtain is taller than half"),
		Streamer->FogVolumeHeightCm > Streamer->FogVolumeHeightHalfCm);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
