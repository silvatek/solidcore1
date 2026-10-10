#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFogMeshBuildGuardsTest,
	"SolidCore1.Fog.MeshBuildGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFogMeshBuildGuardsTest::RunTest(const FString& Parameters)
{
	SolidTerrainFog::FMeshBuildParams Params;
	Params.ChunkCoord = FIntPoint(0, 0);
	Params.ChunkWorldSize = 6400.f;
	Params.FogQuadsPerSide = 4;

	TestNull(TEXT("null outer => null mesh"),
		SolidTerrainFog::BuildChunkFogMesh(nullptr, nullptr, Params, nullptr, nullptr, nullptr));

	USolidTerrainMap* Map = NewObject<USolidTerrainMap>();
	Map->Build(1, 0.00012f, 1000.f, 0.f, 17, 17, 400.f, true);
	// Build leaves every point fully fogged, so there is no curtain until a player is placed.
	Map->CenterExplorationFogOn(0.f, 0.f);

	UObject* Outer = GetTransientPackage();
	TestNull(TEXT("no materials => null mesh"),
		SolidTerrainFog::BuildChunkFogMesh(Outer, Map, Params, nullptr, nullptr, nullptr));

	UMaterialInterface* Half = SolidTerrainFog::CreateHalfMaterial(Outer);
	UMaterialInterface* Full = SolidTerrainFog::CreateFullMaterial(Outer);
	UMaterialInterface* White = SolidTerrainFog::CreateOuterMaterial(Outer);
	TestNotNull(TEXT("half material"), Half);
	TestNotNull(TEXT("full material"), Full);
	TestNotNull(TEXT("outer material"), White);

	UStaticMesh* Mesh = SolidTerrainFog::BuildChunkFogMesh(Outer, Map, Params, Half, Full, White);
	TestNotNull(TEXT("fog mesh builds with materials"), Mesh);
	if (Mesh)
	{
		TestTrue(TEXT("fog mesh has triangles"), Mesh->GetNumTriangles(0) > 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
