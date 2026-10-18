#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HUD/SolidHUD.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Inventory/SolidStorage.h"
#include "SolidClickMove.h"
#include "SolidDungeonLevel.h"
#include "SolidForestTrees.h"
#include "SolidForage.h"
#include "SolidForagePile.h"
#include "SolidGameState.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FSolidTerrainPoint ForagePoint(const int32 Index, const ESolidBiome Biome, const float Fog = 1.f)
	{
		FSolidTerrainPoint Point;
		Point.X = static_cast<float>(Index) * 200.f;
		Point.Y = 0.f;
		Point.Biome = Biome;
		Point.Fog = Fog;
		return Point;
	}

	TArray<FSolidTerrainPoint> ForestRow(const int32 Count, const float Fog = 1.f)
	{
		TArray<FSolidTerrainPoint> Points;
		Points.Reserve(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Points.Add(ForagePoint(Index, ESolidBiome::Forest, Fog));
		}
		return Points;
	}

	bool SameChoices(const TArray<SolidForage::FChoice>& A, const TArray<SolidForage::FChoice>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].PointIndex != B[Index].PointIndex
				|| A[Index].ItemId != B[Index].ItemId
				|| !A[Index].XY.Equals(B[Index].XY, 0.01f))
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidForageScatterTest,
	"SolidCore1.Forage.Scatter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidForageScatterTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("a stick occurs on forest"), SolidForage::OccursInBiome(SolidItemType::IdStick, ESolidBiome::Forest));
	TestTrue(TEXT("pitch occurs on forest"), SolidForage::OccursInBiome(SolidItemType::IdPitch, ESolidBiome::Forest));
	TestFalse(TEXT("a stick does not occur at sea"), SolidForage::OccursInBiome(SolidItemType::IdStick, ESolidBiome::Sea));
	TestFalse(TEXT("pitch does not occur on a road"), SolidForage::OccursInBiome(SolidItemType::IdPitch, ESolidBiome::Road));
	TestFalse(TEXT("pitch does not occur on grassland"), SolidForage::OccursInBiome(SolidItemType::IdPitch, ESolidBiome::Grassland));
	TestFalse(TEXT("coins are not a ground find"), SolidForage::OccursInBiome(SolidItemType::IdCoins, ESolidBiome::Forest));

	TestEqual(TEXT("a stick uses the cube"), FString(SolidForage::MeshPath(SolidItemType::IdStick)), FString(SolidForage::CubeMesh));
	TestEqual(TEXT("pitch uses the same cube"), FString(SolidForage::MeshPath(SolidItemType::IdPitch)), FString(SolidForage::CubeMesh));
	TestEqual(TEXT("any id uses the cube for now"), FString(SolidForage::MeshPath(SolidItemType::IdCoins)), FString(SolidForage::CubeMesh));

	TestTrue(TEXT("the open world has piles"), SolidForage::WorldHasPiles(false, false, false));
	TestFalse(TEXT("the dungeon has no piles"), SolidForage::WorldHasPiles(true, false, false));
	TestFalse(TEXT("a tower has no piles"), SolidForage::WorldHasPiles(false, true, false));
	TestFalse(TEXT("the tunnels have no piles"), SolidForage::WorldHasPiles(false, false, true));

	TArray<FSolidTerrainPoint> Points = ForestRow(40);
	Points.Add(ForagePoint(40, ESolidBiome::Sea));
	Points.Add(ForagePoint(41, ESolidBiome::Road));
	Points.Add(ForagePoint(42, ESolidBiome::Grassland));

	SolidForage::FScatterParams Params;
	TArray<int32> NoTrees;
	TArray<SolidForage::FChoice> Choices;
	TArray<SolidForage::FChoice> Again;
	TestEqual(TEXT("forty treeless forest points choose two piles"),
		SolidForage::CollectChoices(Points, NoTrees, SolidForage::Seed, Params, Choices), 2);
	TestEqual(TEXT("the same seed chooses the same piles"),
		SolidForage::CollectChoices(Points, NoTrees, SolidForage::Seed, Params, Again), 2);
	TestTrue(TEXT("the choice list is deterministic"), SameChoices(Choices, Again));
	TestEqual(TEXT("the first pile is a stick"), FString(Choices[0].ItemId), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the second pile is pitch"), FString(Choices[1].ItemId), FString(SolidItemType::IdPitch));
	TestTrue(TEXT("the two piles use different points"), Choices[0].PointIndex != Choices[1].PointIndex);

	for (const SolidForage::FChoice& Choice : Choices)
	{
		TestTrue(TEXT("a pile sits on a forest point"), Points.IsValidIndex(Choice.PointIndex));
		TestTrue(TEXT("that point is forest"), Points[Choice.PointIndex].Biome == ESolidBiome::Forest);
		const float Dx = FMath::Abs(Choice.XY.X - Points[Choice.PointIndex].X);
		const float Dy = FMath::Abs(Choice.XY.Y - Points[Choice.PointIndex].Y);
		TestTrue(TEXT("jitter stays within 90 cm"), Dx <= Params.JitterCm + 0.01f && Dy <= Params.JitterCm + 0.01f);
	}

	TArray<FSolidTerrainPoint> Bare;
	Bare.Add(ForagePoint(0, ESolidBiome::Sea));
	Bare.Add(ForagePoint(1, ESolidBiome::Grassland));
	TArray<SolidForage::FChoice> None;
	TestEqual(TEXT("a map with no forest chooses none"),
		SolidForage::CollectChoices(Bare, NoTrees, SolidForage::Seed, Params, None), 0);

	TArray<int32> EveryPoint;
	for (int32 Index = 0; Index < 40; ++Index)
	{
		EveryPoint.Add(Index);
	}
	TestEqual(TEXT("forest points that all hold trees choose none"),
		SolidForage::CollectChoices(ForestRow(40), EveryPoint, SolidForage::Seed, Params, None), 0);

	TArray<FSolidTerrainPoint> WithTrees = ForestRow(45);
	TArray<int32> TreePoints;
	for (int32 Index = 40; Index < 45; ++Index)
	{
		TreePoints.Add(Index);
	}
	TArray<SolidForage::FChoice> Open;
	TestEqual(TEXT("forty treeless points among trees still choose two"),
		SolidForage::CollectChoices(WithTrees, TreePoints, SolidForage::Seed, Params, Open), 2);
	for (const SolidForage::FChoice& Choice : Open)
	{
		TestFalse(TEXT("a tree point holds no pile"), TreePoints.Contains(Choice.PointIndex));
	}

	TArray<SolidForage::FChoice> Capped;
	TestEqual(TEXT("the scatter stops at 64"),
		SolidForage::CollectChoices(ForestRow(2000), NoTrees, SolidForage::Seed, Params, Capped), SolidForage::MaxPiles);
	TSet<int32> Used;
	for (int32 Index = 0; Index < Capped.Num(); ++Index)
	{
		const TCHAR* Expected = (Index % 2 == 0) ? SolidItemType::IdStick : SolidItemType::IdPitch;
		TestEqual(TEXT("sticks and pitch alternate"), FString(Capped[Index].ItemId), FString(Expected));
		TestFalse(TEXT("each forest point holds at most one pile"), Used.Contains(Capped[Index].PointIndex));
		Used.Add(Capped[Index].PointIndex);
	}

	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map"), Map);
	if (!Map)
	{
		return false;
	}
	SolidForestTrees::FScatterParams TreeParams;
	TreeParams.Density = 0.5f;
	TreeParams.MaxTrees = 64;
	TreeParams.JitterCm = 0.f;
	TArray<int32> TreeIndices;
	TArray<FVector2D> TreeXY;
	const int32 TreeCount = SolidForestTrees::CollectChosenPointIndices(Map->GetPoints(), 42, TreeParams, TreeIndices);
	TestEqual(TEXT("zero jitter uses those same points"),
		SolidForestTrees::CollectSpawnPositions(Map, 42, TreeParams, TreeXY), TreeCount);
	const TArray<FSolidTerrainPoint>& MapPoints = Map->GetPoints();
	for (int32 Index = 0; Index < TreeIndices.Num(); ++Index)
	{
		const FSolidTerrainPoint& Point = MapPoints[TreeIndices[Index]];
		TestTrue(TEXT("the tree index is that grid point"),
			FMath::IsNearlyEqual(Point.X, TreeXY[Index].X, 0.5f)
			&& FMath::IsNearlyEqual(Point.Y, TreeXY[Index].Y, 0.5f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidForageSeedTest,
	"SolidCore1.Forage.Seed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidForageSeedTest::RunTest(const FString& Parameters)
{
	TArray<FSolidTerrainPoint> Points = ForestRow(40, 1.f);
	SolidForage::FScatterParams Params;
	TArray<int32> NoTrees;
	TArray<SolidForage::FChoice> Choices;
	TestEqual(TEXT("the row chooses two piles"),
		SolidForage::CollectChoices(Points, NoTrees, SolidForage::Seed, Params, Choices), 2);

	TArray<int32> Taken;
	TArray<int32> Seeded;
	TArray<int32> Ready;
	SolidForage::CollectReadyToSeed(Points, Choices, Taken, Seeded, Ready);
	TestEqual(TEXT("full fog seeds nothing"), Ready.Num(), 0);

	Points[Choices[0].PointIndex].Fog = 0.5f;
	SolidForage::CollectReadyToSeed(Points, Choices, Taken, Seeded, Ready);
	TestEqual(TEXT("one opened point seeds that pile"), Ready.Num(), 1);
	TestEqual(TEXT("the opened pile is the one whose fog fell"), Ready[0], 0);

	Seeded.Add(0);
	SolidForage::CollectReadyToSeed(Points, Choices, Taken, Seeded, Ready);
	TestEqual(TEXT("a pile already seeded is left alone"), Ready.Num(), 0);

	Seeded.Reset();
	Taken.Add(0);
	SolidForage::CollectReadyToSeed(Points, Choices, Taken, Seeded, Ready);
	TestEqual(TEXT("a taken pile is not seeded again"), Ready.Num(), 0);

	Points[Choices[1].PointIndex].Fog = 0.f;
	SolidForage::CollectReadyToSeed(Points, Choices, Taken, Seeded, Ready);
	TestEqual(TEXT("the other pile seeds when its own fog falls"), Ready.Num(), 1);
	TestEqual(TEXT("that pile is the second choice"), Ready[0], 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidForagePickupTest,
	"SolidCore1.Forage.Pickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidForagePickupTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("4 m is in reach"), SolidForage::IsWithinReach(SolidForage::ReachCm));
	TestFalse(TEXT("past 4 m is out of reach"), SolidForage::IsWithinReach(SolidForage::ReachCm + 1.f));

	const FBox Cube(FVector(-20.f, -20.f, 0.f), FVector(20.f, 20.f, 40.f));
	float Distance = -1.f;
	TestTrue(TEXT("a ray meets the cube"), SolidForage::RayBoxDistance(
		FVector(0.f, -100.f, 20.f), FVector(0.f, 1.f, 0.f), Cube, SolidClickMove::MaxRayCm, Distance));
	TestTrue(TEXT("the hit is the near face"), FMath::IsNearlyEqual(Distance, 80.f, 0.01f));
	TestFalse(TEXT("a ray aimed away misses"), SolidForage::RayBoxDistance(
		FVector(0.f, -100.f, 20.f), FVector(0.f, -1.f, 0.f), Cube, SolidClickMove::MaxRayCm, Distance));

	SolidForage::FPileHit NearRayFarGround;
	NearRayFarGround.Index = 1;
	NearRayFarGround.RayDistanceCm = 100.f;
	NearRayFarGround.GroundDistanceCm = 900.f;
	SolidForage::FPileHit FarRayNearGround;
	FarRayNearGround.Index = 0;
	FarRayNearGround.RayDistanceCm = 500.f;
	FarRayNearGround.GroundDistanceCm = 100.f;

	TArray<SolidForage::FPileHit> Hits;
	Hits.Add(FarRayNearGround);
	Hits.Add(NearRayFarGround);
	int32 Choice = 99;
	SolidForage::EPointerAct Act = SolidForage::ResolvePileClick(true, false, Hits, true, Choice);
	TestTrue(TEXT("the far-along pile is not the one named"), Act == SolidForage::EPointerAct::None);
	TestEqual(TEXT("a miss leaves no choice"), Choice, INDEX_NONE);
	TestTrue(TEXT("raven sight walks on from a pile that is out of reach"),
		SolidForage::ClickStartsWalk(true, false, Act));
	TestFalse(TEXT("true sight does not walk on"),
		SolidForage::ClickStartsWalk(false, false, Act));

	SolidForage::FPileHit InReach;
	InReach.Index = 0;
	InReach.RayDistanceCm = 80.f;
	InReach.GroundDistanceCm = 200.f;
	Hits.Reset();
	Hits.Add(InReach);
	SolidForage::FPileHit Later = FarRayNearGround;
	Later.Index = 4;
	Hits.Add(Later);
	Act = SolidForage::ResolvePileClick(true, false, Hits, true, Choice);
	TestTrue(TEXT("the nearest pile in reach is taken"), Act == SolidForage::EPointerAct::Take);
	TestEqual(TEXT("that pile is the nearer hit"), Choice, 0);
	TestFalse(TEXT("a take does not start a walk"), SolidForage::ClickStartsWalk(true, false, Act));
	TestFalse(TEXT("true sight does not walk either"), SolidForage::ClickStartsWalk(false, false, Act));

	Act = SolidForage::ResolvePileClick(true, false, Hits, false, Choice);
	TestTrue(TEXT("a full pack leaves the pile"), Act == SolidForage::EPointerAct::Leave);
	TestEqual(TEXT("the full pack still names that pile"), Choice, 0);
	TestFalse(TEXT("a full pack does not start a walk"), SolidForage::ClickStartsWalk(true, false, Act));

	Act = SolidForage::ResolvePileClick(true, true, Hits, true, Choice);
	TestTrue(TEXT("a passage wins"), Act == SolidForage::EPointerAct::None);
	TestFalse(TEXT("a passage does not start a walk"), SolidForage::ClickStartsWalk(true, true, Act));

	Act = SolidForage::ResolvePileClick(false, false, Hits, true, Choice);
	TestTrue(TEXT("a hidden pointer or a menu does not take a pile"), Act == SolidForage::EPointerAct::None);
	TestEqual(TEXT("that click has no pile"), Choice, INDEX_NONE);

	FGameState State;
	const int32 Journal = State.Journal.Num();
	const SolidStorage::FContainer Chest = State.CaveChest;
	const TCHAR* Body = State.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)];
	TestEqual(TEXT("a new game has taken no piles"), State.TakenForagePiles.Num(), 0);
	TestTrue(TEXT("one stick is enough to fill the pack"), SolidForage::TakePile(State, 0, SolidItemType::IdStick));
	TestEqual(TEXT("the pickup writes no journal line"), State.Journal.Num(), Journal);
	TestTrue(TEXT("the scatter index is remembered"), State.TakenForagePiles.Contains(0));
	TestEqual(TEXT("the pack holds the stick"), FString(State.Inventory.Pack[0]), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the stack is one"), State.Inventory.PackStack[0], 1);
	TestFalse(TEXT("the same pile cannot be taken twice"), SolidForage::TakePile(State, 0, SolidItemType::IdStick));
	TestEqual(TEXT("the stack stays one"), State.Inventory.PackStack[0], 1);
	TestTrue(TEXT("a second stick joins the stack"), SolidForage::TakePile(State, 1, SolidItemType::IdStick));
	TestEqual(TEXT("the stack is two"), State.Inventory.PackStack[0], 2);
	TestEqual(TEXT("the body is untouched"), FString(State.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)]), FString(Body));
	TestTrue(TEXT("the craft slots stay empty"), State.Inventory.Craft[0] == nullptr);
	for (int32 Index = 0; Index < SolidStorage::Count; ++Index)
	{
		TestEqual(TEXT("the cave chest keeps its item"), FString(State.CaveChest.Slots[Index]), FString(Chest.Slots[Index]));
		TestEqual(TEXT("the cave chest keeps its count"), State.CaveChest.Counts[Index], Chest.Counts[Index]);
	}

	FGameState Full;
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		SolidInventory::WritePack(Full.Inventory, Index, SolidItemType::IdStick, 10);
	}
	TestFalse(TEXT("a full pack does not take the pile"), SolidForage::TakePile(Full, 3, SolidItemType::IdStick));
	TestEqual(TEXT("the pile is not remembered"), Full.TakenForagePiles.Num(), 0);
	TestEqual(TEXT("the full stack stays full"), Full.Inventory.PackStack[0], 10);

	FGameState Cleared;
	Cleared.TakenForagePiles.Add(4);
	SolidInventory::WritePack(Cleared.Inventory, 0, SolidItemType::IdPitch, 2);
	const int32 ClearedJournal = Cleared.Journal.Num();
	SolidEvents::FResult Result;
	SolidEvents::ApplyChange(Cleared, SolidEvents::Change::ClearInventory(), Result);
	TestTrue(TEXT("closing the monster message leaves the pile taken"), Cleared.TakenForagePiles.Contains(4));
	TestEqual(TEXT("that clear writes no journal line"), Cleared.Journal.Num(), ClearedJournal);
	TestTrue(TEXT("the pack no longer holds the pitch"), Cleared.Inventory.Pack[0] == nullptr);
	TestEqual(
		TEXT("the body is back to rags"),
		FString(Cleared.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)]),
		FString(SolidItemType::IdRags));

	FSolidTravelSession Session;
	Session.Configuration.TakenForagePiles.Add(5);
	Session.Configuration.Inventory = State.Inventory;
	const FGameState Restored = Session.Configuration;
	TestTrue(TEXT("the trip remembers the taken index"), Restored.TakenForagePiles.Contains(5));
	TestEqual(TEXT("the trip keeps the pack"), FString(Restored.Inventory.Pack[0]), FString(SolidItemType::IdStick));

	ASolidForagePile* Pile = NewObject<ASolidForagePile>();
	TestNotNull(TEXT("pile"), Pile);
	if (!Pile)
	{
		return false;
	}
	Pile->SetChoice(0, SolidItemType::IdStick);
	Pile->BuildVisuals();
	const UStaticMeshComponent* Mesh = Pile->GetPileMesh();
	TestNotNull(TEXT("pile mesh"), Mesh);
	if (!Mesh)
	{
		return false;
	}
	TestTrue(TEXT("the cube is a query hit"), Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("the cube ignores the captain"), Mesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	TestTrue(TEXT("the cube answers a visibility query"), Mesh->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
	UStaticMesh* LoadedMesh = Mesh->GetStaticMesh();
	UStaticMesh* ExpectedCube = LoadObject<UStaticMesh>(nullptr, SolidForage::CubeMesh);
	TestNotNull(TEXT("cube mesh"), LoadedMesh);
	if (LoadedMesh)
	{
		TestTrue(TEXT("the mesh is the engine cube"), LoadedMesh == ExpectedCube);
		TestTrue(TEXT("the cube is about 40 cm"), FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X, SolidForage::CubeCm / 100.f, 0.01f));
		TestTrue(TEXT("the cube sits on the ground"), FMath::IsNearlyEqual(Mesh->GetRelativeLocation().Z, SolidForage::CubeCm * 0.5f, 0.01f));
	}

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (HUD)
	{
		HUD->SetForageTip(TEXT("Stick"));
		TestEqual(TEXT("the hud keeps the pile name"), HUD->GetForageTip(), FString(TEXT("Stick")));
		HUD->SetForageTip(FString());
		TestTrue(TEXT("an empty pile tip clears"), HUD->GetForageTip().IsEmpty());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
