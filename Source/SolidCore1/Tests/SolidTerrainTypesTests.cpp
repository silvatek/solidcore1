#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBiome.h"
#include "SolidTerrainMaterials.h"
#include "SolidTerrainTypes.h"
#include "SolidWorldChart.h"
#include "Materials/Material.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeToStringTest,
	"SolidCore1.Types.BiomeToString",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeToStringTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Grassland"), FString(SolidBiome::Of(ESolidBiome::Grassland).Name()), FString(TEXT("Grassland")));
	TestEqual(TEXT("Forest"), FString(SolidBiome::Of(ESolidBiome::Forest).Name()), FString(TEXT("Forest")));
	TestEqual(TEXT("Mountain"), FString(SolidBiome::Of(ESolidBiome::Mountain).Name()), FString(TEXT("Mountain")));
	TestEqual(TEXT("Town"), FString(SolidBiome::Of(ESolidBiome::Town).Name()), FString(TEXT("Town")));
	TestEqual(TEXT("Desert"), FString(SolidBiome::Of(ESolidBiome::Desert).Name()), FString(TEXT("Desert")));
	TestEqual(TEXT("Swamp"), FString(SolidBiome::Of(ESolidBiome::Swamp).Name()), FString(TEXT("Swamp")));
	TestEqual(TEXT("Sea"), FString(SolidBiome::Of(ESolidBiome::Sea).Name()), FString(TEXT("Sea")));
	TestEqual(TEXT("River"), FString(SolidBiome::Of(ESolidBiome::River).Name()), FString(TEXT("River")));
	TestEqual(TEXT("Road"), FString(SolidBiome::Of(ESolidBiome::Road).Name()), FString(TEXT("Road")));
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
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Sea).HeightOffsetCm(), 0.f));
	TestTrue(TEXT("River offset 0"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::River).HeightOffsetCm(), 0.f));
	TestTrue(TEXT("Mountain offset 10m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Mountain).HeightOffsetCm(), 1000.f));
	TestTrue(TEXT("Grassland offset 1m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Grassland).HeightOffsetCm(), 100.f));
	TestTrue(TEXT("Forest offset 1m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Forest).HeightOffsetCm(), 100.f));
	TestTrue(TEXT("Town offset 1m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Town).HeightOffsetCm(), 100.f));
	TestTrue(TEXT("Desert offset 1m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Desert).HeightOffsetCm(), 100.f));
	TestTrue(TEXT("Swamp offset 1m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Swamp).HeightOffsetCm(), 100.f));
	TestTrue(TEXT("road shares the land shelf"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Road).HeightOffsetCm(), 100.f));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeColorTest,
	"SolidCore1.Types.BiomeColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeColorTest::RunTest(const FString& Parameters)
{
	const FLinearColor Sea = SolidBiome::Of(ESolidBiome::Sea).Color();
	const FLinearColor River = SolidBiome::Of(ESolidBiome::River).Color();
	TestTrue(TEXT("sea is blue"), Sea.Equals(FLinearColor(0.15f, 0.35f, 0.75f)));
	TestTrue(TEXT("the river uses the sea blue"), River.Equals(Sea));
	TestTrue(TEXT("grassland is light green"),
		SolidBiome::Of(ESolidBiome::Grassland).Color().Equals(FLinearColor(0.45f, 0.72f, 0.28f)));
	TestTrue(TEXT("forest is dark green"),
		SolidBiome::Of(ESolidBiome::Forest).Color().Equals(FLinearColor(0.08f, 0.28f, 0.08f)));
	TestTrue(TEXT("mountain is dark grey"),
		SolidBiome::Of(ESolidBiome::Mountain).Color().Equals(FLinearColor(0.16f, 0.16f, 0.18f)));
	TestTrue(TEXT("town is brown"),
		SolidBiome::Of(ESolidBiome::Town).Color().Equals(FLinearColor(0.45f, 0.28f, 0.12f)));
	TestTrue(TEXT("desert is yellow"),
		SolidBiome::Of(ESolidBiome::Desert).Color().Equals(FLinearColor(0.85f, 0.75f, 0.32f)));
	TestTrue(TEXT("swamp is a dark green"),
		SolidBiome::Of(ESolidBiome::Swamp).Color().Equals(FLinearColor(0.22f, 0.32f, 0.18f)));
	TestTrue(TEXT("road is brown"),
		SolidBiome::Of(ESolidBiome::Road).Color().Equals(FLinearColor(0.52f, 0.38f, 0.20f)));

	TestTrue(TEXT("the map asks the biome"),
		USolidWorldChart::DefaultColorForBiome(ESolidBiome::River).Equals(River));

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		Text += FString::ChrN(USolidWorldChart::MapWidth, USolidWorldChart::GrassMarker);
		Text += TEXT("\n");
	}
	Text += TEXT("* = River (red)\n");
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("a map with a colour line loads"), Map && Map->LoadFromString(Text)))
	{
		return false;
	}
	TestTrue(TEXT("a colour line leaves the river blue"), Map->GetBiomeColor(ESolidBiome::River).Equals(River));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeGroundTest,
	"SolidCore1.Types.BiomeGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeGroundTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("grass letter"), SolidBiome::Of(ESolidBiome::Grassland).Marker() == USolidWorldChart::GrassMarker);
	TestTrue(TEXT("forest letter"), SolidBiome::Of(ESolidBiome::Forest).Marker() == USolidWorldChart::ForestMarker);
	TestTrue(TEXT("mountain letter"), SolidBiome::Of(ESolidBiome::Mountain).Marker() == TEXT('M'));
	TestTrue(TEXT("town letter"), SolidBiome::Of(ESolidBiome::Town).Marker() == USolidWorldChart::TownMarker);
	TestTrue(TEXT("desert letter"), SolidBiome::Of(ESolidBiome::Desert).Marker() == USolidWorldChart::DesertMarker);
	TestTrue(TEXT("sea letter"), SolidBiome::Of(ESolidBiome::Sea).Marker() == USolidWorldChart::SeaMarker);
	TestTrue(TEXT("river letter"), SolidBiome::Of(ESolidBiome::River).Marker() == USolidWorldChart::RiverMarker);
	TestTrue(TEXT("swamp letter"), SolidBiome::Of(ESolidBiome::Swamp).Marker() == TEXT('S'));
	TestTrue(TEXT("road letter"), SolidBiome::Of(ESolidBiome::Road).Marker() == TEXT('R'));

	TestTrue(TEXT("grass is paintable"), SolidBiome::Of(ESolidBiome::Grassland).Paintable());
	TestTrue(TEXT("forest is paintable"), SolidBiome::Of(ESolidBiome::Forest).Paintable());
	TestTrue(TEXT("mountain is paintable"), SolidBiome::Of(ESolidBiome::Mountain).Paintable());
	TestTrue(TEXT("town is paintable"), SolidBiome::Of(ESolidBiome::Town).Paintable());
	TestTrue(TEXT("desert is paintable"), SolidBiome::Of(ESolidBiome::Desert).Paintable());
	TestTrue(TEXT("sea is paintable"), SolidBiome::Of(ESolidBiome::Sea).Paintable());
	TestTrue(TEXT("river is paintable"), SolidBiome::Of(ESolidBiome::River).Paintable());
	TestFalse(TEXT("swamp is not a paint biome"), SolidBiome::Of(ESolidBiome::Swamp).Paintable());
	TestFalse(TEXT("road is not a paint biome"), SolidBiome::Of(ESolidBiome::Road).Paintable());

	TestFalse(TEXT("grass is open"), SolidBiome::Of(ESolidBiome::Grassland).Impassable());
	TestFalse(TEXT("forest is open"), SolidBiome::Of(ESolidBiome::Forest).Impassable());
	TestFalse(TEXT("town is open"), SolidBiome::Of(ESolidBiome::Town).Impassable());
	TestFalse(TEXT("desert is open"), SolidBiome::Of(ESolidBiome::Desert).Impassable());
	TestFalse(TEXT("swamp is open"), SolidBiome::Of(ESolidBiome::Swamp).Impassable());
	TestFalse(TEXT("road is open"), SolidBiome::Of(ESolidBiome::Road).Impassable());
	TestTrue(TEXT("sea is closed"), SolidBiome::Of(ESolidBiome::Sea).Impassable());
	TestTrue(TEXT("mountain is closed"), SolidBiome::Of(ESolidBiome::Mountain).Impassable());
	TestTrue(TEXT("river is closed"), SolidBiome::Of(ESolidBiome::River).Impassable());

	TestEqual(TEXT("grass is the easiest ground"), SolidBiome::Of(ESolidBiome::Grassland).SurfacePreference(), 0);
	TestEqual(TEXT("town is the easiest ground"), SolidBiome::Of(ESolidBiome::Town).SurfacePreference(), 0);
	TestEqual(TEXT("road is the easiest ground"), SolidBiome::Of(ESolidBiome::Road).SurfacePreference(), 0);
	TestEqual(TEXT("swamp is the easiest ground"), SolidBiome::Of(ESolidBiome::Swamp).SurfacePreference(), 0);
	TestEqual(TEXT("forest is a little harder"), SolidBiome::Of(ESolidBiome::Forest).SurfacePreference(), 2);
	TestEqual(TEXT("desert is a little harder"), SolidBiome::Of(ESolidBiome::Desert).SurfacePreference(), 2);
	TestEqual(TEXT("mountain is harder still"), SolidBiome::Of(ESolidBiome::Mountain).SurfacePreference(), 4);
	TestEqual(TEXT("river is harder still"), SolidBiome::Of(ESolidBiome::River).SurfacePreference(), 6);
	TestEqual(TEXT("sea is the hardest ground"), SolidBiome::Of(ESolidBiome::Sea).SurfacePreference(), 8);

	const FSolidBiome& Unknown = SolidBiome::Of(static_cast<ESolidBiome>(255));
	TestEqual(TEXT("an unknown token has no name"), FString(Unknown.Name()), FString(TEXT("Unknown")));
	TestTrue(TEXT("an unknown token shares the land shelf"), FMath::IsNearlyEqual(Unknown.HeightOffsetCm(), 100.f));
	TestEqual(TEXT("an unknown token is a weak preference"), Unknown.SurfacePreference(), 1);
	TestTrue(TEXT("an unknown token uses the grass letter"), Unknown.Marker() == USolidWorldChart::GrassMarker);
	TestFalse(TEXT("an unknown token stays open"), Unknown.Impassable());
	TestFalse(TEXT("an unknown token is not a paint biome"), Unknown.Paintable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeFlatColourTest,
	"SolidCore1.Types.FlatColour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeFlatColourTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("forest is a flat colour"), SolidBiome::Of(ESolidBiome::Forest).FlatColour());
	TestTrue(TEXT("town is a flat colour"), SolidBiome::Of(ESolidBiome::Town).FlatColour());
	TestTrue(TEXT("desert is a flat colour"), SolidBiome::Of(ESolidBiome::Desert).FlatColour());
	TestTrue(TEXT("swamp is a flat colour"), SolidBiome::Of(ESolidBiome::Swamp).FlatColour());
	TestTrue(TEXT("sea is a flat colour"), SolidBiome::Of(ESolidBiome::Sea).FlatColour());
	TestTrue(TEXT("river is a flat colour"), SolidBiome::Of(ESolidBiome::River).FlatColour());
	TestFalse(TEXT("grassland keeps its material"), SolidBiome::Of(ESolidBiome::Grassland).FlatColour());
	TestFalse(TEXT("a road keeps a hard edge"), SolidBiome::Of(ESolidBiome::Road).FlatColour());
	TestFalse(TEXT("a mountain keeps a hard edge"), SolidBiome::Of(ESolidBiome::Mountain).FlatColour());
	TestFalse(TEXT("an unknown token is not a flat colour"),
		SolidBiome::Of(static_cast<ESolidBiome>(255)).FlatColour());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeFadeWeightTest,
	"SolidCore1.Types.FadeWeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeFadeWeightTest::RunTest(const FString& Parameters)
{
	const ESolidBiome Forest = ESolidBiome::Forest;
	const ESolidBiome Desert = ESolidBiome::Desert;
	const ESolidBiome Patch[3][4] = {
		{ Forest, Forest, Forest, Forest },
		{ Forest, Forest, Desert, Desert },
		{ Desert, Desert, Desert, Desert },
	};
	float Weights[4] = { -1.f, -1.f, -1.f, -1.f };

	auto Matches = [this](const TCHAR* What, const float* Got, const float A, const float B, const float C, const float D)
	{
		return TestTrue(
			What,
			FMath::IsNearlyEqual(Got[0], A)
				&& FMath::IsNearlyEqual(Got[1], B)
				&& FMath::IsNearlyEqual(Got[2], C)
				&& FMath::IsNearlyEqual(Got[3], D));
	};

	TestFalse(
		TEXT("one biome keeps its material"),
		SolidTerrainTypes::FadeWeight(Patch[0][0], Patch[0][1], Patch[1][0], Patch[1][1], Weights));

	TestTrue(
		TEXT("the odd desert corner fades"),
		SolidTerrainTypes::FadeWeight(Patch[0][1], Patch[0][2], Patch[1][1], Patch[1][2], Weights));
	Matches(TEXT("that odd corner is fully desert"), Weights, 0.f, 0.f, 0.f, 1.f);

	TestTrue(
		TEXT("a straight run fades"),
		SolidTerrainTypes::FadeWeight(Patch[0][2], Patch[0][3], Patch[1][2], Patch[1][3], Weights));
	Matches(TEXT("one row is forest and the next is desert"), Weights, 0.f, 0.f, 1.f, 1.f);
	TestTrue(
		TEXT("the halfway line is the middle of that fade"),
		FMath::IsNearlyEqual((Weights[0] + Weights[2]) * 0.5f, 0.5f)
			&& FMath::IsNearlyEqual((Weights[1] + Weights[3]) * 0.5f, 0.5f));

	TestTrue(
		TEXT("the odd forest corner fades"),
		SolidTerrainTypes::FadeWeight(Patch[1][1], Patch[1][2], Patch[2][1], Patch[2][2], Weights));
	Matches(TEXT("that odd corner is fully forest"), Weights, 0.f, 1.f, 1.f, 1.f);

	TestFalse(
		TEXT("desert alone keeps its material"),
		SolidTerrainTypes::FadeWeight(Patch[2][2], Patch[2][3], Patch[2][2], Patch[2][3], Weights));
	TestFalse(
		TEXT("a road corner keeps the hard material"),
		SolidTerrainTypes::FadeWeight(Forest, Forest, Desert, ESolidBiome::Road, Weights));
	TestFalse(
		TEXT("a higher shelf keeps the hard material"),
		SolidTerrainTypes::FadeWeight(Forest, Forest, Desert, ESolidBiome::Mountain, Weights));
	TestFalse(
		TEXT("three biomes keep the hard material"),
		SolidTerrainTypes::FadeWeight(Forest, Desert, ESolidBiome::Town, Forest, Weights));
	TestFalse(
		TEXT("grassland keeps the hard material"),
		SolidTerrainTypes::FadeWeight(ESolidBiome::Grassland, ESolidBiome::Grassland, Forest, Forest, Weights));
	TestFalse(
		TEXT("sea against forest keeps the higher land shelf"),
		SolidTerrainTypes::FadeWeight(ESolidBiome::Sea, ESolidBiome::Sea, Forest, Forest, Weights));

	TestTrue(
		TEXT("sea against river fades"),
		SolidTerrainTypes::FadeWeight(
			ESolidBiome::Sea, ESolidBiome::Sea, ESolidBiome::River, ESolidBiome::River, Weights));
	Matches(TEXT("sea and river share one shelf"), Weights, 0.f, 0.f, 1.f, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeDiagonalCutTest,
	"SolidCore1.Types.DiagonalCut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeDiagonalCutTest::RunTest(const FString& Parameters)
{
	const ESolidBiome Forest = ESolidBiome::Forest;
	const ESolidBiome Desert = ESolidBiome::Desert;
	const ESolidBiome Patch[3][4] = {
		{ Forest, Forest, Forest, Forest },
		{ Forest, Forest, Desert, Desert },
		{ Desert, Desert, Desert, Desert },
	};
	int32 Corner = -1;

	TestFalse(
		TEXT("one biome keeps its material"),
		SolidTerrainTypes::DiagonalCut(Patch[0][0], Patch[0][1], Patch[1][0], Patch[1][1], Corner));

	TestTrue(
		TEXT("the odd desert corner is cut off"),
		SolidTerrainTypes::DiagonalCut(Patch[0][1], Patch[0][2], Patch[1][1], Patch[1][2], Corner));
	TestEqual(TEXT("that corner is 11"), Corner, 3);

	TestFalse(
		TEXT("a straight run keeps the grid edge"),
		SolidTerrainTypes::DiagonalCut(Patch[0][2], Patch[0][3], Patch[1][2], Patch[1][3], Corner));

	TestTrue(
		TEXT("the odd forest corner is cut off"),
		SolidTerrainTypes::DiagonalCut(Patch[1][1], Patch[1][2], Patch[2][1], Patch[2][2], Corner));
	TestEqual(TEXT("that corner is 00"), Corner, 0);

	TestTrue(
		TEXT("an odd 10 corner is cut off"),
		SolidTerrainTypes::DiagonalCut(Forest, Desert, Forest, Forest, Corner));
	TestEqual(TEXT("that corner is 10"), Corner, 1);

	TestTrue(
		TEXT("an odd 01 corner is cut off"),
		SolidTerrainTypes::DiagonalCut(Forest, Forest, Desert, Forest, Corner));
	TestEqual(TEXT("that corner is 01"), Corner, 2);

	TestFalse(
		TEXT("desert alone keeps its material"),
		SolidTerrainTypes::DiagonalCut(Desert, Desert, Desert, Desert, Corner));
	TestFalse(
		TEXT("a checker keeps one material"),
		SolidTerrainTypes::DiagonalCut(Forest, Desert, Desert, Forest, Corner));
	TestFalse(
		TEXT("a road corner keeps the hard material"),
		SolidTerrainTypes::DiagonalCut(Forest, Forest, Desert, ESolidBiome::Road, Corner));
	TestFalse(
		TEXT("a higher shelf keeps the whole quad"),
		SolidTerrainTypes::DiagonalCut(ESolidBiome::Sea, ESolidBiome::Sea, ESolidBiome::Sea, Forest, Corner));
	TestFalse(
		TEXT("three biomes keep one material"),
		SolidTerrainTypes::DiagonalCut(Forest, Desert, ESolidBiome::Town, Forest, Corner));
	TestFalse(
		TEXT("grassland keeps its material"),
		SolidTerrainTypes::DiagonalCut(ESolidBiome::Grassland, ESolidBiome::Grassland, ESolidBiome::Grassland, Forest, Corner));

	TestTrue(
		TEXT("sea against river is cut"),
		SolidTerrainTypes::DiagonalCut(
			ESolidBiome::Sea, ESolidBiome::Sea, ESolidBiome::Sea, ESolidBiome::River, Corner));
	TestEqual(TEXT("the river corner is 11"), Corner, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeFadeMaterialTest,
	"SolidCore1.Types.FadeMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeFadeMaterialTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* Fade = SolidTerrainMaterials::FadeMaterial();
	if (!TestNotNull(TEXT("the fade material"), Fade))
	{
		return false;
	}
	const UMaterial* Material = Fade->GetMaterial();
	if (!TestNotNull(TEXT("the fade parent"), Material))
	{
		return false;
	}
	TestEqual(
		TEXT("the fade is opaque"),
		static_cast<int32>(Material->GetBlendMode()),
		static_cast<int32>(BLEND_Opaque));
	TestTrue(
		TEXT("the fade is lit"),
		Material->GetShadingModels().HasShadingModel(MSM_DefaultLit));
	TestTrue(TEXT("the same material is reused"), SolidTerrainMaterials::FadeMaterial() == Fade);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidQuadTrianglesTest,
	"SolidCore1.Types.QuadTriangles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidQuadTrianglesTest::RunTest(const FString& Parameters)
{
	const ESolidBiome Forest = ESolidBiome::Forest;
	const ESolidBiome Desert = ESolidBiome::Desert;

	TestEqual(
		TEXT("this build uses the northwest corner"),
		static_cast<uint8>(SolidTerrainTypes::QuadMaterial),
		static_cast<uint8>(SolidTerrainTypes::ESolidQuadMaterial::NorthWest));

	auto Expect = [this](const TCHAR* What, const SolidTerrainTypes::FSolidQuadTriangles& Tri, const bool bFlip, const ESolidBiome First, const ESolidBiome Second)
	{
		TestEqual(What, Tri.bFlip, bFlip);
		TestEqual(
			*FString::Printf(TEXT("%s first"), What),
			static_cast<uint8>(Tri.First),
			static_cast<uint8>(First));
		TestEqual(
			*FString::Printf(TEXT("%s second"), What),
			static_cast<uint8>(Tri.Second),
			static_cast<uint8>(Second));
	};

	Expect(
		TEXT("odd 11"),
		SolidTerrainTypes::QuadTriangles(SolidTerrainTypes::ESolidQuadMaterial::OddCorner, Forest, Forest, Forest, Forest, Desert),
		true, Forest, Desert);
	Expect(
		TEXT("odd 00"),
		SolidTerrainTypes::QuadTriangles(SolidTerrainTypes::ESolidQuadMaterial::OddCorner, Forest, Desert, Forest, Forest, Forest),
		true, Desert, Forest);
	Expect(
		TEXT("odd 10"),
		SolidTerrainTypes::QuadTriangles(SolidTerrainTypes::ESolidQuadMaterial::OddCorner, Forest, Forest, Desert, Forest, Forest),
		false, Desert, Forest);
	Expect(
		TEXT("odd 01"),
		SolidTerrainTypes::QuadTriangles(SolidTerrainTypes::ESolidQuadMaterial::OddCorner, Forest, Forest, Forest, Desert, Forest),
		false, Forest, Desert);
	Expect(
		TEXT("a straight run stays on the northwest corner"),
		SolidTerrainTypes::QuadTriangles(SolidTerrainTypes::ESolidQuadMaterial::OddCorner, Desert, Forest, Forest, Desert, Desert),
		false, Desert, Desert);
	Expect(
		TEXT("the northwest method leaves the odd corner"),
		SolidTerrainTypes::QuadTriangles(SolidTerrainTypes::ESolidQuadMaterial::NorthWest, Forest, Forest, Forest, Forest, Desert),
		false, Forest, Forest);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
