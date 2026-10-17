#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonEntrance.h"
#include "SolidStronghold.h"
#include "SolidStrongholds.h"
#include "SolidWorldMap.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString MountainGrid()
	{
		FString Text;
		Text.Reserve((USolidWorldMap::MapWidth + 1) * USolidWorldMap::MapHeight);
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				Text.AppendChar(TEXT('M'));
			}
			Text.AppendChar(TEXT('\n'));
		}
		return Text;
	}

	void SetCell(FString& Grid, const int32 Col, const int32 Row, const TCHAR Marker)
	{
		Grid[Row * (USolidWorldMap::MapWidth + 1) + Col] = Marker;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdCatalogTest,
	"SolidCore1.Stronghold.Catalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdCatalogTest::RunTest(const FString& Parameters)
{
	const TCHAR* TypeNames[] = { TEXT("Dungeon"), TEXT("Fortress"), TEXT("Tower") };
	TestEqual(TEXT("three types"), SolidStronghold::TypeCount, 3);
	for (int32 Index = 0; Index < SolidStronghold::TypeCount; ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("type %d"), Index),
			FString(SolidStronghold::TypeName(SolidStronghold::Types[Index])),
			FString(TypeNames[Index]));
	}

	const TCHAR* SizeNames[] = {
		TEXT("Meagre"), TEXT("Humble"), TEXT("Average"), TEXT("Impressive"),
		TEXT("Grand"), TEXT("Stately"), TEXT("Imperious") };
	TestEqual(TEXT("seven sizes"), SolidStronghold::SizeCount, 7);
	for (int32 Index = 0; Index < SolidStronghold::SizeCount; ++Index)
	{
		TestEqual(
			*FString::Printf(TEXT("size %d"), Index),
			FString(SolidStronghold::SizeName(SolidStronghold::Sizes[Index])),
			FString(SizeNames[Index]));
	}

	TestEqual(
		TEXT("starting type"),
		static_cast<uint8>(SolidStronghold::Starting.Type),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(
		TEXT("starting size"),
		static_cast<uint8>(SolidStronghold::Starting.Size),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(
		TEXT("starting label"),
		SolidStronghold::Label(SolidStronghold::Starting),
		FString(TEXT("Meagre Dungeon")));

	const SolidStronghold::FVisitView Cave = SolidStronghold::VisitViewFor(SolidStronghold::Starting);
	TestTrue(TEXT("entering the cave levels the camera"), FMath::IsNearlyZero(Cave.Enter.PitchDeg, 0.1f));
	TestTrue(TEXT("entering the cave looks in"), FMath::IsNearlyZero(Cave.Enter.YawDeg, 0.1f));
	TestTrue(TEXT("leaving the cave levels the camera"), FMath::IsNearlyZero(Cave.Leave.PitchDeg, 0.1f));
	TestTrue(TEXT("leaving the cave looks out"), FMath::IsNearlyZero(Cave.Leave.YawDeg, 0.1f));
	TestTrue(
		TEXT("a north-facing mouth leaves looking north"),
		FMath::IsNearlyEqual(SolidStronghold::LeaveYawDeg(90.f, Cave.Leave), 90.f, 0.1f));

	FSolidStronghold Fortress;
	Fortress.Type = ESolidStrongholdType::Fortress;
	Fortress.Size = ESolidStrongholdSize::Meagre;
	const SolidStronghold::FVisitView Unlisted = SolidStronghold::VisitViewFor(Fortress);
	TestTrue(TEXT("an unlisted stronghold still arrives level"), FMath::IsNearlyZero(Unlisted.Enter.PitchDeg, 0.1f));
	TestTrue(TEXT("an unlisted stronghold still leaves level"), FMath::IsNearlyZero(Unlisted.Leave.PitchDeg, 0.1f));

	FSolidStronghold FromToken;
	TestTrue(TEXT("MeagreDungeon resolves"), SolidStronghold::FromToken(TEXT("MeagreDungeon"), FromToken));
	TestEqual(
		TEXT("the token is a Dungeon"),
		static_cast<uint8>(FromToken.Type),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(
		TEXT("the token is Meagre"),
		static_cast<uint8>(FromToken.Size),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestFalse(TEXT("M does not resolve"), SolidStronghold::FromToken(TEXT("M"), FromToken));

	ESolidBiome Shelf = ESolidBiome::Grassland;
	TestTrue(TEXT("the cave names a shelf"), SolidStronghold::BiomeFor(SolidStronghold::Starting, Shelf));
	TestEqual(TEXT("the cave shelf is mountain"), static_cast<uint8>(Shelf), static_cast<uint8>(ESolidBiome::Mountain));
	TestTrue(TEXT("the cave uses the cliff mouth"), SolidStronghold::UsesCaveMouth(SolidStronghold::Starting));

	FSolidStronghold Imperious;
	TestTrue(TEXT("ImperiousTower resolves"), SolidStronghold::FromToken(TEXT("ImperiousTower"), Imperious));
	TestEqual(
		TEXT("Imperious Tower type"),
		static_cast<uint8>(Imperious.Type),
		static_cast<uint8>(ESolidStrongholdType::Tower));
	TestEqual(
		TEXT("Imperious Tower size"),
		static_cast<uint8>(Imperious.Size),
		static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestTrue(TEXT("the tower names a shelf"), SolidStronghold::BiomeFor(Imperious, Shelf));
	TestEqual(TEXT("the tower shelf is grassland"), static_cast<uint8>(Shelf), static_cast<uint8>(ESolidBiome::Grassland));
	TestFalse(TEXT("the tower does not use the cliff mouth"), SolidStronghold::UsesCaveMouth(Imperious));

	FSolidStronghold HumbleTower;
	TestTrue(TEXT("Humble Tower resolves"), SolidStronghold::FromToken(TEXT("Humble Tower"), HumbleTower));
	TestFalse(TEXT("an unlisted stronghold names no shelf"), SolidStronghold::BiomeFor(HumbleTower, Shelf));
	TestEqual(
		TEXT("a failed shelf lookup leaves the previous biome"),
		static_cast<uint8>(Shelf),
		static_cast<uint8>(ESolidBiome::Grassland));

	TArray<SolidStrongholds::FEntrance> Mouths;
	SolidStrongholds::FEntrance TowerMouth;
	TowerMouth.Stronghold = Imperious;
	TowerMouth.WorldXY = FVector2D(10.f, 10.f);
	Mouths.Add(TowerMouth);
	SolidStrongholds::FEntrance CaveMouth;
	CaveMouth.Stronghold = SolidStronghold::Starting;
	CaveMouth.WorldXY = FVector2D(40.f, 80.f);
	Mouths.Add(CaveMouth);
	const SolidStrongholds::FEntrance* Chosen = SolidStrongholds::FindCaveMouth(Mouths);
	TestTrue(TEXT("leaving the cave skips a tower mouth"), Chosen != nullptr);
	if (Chosen)
	{
		TestTrue(TEXT("the cave mouth is the dungeon"), SolidStronghold::UsesCaveMouth(Chosen->Stronghold));
		TestTrue(TEXT("the cave mouth is the later entrance"), Chosen->WorldXY.Equals(CaveMouth.WorldXY, 0.1f));
	}
	Mouths.Reset();
	Mouths.Add(TowerMouth);
	TestTrue(TEXT("a tower alone is not a cave mouth"), SolidStrongholds::FindCaveMouth(Mouths) == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdShippedEntranceTest,
	"SolidCore1.Stronghold.ShippedEntrance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdShippedEntranceTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("world map"), Map);
	TestTrue(TEXT("LoadDefault"), Map->LoadDefault());

	TArray<FIntPoint> CaveCells;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			FSolidStronghold Named;
			if (!Map->StrongholdAtCell(Col, Row, Named))
			{
				continue;
			}
			if (Named.Type != ESolidStrongholdType::Dungeon || Named.Size != ESolidStrongholdSize::Meagre)
			{
				ESolidBiome Expected = ESolidBiome::Grassland;
				TestTrue(TEXT("a shipped stronghold names its shelf"), SolidStronghold::BiomeFor(Named, Expected));
				TestEqual(
					TEXT("a shipped stronghold uses its shelf"),
					static_cast<uint8>(Map->GetBiomeAtCell(Col, Row)),
					static_cast<uint8>(Expected));
				continue;
			}
			CaveCells.Add(FIntPoint(Col, Row));
			TestEqual(
				TEXT("cave cell is Mountain"),
				static_cast<uint8>(Map->GetBiomeAtCell(Col, Row)),
				static_cast<uint8>(ESolidBiome::Mountain));
			TestEqual(TEXT("cave is not a town"), Map->GetLocationIdAtCell(Col, Row), INDEX_NONE);
			const FIntPoint North(Col, Row - 1);
			TestEqual(
				TEXT("grass marker north of the cave"),
				static_cast<int32>(Map->GetMarkerAtCell(North.X, North.Y)),
				static_cast<int32>(USolidWorldMap::GrassMarker));
			TestEqual(
				TEXT("north neighbor is grassland"),
				static_cast<uint8>(Map->GetBiomeAtCell(North.X, North.Y)),
				static_cast<uint8>(ESolidBiome::Grassland));
		}
	}
	TestTrue(TEXT("the shipped map has a cave"), CaveCells.Num() > 0);

	const FVector2D WorldMin(-51200.f, -51200.f);
	const FVector2D WorldMax(51200.f, 51200.f);
	TArray<SolidStrongholds::FEntrance> Entrances;
	SolidStrongholds::CollectEntrances(Map, WorldMin, WorldMax, Entrances);
	int32 CaveMouths = 0;
	for (const SolidStrongholds::FEntrance& Entrance : Entrances)
	{
		if (!SolidStronghold::UsesCaveMouth(Entrance.Stronghold))
		{
			continue;
		}
		++CaveMouths;
		TestEqual(
			TEXT("entrance type"),
			static_cast<uint8>(Entrance.Stronghold.Type),
			static_cast<uint8>(ESolidStrongholdType::Dungeon));
		TestEqual(
			TEXT("entrance size"),
			static_cast<uint8>(Entrance.Stronghold.Size),
			static_cast<uint8>(ESolidStrongholdSize::Meagre));
		TestTrue(TEXT("entrance is a cave cell"), CaveCells.Contains(Entrance.Cell));
		const FIntPoint North(Entrance.Cell.X, Entrance.Cell.Y - 1);
		TestEqual(TEXT("grass column"), Entrance.GrassCell.X, North.X);
		TestEqual(TEXT("grass row"), Entrance.GrassCell.Y, North.Y);
		TestTrue(TEXT("yaw faces north"), FMath::IsNearlyEqual(Entrance.FacingYawDeg, 90.f, 0.1f));

		FVector2D CaveXY = FVector2D::ZeroVector;
		FVector2D GrassXY = FVector2D::ZeroVector;
		TestTrue(TEXT("cave projects"), Map->CellToWorldXY(Entrance.Cell, WorldMin, WorldMax, CaveXY));
		TestTrue(TEXT("grass projects"), Map->CellToWorldXY(North, WorldMin, WorldMax, GrassXY));
		const FVector2D Edge = (CaveXY + GrassXY) * 0.5f;
		TestTrue(TEXT("edge is the shared boundary"), Entrance.EdgeXY.Equals(Edge, 0.1f));
		TestTrue(TEXT("spawn is north of the edge"), Entrance.WorldXY.Y > Entrance.EdgeXY.Y);
		TestTrue(
			TEXT("spawn inset matches the grass-side offset"),
			FMath::IsNearlyEqual(FVector2D::Distance(Entrance.WorldXY, Entrance.EdgeXY), SolidStrongholds::FaceInsetCm, 0.1f));
	}
	TestEqual(TEXT("one cliff mouth per cave cell"), CaveMouths, CaveCells.Num());

	TestEqual(TEXT("null map yields none"), SolidStrongholds::CollectEntrances(nullptr, WorldMin, WorldMax, Entrances), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdFaceChoiceTest,
	"SolidCore1.Stronghold.FaceChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdFaceChoiceTest::RunTest(const FString& Parameters)
{
	const FVector2D WorldMin(0.f, 0.f);
	const FVector2D WorldMax(6400.f, 6400.f);

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, USolidWorldMap::LocationMarker);
		SetCell(Grid, 9, 10, USolidWorldMap::GrassMarker);
		Grid += TEXT("! 11,11 = Stronghold (MeagreDungeon)\n");
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("east grass loads"), Map->LoadFromString(Grid));
		TestEqual(
			TEXT("a stronghold is Mountain"),
			static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
			static_cast<uint8>(ESolidBiome::Mountain));

		TArray<SolidStrongholds::FEntrance> Entrances;
		TestEqual(TEXT("east face"), SolidStrongholds::CollectEntrances(Map, WorldMin, WorldMax, Entrances), 1);
		if (Entrances.Num() == 1)
		{
			TestEqual(TEXT("grass column is east"), Entrances[0].GrassCell.X, 9);
			TestEqual(TEXT("grass row"), Entrances[0].GrassCell.Y, 10);
			TestTrue(TEXT("yaw faces east"), FMath::IsNearlyEqual(Entrances[0].FacingYawDeg, 0.f, 0.1f));
			TestTrue(TEXT("spawn is east of the edge"), Entrances[0].WorldXY.X > Entrances[0].EdgeXY.X);
		}
	}

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, TEXT('Q'));
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("unknown letter loads"), Map->LoadFromString(Grid));
		TestEqual(
			TEXT("Q is still grassland"),
			static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
			static_cast<uint8>(ESolidBiome::Grassland));
	}

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, USolidWorldMap::LocationMarker);
		Grid += TEXT("! 11,11 = Stronghold (MeagreDungeon)\n");
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("sealed cave loads"), Map->LoadFromString(Grid));
		TArray<SolidStrongholds::FEntrance> Entrances;
		TestEqual(TEXT("no grass face means no mouth"), SolidStrongholds::CollectEntrances(Map, WorldMin, WorldMax, Entrances), 0);
	}

	{
		FString Grid = MountainGrid();
		SetCell(Grid, 10, 10, USolidWorldMap::LocationMarker);
		SetCell(Grid, 10, 9, USolidWorldMap::GrassMarker);
		SetCell(Grid, 9, 10, USolidWorldMap::GrassMarker);
		Grid += TEXT("! 11,11 = Stronghold (MeagreDungeon)\n");
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("two grass faces load"), Map->LoadFromString(Grid));
		TArray<SolidStrongholds::FEntrance> Entrances;
		TestEqual(TEXT("one mouth"), SolidStrongholds::CollectEntrances(Map, WorldMin, WorldMax, Entrances), 1);
		if (Entrances.Num() == 1)
		{
			TestEqual(TEXT("north column wins"), Entrances[0].GrassCell.X, 10);
			TestEqual(TEXT("north row wins"), Entrances[0].GrassCell.Y, 9);
			TestTrue(TEXT("yaw faces north"), FMath::IsNearlyEqual(Entrances[0].FacingYawDeg, 90.f, 0.1f));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonEntranceVisualsTest,
	"SolidCore1.Stronghold.DungeonEntranceVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonEntranceVisualsTest::RunTest(const FString& Parameters)
{
	ASolidDungeonEntrance* Entrance = NewObject<ASolidDungeonEntrance>();
	TestNotNull(TEXT("entrance"), Entrance);
	Entrance->BuildVisuals();

	UStaticMeshComponent* Mouth = Entrance->GetMouthMesh();
	UStaticMeshComponent* Left = Entrance->GetLeftJamb();
	UStaticMeshComponent* Right = Entrance->GetRightJamb();
	UStaticMeshComponent* Lintel = Entrance->GetLintel();
	TestNotNull(TEXT("mouth"), Mouth);
	TestNotNull(TEXT("left jamb"), Left);
	TestNotNull(TEXT("right jamb"), Right);
	TestNotNull(TEXT("lintel"), Lintel);
	if (!Mouth || !Left || !Right || !Lintel)
	{
		return false;
	}

	UStaticMesh* MouthMesh = Mouth->GetStaticMesh();
	UStaticMesh* LeftMesh = Left->GetStaticMesh();
	UStaticMesh* LintelMesh = Lintel->GetStaticMesh();
	TestNotNull(TEXT("mouth cube"), MouthMesh);
	TestNotNull(TEXT("jamb cube"), LeftMesh);
	TestNotNull(TEXT("lintel cube"), LintelMesh);

	TestTrue(TEXT("mouth is wider than it is deep"), Mouth->GetRelativeScale3D().Y > Mouth->GetRelativeScale3D().X);
	TestTrue(TEXT("jambs are thinner than the mouth"), Left->GetRelativeScale3D().Y < Mouth->GetRelativeScale3D().Y);
	TestTrue(TEXT("left jamb is on -Y"), Left->GetRelativeLocation().Y < 0.f);
	TestTrue(TEXT("right jamb is on +Y"), Right->GetRelativeLocation().Y > 0.f);
	TestTrue(TEXT("lintel sits above the mouth"), Lintel->GetRelativeLocation().Z > Mouth->GetRelativeLocation().Z);
	TestTrue(TEXT("mouth meets the ground"), FMath::IsNearlyEqual(
		Mouth->GetRelativeLocation().Z - Mouth->GetRelativeScale3D().Z * 50.f, 0.f, 1.f));
	const float MouthFront = Mouth->GetRelativeLocation().X + Mouth->GetRelativeScale3D().X * 50.f;
	const float LintelFront = Lintel->GetRelativeLocation().X + Lintel->GetRelativeScale3D().X * 50.f;
	TestTrue(TEXT("mouth is recessed behind the lintel front"), MouthFront < LintelFront - 1.f);
	TestTrue(TEXT("the grass face of the frame sits toward the cliff"),
		FMath::IsNearlyEqual(LintelFront, SolidStrongholds::FrameFrontLocalX, 1.f));
	TestTrue(TEXT("the grass spawn stands in front of the mouth"), MouthFront < -40.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
