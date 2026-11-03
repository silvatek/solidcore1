#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBiome.h"
#include "SolidQuadSplit.h"
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

	TestTrue(TEXT("grassland variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Grassland).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("forest variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Forest).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("desert variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Desert).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("town variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Town).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("road variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Road).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("sea variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Sea).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("swamp variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Swamp).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("mountain variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::Mountain).HeightVarianceCm(), 3000.f));
	TestTrue(TEXT("river variance is 30 m"),
		FMath::IsNearlyEqual(SolidBiome::Of(ESolidBiome::River).HeightVarianceCm(), 3000.f));
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
		static_cast<uint8>(FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).BorderBiome(
			ESolidBiome::River, ESolidBiome::Mountain, ESolidBiome::River, ESolidBiome::River)),
		static_cast<uint8>(ESolidBiome::Mountain));
	TestEqual(
		TEXT("grass bank beats the river"),
		static_cast<uint8>(FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).BorderBiome(
			ESolidBiome::River, ESolidBiome::River, ESolidBiome::Grassland, ESolidBiome::River)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("a river triangle stays river"),
		static_cast<uint8>(FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).BorderBiome(
			ESolidBiome::Mountain, ESolidBiome::River, ESolidBiome::River, ESolidBiome::River)),
		static_cast<uint8>(ESolidBiome::River));
	TestEqual(
		TEXT("equal shelves keep the quad biome"),
		static_cast<uint8>(FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).BorderBiome(
			ESolidBiome::Grassland, ESolidBiome::Forest, ESolidBiome::Grassland, ESolidBiome::Desert)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("mountain already on the quad stays"),
		static_cast<uint8>(FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).BorderBiome(
			ESolidBiome::Mountain, ESolidBiome::Mountain, ESolidBiome::River, ESolidBiome::Grassland)),
		static_cast<uint8>(ESolidBiome::Mountain));
	TestEqual(
		TEXT("sea and river share the shelf"),
		static_cast<uint8>(FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).BorderBiome(
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
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBiomeGroundTest,
	"SolidCore1.Types.BiomeGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBiomeGroundTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("grass letter"), SolidBiome::Of(ESolidBiome::Grassland).Marker() == TEXT('.'));
	TestTrue(TEXT("forest letter"), SolidBiome::Of(ESolidBiome::Forest).Marker() == TEXT('#'));
	TestTrue(TEXT("mountain letter"), SolidBiome::Of(ESolidBiome::Mountain).Marker() == TEXT('M'));
	TestTrue(TEXT("town letter"), SolidBiome::Of(ESolidBiome::Town).Marker() == TEXT('o'));
	TestTrue(TEXT("desert letter"), SolidBiome::Of(ESolidBiome::Desert).Marker() == TEXT('^'));
	TestTrue(TEXT("sea letter"), SolidBiome::Of(ESolidBiome::Sea).Marker() == TEXT('~'));
	TestTrue(TEXT("river letter"), SolidBiome::Of(ESolidBiome::River).Marker() == TEXT('*'));
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
	TestTrue(TEXT("an unknown token's height variance is 30 m"),
		FMath::IsNearlyEqual(Unknown.HeightVarianceCm(), 3000.f));
	TestEqual(TEXT("an unknown token is a weak preference"), Unknown.SurfacePreference(), 1);
	TestTrue(TEXT("an unknown token uses the grass letter"), Unknown.Marker() == SolidBiome::Of(ESolidBiome::Grassland).Marker());
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
		SolidTerrainMaterials::FadeWeight(Patch[0][0], Patch[0][1], Patch[1][0], Patch[1][1], Weights));

	TestTrue(
		TEXT("the odd desert corner fades"),
		SolidTerrainMaterials::FadeWeight(Patch[0][1], Patch[0][2], Patch[1][1], Patch[1][2], Weights));
	Matches(TEXT("that odd corner is fully desert"), Weights, 0.f, 0.f, 0.f, 1.f);

	TestTrue(
		TEXT("a straight run fades"),
		SolidTerrainMaterials::FadeWeight(Patch[0][2], Patch[0][3], Patch[1][2], Patch[1][3], Weights));
	Matches(TEXT("one row is forest and the next is desert"), Weights, 0.f, 0.f, 1.f, 1.f);
	TestTrue(
		TEXT("the halfway line is the middle of that fade"),
		FMath::IsNearlyEqual((Weights[0] + Weights[2]) * 0.5f, 0.5f)
			&& FMath::IsNearlyEqual((Weights[1] + Weights[3]) * 0.5f, 0.5f));

	TestTrue(
		TEXT("the odd forest corner fades"),
		SolidTerrainMaterials::FadeWeight(Patch[1][1], Patch[1][2], Patch[2][1], Patch[2][2], Weights));
	Matches(TEXT("that odd corner is fully forest"), Weights, 0.f, 1.f, 1.f, 1.f);

	TestFalse(
		TEXT("desert alone keeps its material"),
		SolidTerrainMaterials::FadeWeight(Patch[2][2], Patch[2][3], Patch[2][2], Patch[2][3], Weights));
	TestFalse(
		TEXT("a road corner keeps the hard material"),
		SolidTerrainMaterials::FadeWeight(Forest, Forest, Desert, ESolidBiome::Road, Weights));
	TestFalse(
		TEXT("a higher shelf keeps the hard material"),
		SolidTerrainMaterials::FadeWeight(Forest, Forest, Desert, ESolidBiome::Mountain, Weights));
	TestFalse(
		TEXT("three biomes keep the hard material"),
		SolidTerrainMaterials::FadeWeight(Forest, Desert, ESolidBiome::Town, Forest, Weights));
	TestFalse(
		TEXT("grassland keeps the hard material"),
		SolidTerrainMaterials::FadeWeight(ESolidBiome::Grassland, ESolidBiome::Grassland, Forest, Forest, Weights));
	TestFalse(
		TEXT("sea against forest keeps the higher land shelf"),
		SolidTerrainMaterials::FadeWeight(ESolidBiome::Sea, ESolidBiome::Sea, Forest, Forest, Weights));

	TestTrue(
		TEXT("sea against river fades"),
		SolidTerrainMaterials::FadeWeight(
			ESolidBiome::Sea, ESolidBiome::Sea, ESolidBiome::River, ESolidBiome::River, Weights));
	Matches(TEXT("sea and river share one shelf"), Weights, 0.f, 0.f, 1.f, 1.f);
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
		TEXT("this build uses PriorityMidpoint"),
		static_cast<uint8>(FSolidQuadSplit::QuadSplit),
		static_cast<uint8>(ESolidQuadSplit::PriorityMidpoint));

	auto Expect = [this](const TCHAR* What, const FSolidQuadTriangles& Tri, const bool bFlip, const ESolidBiome First, const ESolidBiome Second)
	{
		TestEqual(*FString::Printf(TEXT("%s count"), What), Tri.Count, 2);
		if (Tri.Count < 2)
		{
			return;
		}

		const int32 FirstCorner[3] = {
			bFlip ? 1 : 0,
			bFlip ? 0 : 3,
			bFlip ? 2 : 1 };
		const int32 SecondCorner[3] = {
			bFlip ? 1 : 0,
			2,
			3 };
		TestEqual(
			*FString::Printf(TEXT("%s first"), What),
			static_cast<uint8>(Tri.Triangles[0].Material),
			static_cast<uint8>(First));
		TestEqual(*FString::Printf(TEXT("%s first corner 0"), What), Tri.Triangles[0].Corner0, FirstCorner[0]);
		TestEqual(*FString::Printf(TEXT("%s first corner 1"), What), Tri.Triangles[0].Corner1, FirstCorner[1]);
		TestEqual(*FString::Printf(TEXT("%s first corner 2"), What), Tri.Triangles[0].Corner2, FirstCorner[2]);
		TestEqual(
			*FString::Printf(TEXT("%s second"), What),
			static_cast<uint8>(Tri.Triangles[1].Material),
			static_cast<uint8>(Second));
		TestEqual(*FString::Printf(TEXT("%s second corner 0"), What), Tri.Triangles[1].Corner0, SecondCorner[0]);
		TestEqual(*FString::Printf(TEXT("%s second corner 1"), What), Tri.Triangles[1].Corner1, SecondCorner[1]);
		TestEqual(*FString::Printf(TEXT("%s second corner 2"), What), Tri.Triangles[1].Corner2, SecondCorner[2]);
	};

	const ESolidBiome Grass = ESolidBiome::Grassland;
	const ESolidBiome Town = ESolidBiome::Town;
	const ESolidBiome Mountain = ESolidBiome::Mountain;

	Expect(
		TEXT("the northwest method leaves the odd corner"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).Triangles( Forest, Forest, Forest, Forest, Desert, Forest, Forest),
		false, Forest, Forest);
	TestEqual(
		TEXT("the debug line names both triangles"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Description(
			Desert, Forest, Forest, Forest, Desert, Forest, Forest),
		FString(TEXT("Forest / Desert")));
	TestEqual(
		TEXT("a matching pair is named twice"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWest).Description(
			Forest, Forest, Forest, Forest, Desert, Forest, Forest),
		FString(TEXT("Forest / Forest")));
	Expect(
		TEXT("a lone northwest corner is cut"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Desert, Forest, Forest, Forest, Desert, Forest, Forest),
		true, Forest, Desert);
	Expect(
		TEXT("a south corner that continues west is cut"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Grass, Grass, Town, Grass, Grass, Town, Town),
		false, Town, Grass);
	Expect(
		TEXT("a south corner whose west point matches the northwest stays"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Forest, Forest, Town, Forest, Forest, Forest, Forest),
		false, Forest, Forest);
	Expect(
		TEXT("a straight run stays whole under NorthWestCut"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Forest, Town, Town, Forest, Forest, Forest, Town),
		false, Forest, Forest);
	Expect(
		TEXT("a two-and-two edge cuts at its west end"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Town, Grass, Grass, Town, Town, Grass, Grass),
		false, Grass, Town);
	Expect(
		TEXT("a two-and-two edge stays whole where the west point matches"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Town, Grass, Grass, Town, Town, Town, Grass),
		false, Town, Town);
	Expect(
		TEXT("a two-and-two edge stays whole when the south-west point differs"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Town, Grass, Grass, Town, Town, Grass, Town),
		false, Town, Town);
	Expect(
		TEXT("a mountain top uses the mountain"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Grass, Grass, Grass, Mountain, Mountain, Grass, Grass),
		false, Mountain, Mountain);
	Expect(
		TEXT("a lone mountain corner leaves the grass triangle"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Grass, Grass, Grass, Grass, Mountain, Grass, Grass),
		true, Grass, Mountain);
	const ESolidBiome Sea = ESolidBiome::Sea;
	Expect(
		TEXT("a grass shelf replaces the sea on that triangle"),
		FSolidQuadSplit::Of(ESolidQuadSplit::NorthWestCut).Triangles( Grass, Sea, Sea, Grass, Grass, Sea, Sea),
		false, Grass, Grass);
	Expect(
		TEXT("four matching corners use that corner"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Forest, Grass, Grass, Grass, Grass, Desert, Town),
		false, Grass, Grass);
	Expect(
		TEXT("four different corners stay on the northwest corner"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Forest, Town, Desert, Forest, Grass, Town, Town),
		false, Forest, Forest);

	const int32 NorthSouthCorners[4][3] = {
		{ 0, 6, 7 }, { 0, 7, 1 }, { 2, 3, 7 }, { 2, 7, 6 } };
	const int32 WestEastCorners[4][3] = {
		{ 1, 5, 3 }, { 1, 4, 5 }, { 0, 2, 5 }, { 0, 5, 4 } };
	auto ExpectHalves = [this](
		const TCHAR* What,
		const FSolidQuadTriangles& Tri,
		const ESolidBiome First,
		const ESolidBiome Second,
		const int32 Corners[4][3])
	{
		TestEqual(*FString::Printf(TEXT("%s count"), What), Tri.Count, 4);
		if (Tri.Count < 4)
		{
			return;
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const ESolidBiome Material = Index < 2 ? First : Second;
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d"), What, Index),
				static_cast<uint8>(Tri.Triangles[Index].Material),
				static_cast<uint8>(Material));
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 0"), What, Index),
				Tri.Triangles[Index].Corner0, Corners[Index][0]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 1"), What, Index),
				Tri.Triangles[Index].Corner1, Corners[Index][1]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 2"), What, Index),
				Tri.Triangles[Index].Corner2, Corners[Index][2]);
		}
	};
	ExpectHalves(
		TEXT("north and south meet at the east and west midpoints"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Forest, Town, Town, Forest, Forest, Forest, Town),
		Town, Forest, NorthSouthCorners);
	ExpectHalves(
		TEXT("west and east meet at the north and south midpoints"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Forest, Forest, Grass, Forest, Grass, Town, Town),
		Grass, Forest, WestEastCorners);
	ExpectHalves(
		TEXT("a sea half beside grass takes the grass"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Sea, Sea, Grass, Grass, Town, Town),
		Grass, Grass, NorthSouthCorners);
	ExpectHalves(
		TEXT("a grass half beside a mountain takes the rock"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Grass, Grass, Mountain, Mountain, Town, Town),
		Mountain, Mountain, NorthSouthCorners);

	const int32 AcrossBand[6][3] = {
		{ 0, 7, 4 }, { 0, 3, 7 }, { 0, 6, 3 }, { 6, 5, 3 }, { 1, 4, 7 }, { 2, 5, 6 } };
	const int32 OtherBand[6][3] = {
		{ 1, 4, 6 }, { 1, 6, 2 }, { 1, 5, 7 }, { 1, 2, 5 }, { 0, 6, 4 }, { 3, 7, 5 } };
	auto ExpectChecker = [this](
		const TCHAR* What,
		const FSolidQuadTriangles& Tri,
		const ESolidBiome Band,
		const ESolidBiome Corner,
		const int32 Corners[6][3])
	{
		TestEqual(*FString::Printf(TEXT("%s count"), What), Tri.Count, 6);
		if (Tri.Count < 6)
		{
			return;
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const ESolidBiome Material = Index < 4 ? Band : Corner;
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d"), What, Index),
				static_cast<uint8>(Tri.Triangles[Index].Material),
				static_cast<uint8>(Material));
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 0"), What, Index),
				Tri.Triangles[Index].Corner0, Corners[Index][0]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 1"), What, Index),
				Tri.Triangles[Index].Corner1, Corners[Index][1]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 2"), What, Index),
				Tri.Triangles[Index].Corner2, Corners[Index][2]);
		}
	};
	ExpectChecker(
		TEXT("the northwest diagonal outranks the other pair"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Forest, Grass, Grass, Forest, Town, Town),
		Forest, Grass, AcrossBand);
	ExpectChecker(
		TEXT("the other diagonal outranks the northwest pair"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Grass, Forest, Forest, Grass, Town, Town),
		Forest, Grass, OtherBand);
	auto ExpectOneDiagonal = [this](
		const TCHAR* What,
		const FSolidQuadTriangles& Tri,
		const ESolidBiome Band,
		const ESolidBiome FirstCorner,
		const ESolidBiome SecondCorner,
		const int32 Corners[6][3])
	{
		TestEqual(*FString::Printf(TEXT("%s count"), What), Tri.Count, 6);
		if (Tri.Count < 6)
		{
			return;
		}
		const ESolidBiome Materials[6] = { Band, Band, Band, Band, FirstCorner, SecondCorner };
		for (int32 Index = 0; Index < 6; ++Index)
		{
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d"), What, Index),
				static_cast<uint8>(Tri.Triangles[Index].Material),
				static_cast<uint8>(Materials[Index]));
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 0"), What, Index),
				Tri.Triangles[Index].Corner0, Corners[Index][0]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 1"), What, Index),
				Tri.Triangles[Index].Corner1, Corners[Index][1]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 2"), What, Index),
				Tri.Triangles[Index].Corner2, Corners[Index][2]);
		}
	};
	ExpectOneDiagonal(
		TEXT("the northwest diagonal is the only pair"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Forest, Forest, Grass, Town, Forest, Town, Town),
		Forest, Grass, Town, AcrossBand);
	ExpectOneDiagonal(
		TEXT("the other diagonal is the only pair"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Grass, Town, Town, Desert, Town, Town),
		Town, Grass, Desert, OtherBand);

	const ESolidBiome Road = ESolidBiome::Road;
	const int32 Centre = FSolidQuadTriangle::Centre;
	const int32 SouthHigh[6][3] = {
		{ 0, 6, 7 }, { 0, 7, 1 }, { 2, Centre, 6 }, { 2, 5, Centre }, { 3, 7, Centre }, { 3, Centre, 5 } };
	const int32 WestHigh[6][3] = {
		{ 1, 5, 3 }, { 1, 4, 5 }, { 0, 6, Centre }, { 0, Centre, 4 }, { 2, Centre, 6 }, { 2, 5, Centre } };
	auto ExpectHigh = [this](
		const TCHAR* What,
		const FSolidQuadTriangles& Tri,
		const ESolidBiome High,
		const ESolidBiome Low0,
		const ESolidBiome Low1,
		const int32 Corners[6][3])
	{
		TestEqual(*FString::Printf(TEXT("%s count"), What), Tri.Count, 6);
		if (Tri.Count < 6)
		{
			return;
		}
		const ESolidBiome Materials[6] = { High, High, Low0, Low0, Low1, Low1 };
		for (int32 Index = 0; Index < 6; ++Index)
		{
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d"), What, Index),
				static_cast<uint8>(Tri.Triangles[Index].Material),
				static_cast<uint8>(Materials[Index]));
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 0"), What, Index),
				Tri.Triangles[Index].Corner0, Corners[Index][0]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 1"), What, Index),
				Tri.Triangles[Index].Corner1, Corners[Index][1]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 2"), What, Index),
				Tri.Triangles[Index].Corner2, Corners[Index][2]);
		}
	};
	ExpectHigh(
		TEXT("a south road splits the two biomes above it"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Road, Road, Grass, Town, Grass, Grass),
		Road, Grass, Town, SouthHigh);
	ExpectHigh(
		TEXT("a west road splits the two biomes to the east"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Grass, Road, Forest, Road, Grass, Grass),
		Road, Grass, Forest, WestHigh);

	const int32 SouthEastHigh[5][3] = {
		{ 0, 6, 4 }, { 2, 3, 7 }, { 2, 7, 6 }, { 1, 4, 6 }, { 1, 6, 7 } };
	const int32 NorthEastHigh[5][3] = {
		{ 2, 5, 6 }, { 0, 6, 7 }, { 0, 7, 1 }, { 3, 7, 6 }, { 3, 6, 5 } };
	const int32 NorthWestHigh[5][3] = {
		{ 3, 7, 5 }, { 0, 2, 5 }, { 0, 5, 4 }, { 1, 4, 5 }, { 1, 5, 7 } };
	auto ExpectOpposite = [this](
		const TCHAR* What,
		const FSolidQuadTriangles& Tri,
		const ESolidBiome High,
		const ESolidBiome Pair,
		const ESolidBiome Remaining,
		const int32 Corners[5][3])
	{
		TestEqual(*FString::Printf(TEXT("%s count"), What), Tri.Count, 5);
		if (Tri.Count < 5)
		{
			return;
		}
		const ESolidBiome Materials[5] = { High, Pair, Pair, Remaining, Remaining };
		for (int32 Index = 0; Index < 5; ++Index)
		{
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d"), What, Index),
				static_cast<uint8>(Tri.Triangles[Index].Material),
				static_cast<uint8>(Materials[Index]));
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 0"), What, Index),
				Tri.Triangles[Index].Corner0, Corners[Index][0]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 1"), What, Index),
				Tri.Triangles[Index].Corner1, Corners[Index][1]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 2"), What, Index),
				Tri.Triangles[Index].Corner2, Corners[Index][2]);
		}
	};
	ExpectOpposite(
		TEXT("a southeast town keeps its triangle above a grass pair"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Forest, Town, Desert, Grass, Grass, Town, Town),
		Town, Grass, Desert, SouthEastHigh);
	ExpectOpposite(
		TEXT("a northeast road keeps its triangle above a grass pair"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Grass, Grass, Road, Forest, Grass, Grass),
		Road, Grass, Forest, NorthEastHigh);
	ExpectOpposite(
		TEXT("a northwest road keeps its triangle beside an east grass pair"),
		FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Grass, Grass, Town, Grass, Road, Grass, Grass),
		Road, Grass, Town, NorthWestHigh);

	const ESolidBiome OddQuads[4][4] = {
		{ Desert, Grass, Grass, Grass },
		{ Grass, Desert, Grass, Grass },
		{ Grass, Grass, Desert, Grass },
		{ Grass, Grass, Grass, Desert },
	};
	const int32 OddCuts[4][4][3] = {
		{ { 0, 6, 4 }, { 3, 6, 2 }, { 3, 4, 6 }, { 3, 1, 4 } },
		{ { 1, 4, 7 }, { 2, 4, 0 }, { 2, 7, 4 }, { 2, 3, 7 } },
		{ { 2, 5, 6 }, { 1, 5, 3 }, { 1, 6, 5 }, { 1, 0, 6 } },
		{ { 3, 7, 5 }, { 0, 7, 1 }, { 0, 5, 7 }, { 0, 2, 5 } },
	};
	const TCHAR* OddNames[4] = {
		TEXT("southeast"), TEXT("southwest"), TEXT("northeast"), TEXT("northwest") };
	for (int32 Case = 0; Case < 4; ++Case)
	{
		const FSolidQuadTriangles Tri = FSolidQuadSplit::Of(ESolidQuadSplit::PriorityMidpoint).Triangles(
			Forest,
			OddQuads[Case][0], OddQuads[Case][1], OddQuads[Case][2], OddQuads[Case][3],
			Town, Town);
		TestEqual(*FString::Printf(TEXT("%s odd corner count"), OddNames[Case]), Tri.Count, 4);
		if (Tri.Count < 4)
		{
			continue;
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const ESolidBiome Material = Index == 0 ? Desert : Grass;
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d"), OddNames[Case], Index),
				static_cast<uint8>(Tri.Triangles[Index].Material),
				static_cast<uint8>(Material));
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 0"), OddNames[Case], Index),
				Tri.Triangles[Index].Corner0, OddCuts[Case][Index][0]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 1"), OddNames[Case], Index),
				Tri.Triangles[Index].Corner1, OddCuts[Case][Index][1]);
			TestEqual(
				*FString::Printf(TEXT("%s triangle %d corner 2"), OddNames[Case], Index),
				Tri.Triangles[Index].Corner2, OddCuts[Case][Index][2]);
		}
	}

	int32 End0 = -1;
	int32 End1 = -1;
	TestTrue(TEXT("the south midpoint joins the south corners"), FSolidQuadTriangle::MidpointEnds(4, End0, End1));
	TestEqual(TEXT("south end 0"), End0, 0);
	TestEqual(TEXT("south end 1"), End1, 1);
	TestTrue(TEXT("the north midpoint joins the north corners"), FSolidQuadTriangle::MidpointEnds(5, End0, End1));
	TestEqual(TEXT("north end 0"), End0, 2);
	TestEqual(TEXT("north end 1"), End1, 3);
	TestTrue(TEXT("the east midpoint joins the east corners"), FSolidQuadTriangle::MidpointEnds(6, End0, End1));
	TestEqual(TEXT("east end 0"), End0, 0);
	TestEqual(TEXT("east end 1"), End1, 2);
	TestTrue(TEXT("the west midpoint joins the west corners"), FSolidQuadTriangle::MidpointEnds(7, End0, End1));
	TestEqual(TEXT("west end 0"), End0, 1);
	TestEqual(TEXT("west end 1"), End1, 3);
	TestFalse(TEXT("a grid corner is not a midpoint"), FSolidQuadTriangle::MidpointEnds(3, End0, End1));

	FSolidQuadSplit::Cycle();
	TestEqual(
		TEXT("NorthWest follows PriorityMidpoint"),
		static_cast<uint8>(FSolidQuadSplit::QuadSplit),
		static_cast<uint8>(ESolidQuadSplit::NorthWest));
	TestEqual(
		TEXT("the debug line names NorthWest"),
		FString(FSolidQuadSplit::Of(FSolidQuadSplit::QuadSplit).Name()),
		FString(TEXT("NorthWest")));
	Expect(
		TEXT("selecting NorthWest restores those two triangles"),
		FSolidQuadSplit::Of(FSolidQuadSplit::QuadSplit).Triangles(Forest, Forest, Forest, Forest, Desert, Forest, Forest),
		false, Forest, Forest);
	FSolidQuadSplit::Cycle();
	TestEqual(
		TEXT("the next split is NorthWestCut"),
		static_cast<uint8>(FSolidQuadSplit::QuadSplit),
		static_cast<uint8>(ESolidQuadSplit::NorthWestCut));
	TestEqual(
		TEXT("the debug line names NorthWestCut"),
		FString(FSolidQuadSplit::Of(FSolidQuadSplit::QuadSplit).Name()),
		FString(TEXT("NorthWestCut")));
	FSolidQuadSplit::Cycle();
	TestEqual(
		TEXT("PriorityMidpoint follows NorthWestCut"),
		static_cast<uint8>(FSolidQuadSplit::QuadSplit),
		static_cast<uint8>(ESolidQuadSplit::PriorityMidpoint));
	TestEqual(
		TEXT("the debug line names PriorityMidpoint"),
		FString(FSolidQuadSplit::Of(FSolidQuadSplit::QuadSplit).Name()),
		FString(TEXT("PriorityMidpoint")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
