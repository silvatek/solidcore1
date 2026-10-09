#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBuilding.h"
#include "SolidForestTrees.h"
#include "SolidMonolith.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidTownBuildings.h"
#include "SolidTree.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

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

		// Match an exact Forest grid node (avoids SamplePoint edge rounding at cell borders).
		bool bMatchesForestNode = false;
		for (const FSolidTerrainPoint& Point : Map->GetPoints())
		{
			if (Point.Biome == ESolidBiome::Forest
				&& FMath::IsNearlyEqual(Point.X, PositionsA[I].X, 0.5f)
				&& FMath::IsNearlyEqual(Point.Y, PositionsA[I].Y, 0.5f))
			{
				bMatchesForestNode = true;
				break;
			}
		}
		TestTrue(TEXT("spawn matches a Forest grid point"), bMatchesForestNode);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownBuildingPackTest,
	"SolidCore1.Vegetation.TownBuildingPack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownBuildingPackTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map"), Map);

	int32 TownPoints = 0;
	for (const FSolidTerrainPoint& Point : Map->GetPoints())
	{
		if (Point.Biome == ESolidBiome::Town)
		{
			++TownPoints;
		}
	}
	TestTrue(TEXT("map has some town cells"), TownPoints > 0);

	SolidTownBuildings::FScatterParams Params;
	Params.Density = 0.8f;
	Params.MaxBuildings = 64;
	Params.MinSeparationCm = 50.f;
	Params.ClearRadiusAroundTownCenterCm = 450.f;

	TArray<SolidTownBuildings::FPlacement> PlacementsA;
	TArray<SolidTownBuildings::FPlacement> PlacementsB;
	const int32 CountA = SolidTownBuildings::CollectPlacements(Map, 77, Params, PlacementsA);
	const int32 CountB = SolidTownBuildings::CollectPlacements(Map, 77, Params, PlacementsB);
	TestEqual(TEXT("same seed => same count"), CountA, CountB);
	TestEqual(TEXT("placements filled"), PlacementsA.Num(), CountA);
	TestTrue(TEXT("packed some buildings"), CountA > 0);
	TestTrue(TEXT("respects max"), CountA <= Params.MaxBuildings);

	FVector2D TownCenter = FVector2D::ZeroVector;
	const bool bHasTownCenter = Map->GetStartTownWorldXY(TownCenter);
	TestTrue(TEXT("start town XY available"), bHasTownCenter);

	for (int32 I = 0; I < PlacementsA.Num(); ++I)
	{
		const SolidTownBuildings::FPlacement& A = PlacementsA[I];
		const SolidTownBuildings::FPlacement& B = PlacementsB[I];
		TestTrue(TEXT("deterministic center"), A.CenterXY.Equals(B.CenterXY, 0.01f));
		TestTrue(TEXT("deterministic footprint X"),
			FMath::IsNearlyEqual(A.FootprintXCm, B.FootprintXCm));
		TestTrue(TEXT("deterministic yaw"), FMath::IsNearlyEqual(A.YawDeg, B.YawDeg));

		bool bMatchesTownNode = false;
		for (const FSolidTerrainPoint& Point : Map->GetPoints())
		{
			if (Point.Biome == ESolidBiome::Town
				&& FMath::IsNearlyEqual(Point.X, A.CenterXY.X, 0.5f)
				&& FMath::IsNearlyEqual(Point.Y, A.CenterXY.Y, 0.5f))
			{
				bMatchesTownNode = true;
				break;
			}
		}
		TestTrue(TEXT("placement matches a Town grid point"), bMatchesTownNode);

		if (bHasTownCenter)
		{
			TestTrue(TEXT("clears town center"),
				FVector2D::Distance(A.CenterXY, TownCenter) >= Params.ClearRadiusAroundTownCenterCm - 0.5f);
		}

		for (int32 J = I + 1; J < PlacementsA.Num(); ++J)
		{
			const SolidTownBuildings::FPlacement& Other = PlacementsA[J];
			const float MinDist =
				A.PackingRadiusCm() + Other.PackingRadiusCm() + Params.MinSeparationCm;
			TestTrue(TEXT("non-overlapping packing circles"),
				FVector2D::Distance(A.CenterXY, Other.CenterXY) + 0.5f >= MinDist);
		}
	}

	SolidTownBuildings::FScatterParams EmptyParams;
	EmptyParams.Density = 0.f;
	TArray<SolidTownBuildings::FPlacement> None;
	TestEqual(TEXT("zero density yields none"),
		SolidTownBuildings::CollectPlacements(Map, 77, EmptyParams, None), 0);
	TestEqual(TEXT("null map yields none"),
		SolidTownBuildings::CollectPlacements(nullptr, 77, Params, None), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBuildingDefaultsTest,
	"SolidCore1.Vegetation.BuildingDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBuildingDefaultsTest::RunTest(const FString& Parameters)
{
	ASolidBuilding* Building = NewObject<ASolidBuilding>();
	TestNotNull(TEXT("building"), Building);
	TestTrue(TEXT("footprint X positive"), Building->FootprintXCm > 0.f);
	TestTrue(TEXT("footprint Y positive"), Building->FootprintYCm > 0.f);
	TestTrue(TEXT("body height positive"), Building->BodyHeightCm > 0.f);
	TestTrue(TEXT("roof height positive"), Building->RoofHeightCm > 0.f);
	TestTrue(TEXT("packing radius positive"), Building->GetPackingRadiusCm() > 0.f);

	FRandomStream RngA(42);
	FRandomStream RngB(42);
	ASolidBuilding* A = NewObject<ASolidBuilding>();
	ASolidBuilding* B = NewObject<ASolidBuilding>();
	A->ApplyRandomVariation(RngA);
	B->ApplyRandomVariation(RngB);
	TestTrue(TEXT("same seed => same footprint X"),
		FMath::IsNearlyEqual(A->FootprintXCm, B->FootprintXCm));
	TestTrue(TEXT("same seed => same body height"),
		FMath::IsNearlyEqual(A->BodyHeightCm, B->BodyHeightCm));

	Building->BuildVisuals();
	TestNotNull(TEXT("root component"), Building->GetRootComponent());

	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	UStaticMeshComponent* RoofComp = nullptr;
	TArray<UStaticMeshComponent*> MeshComps;
	Building->GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Comp : MeshComps)
	{
		if (Comp && Comp->GetName() == TEXT("RoofMesh"))
		{
			RoofComp = Comp;
			break;
		}
	}
	TestNotNull(TEXT("RoofMesh component"), RoofComp);
	UStaticMesh* RoofMesh = RoofComp ? RoofComp->GetStaticMesh() : nullptr;
	TestNotNull(TEXT("roof has static mesh"), RoofMesh);
	if (RoofMesh)
	{
		TestTrue(TEXT("roof is not Engine Cone"), RoofMesh != Cone);
		TestTrue(TEXT("roof mesh is transient gable prism"),
			RoofMesh->HasAnyFlags(RF_Transient));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
