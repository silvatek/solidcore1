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
	TestEqual(TEXT("Sea"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Sea)), FString(TEXT("Sea")));
	TestEqual(TEXT("River"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::River)), FString(TEXT("River")));
	TestEqual(TEXT("Road"), FString(SolidTerrainTypes::BiomeToString(ESolidBiome::Road)), FString(TEXT("Road")));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeHeightOffsetTest,
	"SolidCore1.Types.BiomeHeightOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeHeightOffsetTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Sea offset 0"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Sea), 0.f));
	TestTrue(TEXT("River offset 0"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::River), 0.f));
	TestTrue(TEXT("Mountain offset 10m"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Mountain), 1000.f));
	TestTrue(TEXT("Grassland offset 1m"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Grassland), 100.f));
	TestTrue(TEXT("Forest offset 1m"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Forest), 100.f));
	TestTrue(TEXT("Town offset 1m"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Town), 100.f));
	TestTrue(TEXT("Desert offset 1m"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Desert), 100.f));
	TestTrue(TEXT("Swamp offset 1m"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Swamp), 100.f));
	TestTrue(TEXT("road shares the land shelf"),
		FMath::IsNearlyEqual(SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Road), 100.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeBorderTextureTest,
	"SolidCore1.Types.BorderBiome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeBorderTextureTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("mountain cliff beats the river"),
		static_cast<uint8>(SolidTerrainTypes::BorderBiome(
			ESolidBiome::River, ESolidBiome::Mountain, ESolidBiome::River, ESolidBiome::River)),
		static_cast<uint8>(ESolidBiome::Mountain));
	TestEqual(
		TEXT("grass bank beats the river"),
		static_cast<uint8>(SolidTerrainTypes::BorderBiome(
			ESolidBiome::River, ESolidBiome::River, ESolidBiome::Grassland, ESolidBiome::River)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("a river triangle stays river"),
		static_cast<uint8>(SolidTerrainTypes::BorderBiome(
			ESolidBiome::Mountain, ESolidBiome::River, ESolidBiome::River, ESolidBiome::River)),
		static_cast<uint8>(ESolidBiome::River));
	TestEqual(
		TEXT("equal shelves keep the quad biome"),
		static_cast<uint8>(SolidTerrainTypes::BorderBiome(
			ESolidBiome::Grassland, ESolidBiome::Forest, ESolidBiome::Grassland, ESolidBiome::Desert)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("mountain already on the quad stays"),
		static_cast<uint8>(SolidTerrainTypes::BorderBiome(
			ESolidBiome::Mountain, ESolidBiome::Mountain, ESolidBiome::River, ESolidBiome::Grassland)),
		static_cast<uint8>(ESolidBiome::Mountain));
	TestEqual(
		TEXT("sea and river share the shelf"),
		static_cast<uint8>(SolidTerrainTypes::BorderBiome(
			ESolidBiome::Sea, ESolidBiome::River, ESolidBiome::Sea, ESolidBiome::River)),
		static_cast<uint8>(ESolidBiome::Sea));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
