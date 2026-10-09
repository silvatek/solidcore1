#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeToStringTest,
	"SolidCore1.Types.BiomeToString",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeToStringTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Grassland"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Grassland)), FString(TEXT("Grassland")));
	TestEqual(TEXT("Forest"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Forest)), FString(TEXT("Forest")));
	TestEqual(TEXT("Mountain"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Mountain)), FString(TEXT("Mountain")));
	TestEqual(TEXT("Town"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Town)), FString(TEXT("Town")));
	TestEqual(TEXT("Desert"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Desert)), FString(TEXT("Desert")));
	TestEqual(TEXT("Swamp"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Swamp)), FString(TEXT("Swamp")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainPointDefaultsTest,
	"SolidCore1.Types.TerrainPointDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainPointDefaultsTest::RunTest(const FString& Parameters)
{
	const FSolidTerrainPoint Point;
	TestTrue(TEXT("default fog is fully fogged"), FMath::IsNearlyEqual(Point.Fog, 1.f));
	TestTrue(TEXT("default threat is zero"), FMath::IsNearlyEqual(Point.Threat, 0.f));
	TestEqual(TEXT("default biome grassland"), static_cast<uint8>(Point.Biome), static_cast<uint8>(ESolidBiome::Grassland));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
