#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidForestTrees.h"
#include "SolidMonolith.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidTree.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidForestTreeScatterTest,
	"SolidCore1.Vegetation.ForestTreeScatter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidForestTreeScatterTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map"), Map);

	int32 ForestPoints = 0;
	for (const FSolidTerrainPoint& Point : Map->GetPoints())
	{
		if (Point.Biome == ESolidBiome::Forest)
		{
			++ForestPoints;
		}
	}
	TestTrue(TEXT("map has some forest cells"), ForestPoints > 0);

	SolidForestTrees::FScatterParams Params;
	Params.Density = 0.5f;
	Params.MaxTrees = 64;
	Params.JitterCm = 0.f; // exact grid points so biome checks stay crisp

	TArray<FVector2D> PositionsA;
	TArray<FVector2D> PositionsB;
	const int32 CountA = SolidForestTrees::CollectSpawnPositions(Map, 42, Params, PositionsA);
	const int32 CountB = SolidForestTrees::CollectSpawnPositions(Map, 42, Params, PositionsB);
	TestEqual(TEXT("same seed => same count"), CountA, CountB);
	TestEqual(TEXT("positions filled"), PositionsA.Num(), CountA);
	TestTrue(TEXT("spawned some trees"), CountA > 0);
	TestTrue(TEXT("respects max"), CountA <= Params.MaxTrees);

	for (int32 I = 0; I < PositionsA.Num(); ++I)
	{
		TestTrue(TEXT("deterministic positions"), PositionsA[I].Equals(PositionsB[I], 0.01f));
		const FSolidTerrainPoint Sample = Map->SamplePoint(PositionsA[I].X, PositionsA[I].Y);
		TestEqual(
			TEXT("spawn on forest biome"),
			static_cast<uint8>(Sample.Biome),
			static_cast<uint8>(ESolidBiome::Forest));
	}

	SolidForestTrees::FScatterParams EmptyParams;
	EmptyParams.Density = 0.f;
	TArray<FVector2D> None;
	TestEqual(TEXT("zero density yields none"),
		SolidForestTrees::CollectSpawnPositions(Map, 42, EmptyParams, None), 0);

	TestEqual(TEXT("null map yields none"),
		SolidForestTrees::CollectSpawnPositions(nullptr, 42, Params, None), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTreeRandomVariationTest,
	"SolidCore1.Vegetation.TreeRandomVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTreeRandomVariationTest::RunTest(const FString& Parameters)
{
	ASolidTree* A = NewObject<ASolidTree>();
	ASolidTree* B = NewObject<ASolidTree>();
	TestNotNull(TEXT("tree A"), A);
	TestNotNull(TEXT("tree B"), B);

	FRandomStream RngA(42);
	FRandomStream RngB(42);
	A->ApplyRandomVariation(RngA);
	B->ApplyRandomVariation(RngB);

	TestTrue(TEXT("same seed => same trunk height"),
		FMath::IsNearlyEqual(A->TrunkHeightCm, B->TrunkHeightCm));
	TestTrue(TEXT("same seed => same trunk radius"),
		FMath::IsNearlyEqual(A->TrunkRadiusCm, B->TrunkRadiusCm));
	TestTrue(TEXT("same seed => same canopy height"),
		FMath::IsNearlyEqual(A->CanopyHeightCm, B->CanopyHeightCm));
	TestTrue(TEXT("same seed => same canopy radius"),
		FMath::IsNearlyEqual(A->CanopyRadiusCm, B->CanopyRadiusCm));

	TestTrue(TEXT("trunk height positive"), A->TrunkHeightCm > 10.f);
	TestTrue(TEXT("trunk radius clamped"), A->TrunkRadiusCm >= 8.f);
	TestTrue(TEXT("canopy radius clamped"), A->CanopyRadiusCm >= 40.f);

	FRandomStream RngOther(99);
	ASolidTree* C = NewObject<ASolidTree>();
	C->ApplyRandomVariation(RngOther);
	TestTrue(TEXT("different seed usually changes size"),
		!FMath::IsNearlyEqual(A->TrunkHeightCm, C->TrunkHeightCm)
		|| !FMath::IsNearlyEqual(A->CanopyRadiusCm, C->CanopyRadiusCm));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonolithDefaultsTest,
	"SolidCore1.Vegetation.MonolithDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonolithDefaultsTest::RunTest(const FString& Parameters)
{
	ASolidMonolith* Mono = NewObject<ASolidMonolith>();
	TestNotNull(TEXT("monolith"), Mono);
	TestTrue(TEXT("width positive"), Mono->WidthCm > 0.f);
	TestTrue(TEXT("thickness positive"), Mono->ThicknessCm > 0.f);
	TestTrue(TEXT("height positive"), Mono->HeightCm > 0.f);
	TestTrue(TEXT("taller than wide"), Mono->HeightCm > Mono->WidthCm);

	Mono->BuildVisuals();
	TestNotNull(TEXT("slab mesh component"), Mono->GetRootComponent());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
