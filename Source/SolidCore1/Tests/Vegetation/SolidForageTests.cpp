#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HUD/SolidHUD.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "SolidClickMove.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "SolidForestTrees.h"
#include "SolidForage.h"
#include "SolidForagePile.h"
#include "SolidGameInstance.h"
#include "SolidGameState.h"
#include "Company/SolidCaptain.h"
#include "SolidTravelSession.h"
#include "SolidTerrainMap.h"
#include "Terrain/SolidTerrainTestHelpers.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

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

	TestTrue(TEXT("the open world has piles"), SolidForage::WorldHasPiles(false));
	TestFalse(TEXT("a closed visit has no piles"), SolidForage::WorldHasPiles(true));

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

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	const int32 Journal = State.GetJournal()->Entries.Num();
	const FString Body = State.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)].Id();
	TestEqual(TEXT("a new game has taken no piles"), State.TakenForagePiles.Num(), 0);
	TestTrue(TEXT("one stick is enough to fill the pack"), SolidForage::TakePile(State, 0, SolidItemType::IdStick));
	TestEqual(TEXT("the pickup writes no journal line"), State.GetJournal()->Entries.Num(), Journal);
	TestTrue(TEXT("the scatter index is remembered"), State.TakenForagePiles.Contains(0));
	TestEqual(TEXT("the pack holds the stick"), FString(State.Inventory.Pack[0].Id()), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the stack is one"), State.Inventory.Pack[0].Count, 1);
	TestFalse(TEXT("the same pile cannot be taken twice"), SolidForage::TakePile(State, 0, SolidItemType::IdStick));
	TestEqual(TEXT("the stack stays one"), State.Inventory.Pack[0].Count, 1);
	TestTrue(TEXT("a second stick joins the stack"), SolidForage::TakePile(State, 1, SolidItemType::IdStick));
	TestEqual(TEXT("the stack is two"), State.Inventory.Pack[0].Count, 2);
	TestEqual(TEXT("the body is untouched"), FString(State.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)].Id()), FString(Body));
	TestTrue(TEXT("the craft slots stay empty"), State.Inventory.Craft[0].IsEmpty());

	USolidGameState& Full = *NewObject<USolidGameState>();
	USolidCaptain& FullCaptain = *NewObject<USolidCaptain>();
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		SolidInventory::WritePack(Full.Inventory, Index, SolidItemType::IdStick, 10);
	}
	TestFalse(TEXT("a full pack does not take the pile"), SolidForage::TakePile(Full, 3, SolidItemType::IdStick));
	TestEqual(TEXT("the pile is not remembered"), Full.TakenForagePiles.Num(), 0);
	TestEqual(TEXT("the full stack stays full"), Full.Inventory.Pack[0].Count, 10);

	USolidGameState& Cleared = *NewObject<USolidGameState>();
	USolidCaptain& ClearedCaptain = *NewObject<USolidCaptain>();
	Cleared.TakenForagePiles.Add(4);
	SolidInventory::WritePack(Cleared.Inventory, 0, SolidItemType::IdPitch, 2);
	const int32 ClearedJournal = Cleared.GetJournal()->Entries.Num();
	SolidEvents::FResult Result;
	SolidEvents::ApplyChange(Cleared, ClearedCaptain, SolidEvents::Change::ClearInventory(), Result);
	TestTrue(TEXT("closing the monster message leaves the pile taken"), Cleared.TakenForagePiles.Contains(4));
	TestEqual(TEXT("that clear writes no journal line"), Cleared.GetJournal()->Entries.Num(), ClearedJournal);
	TestTrue(TEXT("the pack no longer holds the pitch"), Cleared.Inventory.Pack[0].IsEmpty());
	TestEqual(
		TEXT("the body is back to rags"),
		FString(Cleared.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)].Id()),
		FString(SolidItemType::IdRags));

	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	TestNotNull(TEXT("game instance"), Instance);
	if (!Instance)
	{
		return false;
	}
	USolidGameState* Kept = Instance->GetConfiguration();
	Kept->TakenForagePiles.Add(5);
	Kept->Inventory = State.Inventory;
	Instance->Capture(FSolidTravelSession());
	const USolidGameState* Restored = Instance->GetConfiguration();
	TestTrue(TEXT("the trip is the same play state"), Restored == Kept);
	TestTrue(TEXT("the trip remembers the taken index"), Restored->TakenForagePiles.Contains(5));
	TestEqual(TEXT("the trip keeps the pack"), FString(Restored->Inventory.Pack[0].Id()), FString(SolidItemType::IdStick));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidForageIglinTest,
	"SolidCore1.Forage.Iglin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidForageIglinTest::RunTest(const FString& Parameters)
{
	TArray<FSolidTerrainPoint> OnePoint;
	OnePoint.Add(FSolidTerrainPoint());
	TArray<int32> NoTrees;
	TArray<SolidForage::FChoice> EmptyChoices;
	const FVector2D MinXY = FVector2D::ZeroVector;
	const FVector2D MaxXY(102400.f, 102400.f);
	TestEqual(TEXT("a missing map adds nothing"),
		SolidForage::GuaranteeForestCells(
			OnePoint, nullptr, MinXY, MaxXY, FVector2D::ZeroVector, SolidForage::IglinRadiusCm, NoTrees, EmptyChoices),
		0);
	TestEqual(TEXT("a missing map leaves the list empty"), EmptyChoices.Num(), 0);

	USolidWorldChart* Unloaded = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("unloaded map"), Unloaded);
	if (Unloaded)
	{
		TestEqual(TEXT("an unloaded map adds nothing"),
			SolidForage::GuaranteeForestCells(
				OnePoint, Unloaded, MinXY, MaxXY, FVector2D::ZeroVector, SolidForage::IglinRadiusCm, NoTrees, EmptyChoices),
			0);
	}

	USolidTerrainMap* Map = NewObject<USolidTerrainMap>();
	TestNotNull(TEXT("map"), Map);
	if (!Map)
	{
		return false;
	}
	Map->Build(1337, 0.00012f, 3000.f, 0.f, 513, 513, 200.f, true);
	TestTrue(TEXT("the world is built"), Map->IsBuilt());

	const USolidWorldChart* WorldMap = Map->GetWorldMap();
	TestNotNull(TEXT("world map"), WorldMap);
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return false;
	}

	const FVector2D WorldMin = Map->GetWorldMinXY();
	const FVector2D WorldMax = Map->GetWorldMaxXY();
	FVector2D IglinXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Iglin has a world position"),
		WorldMap->GetLocationWorldXY(SolidTowns::Iglin, WorldMin, WorldMax, IglinXY));

	const TArray<FSolidTerrainPoint>& Points = Map->GetPoints();
	TestEqual(TEXT("a zero radius adds nothing"),
		SolidForage::GuaranteeForestCells(
			Points, WorldMap, WorldMin, WorldMax, IglinXY, 0.f, NoTrees, EmptyChoices),
		0);

	SolidForestTrees::FScatterParams TreeParams;
	TreeParams.Density = 0.15f;
	TreeParams.MaxTrees = 256;
	TreeParams.JitterCm = 90.f;
	TArray<int32> TreePoints;
	SolidForestTrees::CollectChosenPointIndices(Points, 42, TreeParams, TreePoints);
	const TSet<int32> TreeSet(TreePoints);

	auto CellKey = [](const FIntPoint& Cell) -> int32
	{
		return Cell.Y * USolidWorldChart::MapWidth + Cell.X;
	};

	TArray<int32> NearCells;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (WorldMap->GetMarkerAtCell(Col, Row) != USolidWorldChart::ForestMarker)
			{
				continue;
			}
			FVector2D Center = FVector2D::ZeroVector;
			if (!WorldMap->CellToWorldXY(FIntPoint(Col, Row), WorldMin, WorldMax, Center))
			{
				continue;
			}
			if (FVector2D::Distance(Center, IglinXY) <= SolidForage::IglinRadiusCm)
			{
				NearCells.Add(CellKey(FIntPoint(Col, Row)));
			}
		}
	}

	TArray<SolidForage::FChoice> Choices;
	const int32 Added = SolidForage::GuaranteeForestCells(
		Points, WorldMap, WorldMin, WorldMax, IglinXY, SolidForage::IglinRadiusCm, TreePoints, Choices);
	TestEqual(TEXT("each nearby forest cell gains a stick and a pitch"), Added, NearCells.Num() * 2);
	TestEqual(TEXT("a second pass adds nothing"),
		SolidForage::GuaranteeForestCells(
			Points, WorldMap, WorldMin, WorldMax, IglinXY, SolidForage::IglinRadiusCm, TreePoints, Choices),
		0);

	TMap<int32, int32> Sticks;
	TMap<int32, int32> Pitches;
	TSet<int32> UsedPoints;
	for (const SolidForage::FChoice& Choice : Choices)
	{
		TestTrue(TEXT("the pile sits on a grid point"), Points.IsValidIndex(Choice.PointIndex));
		if (!Points.IsValidIndex(Choice.PointIndex))
		{
			continue;
		}
		const FSolidTerrainPoint& Point = Points[Choice.PointIndex];
		TestTrue(TEXT("that point is forest"), Point.Biome == ESolidBiome::Forest);
		TestFalse(TEXT("a tree point holds no pile"), TreeSet.Contains(Choice.PointIndex));
		TestFalse(TEXT("each point holds one pile"), UsedPoints.Contains(Choice.PointIndex));
		UsedPoints.Add(Choice.PointIndex);
		TestTrue(TEXT("the guaranteed pile is not jittered"),
			Choice.XY.Equals(FVector2D(Point.X, Point.Y), 0.01f));

		FIntPoint Cell = FIntPoint::ZeroValue;
		TestTrue(TEXT("the point lies on the world map"),
			WorldMap->CellAtWorld(Point.X, Point.Y, WorldMin, WorldMax, Cell));
		TestEqual(TEXT("the cell is forest on the world map"),
			FString::Chr(WorldMap->GetMarkerAtCell(Cell.X, Cell.Y)),
			FString::Chr(USolidWorldChart::ForestMarker));
		FVector2D Center = FVector2D::ZeroVector;
		TestTrue(TEXT("the cell has a center"),
			WorldMap->CellToWorldXY(Cell, WorldMin, WorldMax, Center));
		TestTrue(TEXT("the cell is within 100 m of Iglin"),
			FVector2D::Distance(Center, IglinXY) <= SolidForage::IglinRadiusCm);

		const int32 Key = CellKey(Cell);
		if (Choice.ItemId && FCString::Strcmp(Choice.ItemId, SolidItemType::IdStick) == 0)
		{
			Sticks.FindOrAdd(Key) += 1;
		}
		else if (Choice.ItemId && FCString::Strcmp(Choice.ItemId, SolidItemType::IdPitch) == 0)
		{
			Pitches.FindOrAdd(Key) += 1;
		}
		else
		{
			AddError(TEXT("a guaranteed pile is a stick or a pitch"));
		}
	}

	int32 StickPoint = INDEX_NONE;
	int32 StickCell = INDEX_NONE;
	for (const int32 Key : NearCells)
	{
		TestEqual(TEXT("the cell has one stick"), Sticks.FindRef(Key), 1);
		TestEqual(TEXT("the cell has one pitch"), Pitches.FindRef(Key), 1);
		if (StickPoint == INDEX_NONE)
		{
			for (const SolidForage::FChoice& Choice : Choices)
			{
				if (!Points.IsValidIndex(Choice.PointIndex))
				{
					continue;
				}
				FIntPoint Cell = FIntPoint::ZeroValue;
				if (!WorldMap->CellAtWorld(Points[Choice.PointIndex].X, Points[Choice.PointIndex].Y, WorldMin, WorldMax, Cell))
				{
					continue;
				}
				if (CellKey(Cell) != Key || !Choice.ItemId || FCString::Strcmp(Choice.ItemId, SolidItemType::IdStick) != 0)
				{
					continue;
				}
				StickPoint = Choice.PointIndex;
				StickCell = Key;
				break;
			}
		}
	}

	SolidForage::FScatterParams PileParams;
	TArray<SolidForage::FChoice> Scattered;
	TestEqual(TEXT("the scatter still stops at 64"),
		SolidForage::CollectChoices(Points, TreePoints, SolidForage::Seed, PileParams, Scattered),
		SolidForage::MaxPiles);
	TArray<SolidForage::FChoice> Combined = Scattered;
	const int32 Extra = SolidForage::GuaranteeForestCells(
		Points, WorldMap, WorldMin, WorldMax, IglinXY, SolidForage::IglinRadiusCm, TreePoints, Combined);
	TestEqual(TEXT("guarantees append after the scatter"), Combined.Num(), Scattered.Num() + Extra);
	TestTrue(TEXT("the 64 cap does not limit the nearby forest"), Combined.Num() > SolidForage::MaxPiles);
	for (int32 Index = 0; Index < Scattered.Num(); ++Index)
	{
		TestEqual(TEXT("the scatter index stays put"), Combined[Index].PointIndex, Scattered[Index].PointIndex);
		TestEqual(TEXT("the scatter item stays put"), FString(Combined[Index].ItemId), FString(Scattered[Index].ItemId));
		TestTrue(TEXT("the scatter position stays put"), Combined[Index].XY.Equals(Scattered[Index].XY, 0.01f));
	}

	TSet<int32> CombinedPoints;
	TMap<int32, int32> CombinedSticks;
	TMap<int32, int32> CombinedPitches;
	for (const SolidForage::FChoice& Choice : Combined)
	{
		TestTrue(TEXT("a combined pile sits on a grid point"), Points.IsValidIndex(Choice.PointIndex));
		if (!Points.IsValidIndex(Choice.PointIndex))
		{
			continue;
		}
		TestFalse(TEXT("a combined pile is not on a tree"), TreeSet.Contains(Choice.PointIndex));
		TestFalse(TEXT("a combined point holds one pile"), CombinedPoints.Contains(Choice.PointIndex));
		CombinedPoints.Add(Choice.PointIndex);
		TestTrue(TEXT("a combined pile is still forest"), Points[Choice.PointIndex].Biome == ESolidBiome::Forest);

		FIntPoint Cell = FIntPoint::ZeroValue;
		if (!WorldMap->CellAtWorld(Points[Choice.PointIndex].X, Points[Choice.PointIndex].Y, WorldMin, WorldMax, Cell))
		{
			continue;
		}
		const int32 Key = CellKey(Cell);
		if (Choice.ItemId && FCString::Strcmp(Choice.ItemId, SolidItemType::IdStick) == 0)
		{
			CombinedSticks.FindOrAdd(Key) += 1;
		}
		else if (Choice.ItemId && FCString::Strcmp(Choice.ItemId, SolidItemType::IdPitch) == 0)
		{
			CombinedPitches.FindOrAdd(Key) += 1;
		}
	}
	for (const int32 Key : NearCells)
	{
		TestTrue(TEXT("the scatter plus the guarantee leaves a stick"), CombinedSticks.FindRef(Key) >= 1);
		TestTrue(TEXT("the scatter plus the guarantee leaves a pitch"), CombinedPitches.FindRef(Key) >= 1);
	}

	TestTrue(TEXT("a nearby cell can already hold a stick"), StickPoint != INDEX_NONE && Points.IsValidIndex(StickPoint));
	if (Points.IsValidIndex(StickPoint))
	{
		TArray<SolidForage::FChoice> Partial;
		SolidForage::FChoice Kept;
		Kept.PointIndex = StickPoint;
		Kept.XY = FVector2D(Points[StickPoint].X, Points[StickPoint].Y);
		Kept.ItemId = SolidItemType::IdStick;
		Partial.Add(Kept);
		const int32 PartialAdded = SolidForage::GuaranteeForestCells(
			Points, WorldMap, WorldMin, WorldMax, IglinXY, SolidForage::IglinRadiusCm, TreePoints, Partial);
		TestEqual(TEXT("a cell that already has a stick gains only the pitch"), PartialAdded, Added - 1);

		int32 KeptSticks = 0;
		int32 KeptPitches = 0;
		int32 PitchPoint = INDEX_NONE;
		for (const SolidForage::FChoice& Choice : Partial)
		{
			if (!Points.IsValidIndex(Choice.PointIndex))
			{
				continue;
			}
			FIntPoint Cell = FIntPoint::ZeroValue;
			if (!WorldMap->CellAtWorld(Points[Choice.PointIndex].X, Points[Choice.PointIndex].Y, WorldMin, WorldMax, Cell))
			{
				continue;
			}
			if (CellKey(Cell) != StickCell)
			{
				continue;
			}
			if (Choice.ItemId && FCString::Strcmp(Choice.ItemId, SolidItemType::IdStick) == 0)
			{
				++KeptSticks;
				TestEqual(TEXT("the existing stick stays on its point"), Choice.PointIndex, StickPoint);
			}
			else if (Choice.ItemId && FCString::Strcmp(Choice.ItemId, SolidItemType::IdPitch) == 0)
			{
				++KeptPitches;
				PitchPoint = Choice.PointIndex;
				TestTrue(TEXT("the added pitch stays on its point"),
					Choice.XY.Equals(FVector2D(Points[Choice.PointIndex].X, Points[Choice.PointIndex].Y), 0.01f));
			}
		}
		TestEqual(TEXT("that cell still has one stick"), KeptSticks, 1);
		TestEqual(TEXT("that cell gains one pitch"), KeptPitches, 1);
		TestTrue(TEXT("the pitch uses another point"), PitchPoint != INDEX_NONE && PitchPoint != StickPoint);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
