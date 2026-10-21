#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidEvents.h"
#include "Inventory/SolidInventory.h"
#include "Lairs/SolidLair.h"
#include "Lairs/SolidLairInterior.h"
#include "Lairs/SolidLairEntrance.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "SolidGameState.h"
#include "Squad/SolidCaptain.h"
#include "SolidStronghold.h"
#include "SolidWorldMap.h"
#include "Towns/SolidTowns.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/UObjectIterator.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidLairTestPrivate
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
	FSolidLairMazeTest,
	"SolidCore1.Lair.Maze",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairMazeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("lair name"), FString(SolidLair::IglinTunnels.Name), FString(TEXT("Iglin tunnels")));
	TestEqual(TEXT("lair id"), FString(SolidLair::IglinTunnels.Id), FString(SolidLair::IdIglinTunnels));
	const FString LairId(SolidLair::IdIglinTunnels);
	TestEqual(TEXT("lair id is eight digits"), LairId.Len(), SolidLair::IdLength);
	bool bLairIdHex = LairId.Len() == SolidLair::IdLength;
	for (const TCHAR Char : LairId)
	{
		bLairIdHex = bLairIdHex && FChar::IsHexDigit(Char);
	}
	TestTrue(TEXT("lair id is hex"), bLairIdHex);
	TestTrue(
		TEXT("find by id"),
		SolidLair::FindById(SolidLair::IdIglinTunnels) == &SolidLair::IglinTunnels);
	TestTrue(TEXT("the display name still finds it"), SolidLair::FindByName(TEXT("Iglin tunnels")) == &SolidLair::IglinTunnels);
	TestNull(TEXT("an unknown id is missing"), SolidLair::FindById(TEXT("00000000")));
	TestEqual(TEXT("enter journal"), SolidLair::VisitJournalText(true), FString(TEXT("Enter Iglin tunnels")));
	TestEqual(TEXT("exit journal"), SolidLair::VisitJournalText(false), FString(TEXT("Exit Iglin tunnels")));
	TestTrue(TEXT("the doorway is wider than the captain"), SolidLair::DoorwayWidthCm > 110.f);

	const FIntPoint X = SolidLair::CellOf(USolidWorldMap::XMarker);
	const FIntPoint Y = SolidLair::CellOf(USolidWorldMap::YMarker);
	TestTrue(TEXT("X is on the plan"),
		X.X >= 0 && X.Y >= 0 && X.X < SolidLair::MazeCols && X.Y < SolidLair::MazeRows);
	TestTrue(TEXT("Y is on the plan"),
		Y.X >= 0 && Y.Y >= 0 && Y.X < SolidLair::MazeCols && Y.Y < SolidLair::MazeRows);
	TestTrue(TEXT("the mouths are different cells"), X != Y);
	TestEqual(TEXT("X marker"), static_cast<int32>(SolidLair::MazeAt(X.X, X.Y)), static_cast<int32>(USolidWorldMap::XMarker));
	TestEqual(TEXT("Y marker"), static_cast<int32>(SolidLair::MazeAt(Y.X, Y.Y)), static_cast<int32>(USolidWorldMap::YMarker));
	TestEqual(TEXT("west of X is rock"), static_cast<int32>(SolidLair::MazeAt(X.X - 1, X.Y)), static_cast<int32>(TEXT('#')));
	TestEqual(TEXT("east of Y is rock"), static_cast<int32>(SolidLair::MazeAt(Y.X + 1, Y.Y)), static_cast<int32>(TEXT('#')));
	for (int32 Row = 0; Row < SolidLair::MazeRows; ++Row)
	{
		TestEqual(TEXT("row width"), static_cast<int32>(FCString::Strlen(SolidLair::Maze[Row])), SolidLair::MazeCols);
	}

	TestTrue(TEXT("X is open"), SolidLair::IsOpenCell(X.X, X.Y));
	TestTrue(TEXT("Y is open"), SolidLair::IsOpenCell(Y.X, Y.Y));

	TestTrue(TEXT("the X spawn is inside"), SolidLair::ContainsLocal(SolidLair::SpawnLocal(USolidWorldMap::XMarker)));
	TestTrue(TEXT("the Y spawn is inside"), SolidLair::ContainsLocal(SolidLair::SpawnLocal(USolidWorldMap::YMarker)));
	bool bFoundRock = false;
	for (int32 Row = 0; Row < SolidLair::MazeRows && !bFoundRock; ++Row)
	{
		for (int32 Col = 0; Col < SolidLair::MazeCols; ++Col)
		{
			if (SolidLair::IsOpenCell(Col, Row))
			{
				continue;
			}
			const FVector Rock(
				(static_cast<float>(Col) + 0.5f) * SolidLair::CellCm,
				(static_cast<float>(Row) + 0.5f) * SolidLair::CellCm,
				100.f);
			TestFalse(TEXT("a rock cell is outside"), SolidLair::ContainsLocal(Rock));
			bFoundRock = true;
			break;
		}
	}
	TestTrue(TEXT("the plan has rock"), bFoundRock);
	TestFalse(
		TEXT("above the ceiling is outside"),
		SolidLair::ContainsLocal(SolidLair::SpawnLocal(USolidWorldMap::XMarker) + FVector(0.f, 0.f, SolidLair::RoomHeightCm)));
	TestTrue(TEXT("X looks into the maze"), FMath::IsNearlyZero(SolidLair::SpawnYawDeg(USolidWorldMap::XMarker), 0.1f));
	TestTrue(TEXT("Y looks into the maze"), FMath::IsNearlyEqual(SolidLair::SpawnYawDeg(USolidWorldMap::YMarker), 180.f, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidLairGateTest,
	"SolidCore1.Lair.Gate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairGateTest::RunTest(const FString& Parameters)
{
	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	TestFalse(TEXT("the tunnels start shut"), SolidLair::TunnelsOpen(State));

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, SolidTowns::Iglin);
	TestFalse(TEXT("Iglin leaves the tunnels shut"), SolidLair::TunnelsOpen(State));
	SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestFalse(TEXT("the cave close leaves the tunnels shut"), SolidLair::TunnelsOpen(State));
	TestTrue(TEXT("that close still opens the cave"), State.bStrongholdsEnabled);

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, SolidTowns::Iglin);
	TestTrue(TEXT("Driven from the village opens the tunnels"), SolidLair::TunnelsOpen(State));

	const SolidLair::FPassageHit CaveEnter = SolidLair::HitOnStronghold(SolidStronghold::StartingCell(), false);
	const SolidLair::FPassageHit TunnelEnter = SolidLair::HitOnMouth(true, USolidWorldMap::XMarker, false);
	const SolidLair::FPassageHit TunnelLeave = SolidLair::HitOnMouth(true, USolidWorldMap::YMarker, true);
	TestEqual(TEXT("a shut tunnel click is ignored"), static_cast<uint8>(SolidLair::AllowPassage(TunnelEnter, false).Action), static_cast<uint8>(SolidLair::EHit::None));
	TestEqual(
		TEXT("an open tunnel click enters"),
		static_cast<uint8>(SolidLair::AllowPassage(TunnelEnter, true).Action),
		static_cast<uint8>(SolidLair::EHit::Enter));
	TestTrue(TEXT("that click keeps the X end"), SolidLair::AllowPassage(TunnelEnter, true).bLair);
	TestEqual(
		TEXT("the X end is kept"),
		static_cast<int32>(SolidLair::AllowPassage(TunnelEnter, true).EndMarker),
		static_cast<int32>(USolidWorldMap::XMarker));
	TestEqual(
		TEXT("leaving stays available while the tunnels are shut"),
		static_cast<uint8>(SolidLair::AllowPassage(TunnelLeave, false).Action),
		static_cast<uint8>(SolidLair::EHit::Leave));
	TestEqual(
		TEXT("the lair gate leaves a stronghold mouth"),
		static_cast<uint8>(SolidLair::AllowPassage(CaveEnter, false).Action),
		static_cast<uint8>(SolidLair::EHit::Enter));
	TestEqual(TEXT("tunnel tip"), FString(SolidLair::PassageTip(TunnelEnter)), FString(SolidLair::EnterLabel));
	TestEqual(TEXT("leave tip"), FString(SolidLair::PassageTip(TunnelLeave)), FString(SolidLair::LeaveLabel));
	TestEqual(TEXT("a cave enter logs Enter stronghold"), FString(SolidLair::PassageTip(CaveEnter)), FString(TEXT("Enter stronghold")));
	const SolidLair::FPassageHit CaveLeave = SolidLair::HitOnStronghold(SolidStronghold::StartingCell(), true);
	TestEqual(TEXT("a cave leave logs Exit stronghold"), FString(SolidLair::PassageTip(CaveLeave)), FString(TEXT("Exit stronghold")));
	TestTrue(TEXT("a miss has no tip"), FString(SolidLair::PassageTip(SolidLair::FPassageHit())).IsEmpty());

	SolidEvents::RememberLair(State, SolidWorldTime::FStamp(), true);
	TestEqual(TEXT("the visit is recorded"), State.Journal.Last().Text, FString(TEXT("Enter Iglin tunnels")));
	TestTrue(TEXT("the visit is a lair line"), State.Journal.Last().Type == SolidJournal::EJournalType::Lair);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidLairTravelTest,
	"SolidCore1.Lair.Travel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairTravelTest::RunTest(const FString& Parameters)
{
	const FString EnterX = SolidLair::EnterOptions(USolidWorldMap::XMarker);
	const FString EnterY = SolidLair::EnterOptions(TEXT('y'));
	TestTrue(TEXT("X options request the tunnels"), SolidLair::RequestsVisit(EnterX));
	TestTrue(TEXT("Y options request the tunnels"), SolidLair::RequestsVisit(EnterY));
	TestFalse(TEXT("tunnel options are not the stronghold"), SolidTravel::RequestsStronghold(EnterX));
	TestFalse(TEXT("stronghold options are not the tunnels"), SolidLair::RequestsVisit(SolidTravel::EnterStrongholdOptions()));
	TestFalse(TEXT("the open world is not the tunnels"), SolidLair::RequestsVisit(FString()));
	TestEqual(TEXT("X end"), static_cast<int32>(SolidLair::RequestedEnd(EnterX)), static_cast<int32>(USolidWorldMap::XMarker));
	TestEqual(TEXT("Y end"), static_cast<int32>(SolidLair::RequestedEnd(EnterY)), static_cast<int32>(USolidWorldMap::YMarker));
	TestEqual(TEXT("a stronghold url has no tunnel end"), static_cast<int32>(SolidLair::RequestedEnd(SolidTravel::EnterStrongholdOptions())), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidLairEntranceTest,
	"SolidCore1.Lair.Entrance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairEntranceTest::RunTest(const FString& Parameters)
{
	const FVector2D WorldMin(0.f, 0.f);
	const FVector2D WorldMax(6400.f, 6400.f);

	{
		FString Grid = SolidLairTestPrivate::MountainGrid();
		SolidLairTestPrivate::SetCell(Grid, 10, 10, USolidWorldMap::XMarker);
		SolidLairTestPrivate::SetCell(Grid, 10, 9, USolidWorldMap::GrassMarker);
		SolidLairTestPrivate::SetCell(Grid, 12, 10, USolidWorldMap::YMarker);
		SolidLairTestPrivate::SetCell(Grid, 11, 10, USolidWorldMap::GrassMarker);
		SolidLairTestPrivate::SetCell(Grid, 14, 10, USolidWorldMap::LocationMarker);
		SolidLairTestPrivate::SetCell(Grid, 14, 9, USolidWorldMap::GrassMarker);
		Grid += TEXT("! 11,15 = Stronghold (MeagreDungeon)\n");
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("mouths load"), Map->LoadFromString(Grid));

		TArray<SolidLair::FEntrance> Entrances;
		TestEqual(TEXT("letters are not tunnel mouths"), SolidLair::CollectEntrances(Map, WorldMin, WorldMax, Entrances), 0);

		SolidStrongholds::FEntrance CaveFace;
		TestTrue(
			TEXT("the cave cell has a grass face"),
			SolidStrongholds::MakeEntrance(Map, WorldMin, WorldMax, FIntPoint(14, 10), CaveFace));
		ESolidStrongholdType HoldType = ESolidStrongholdType::Dungeon;
		ESolidStrongholdSize HoldSize = ESolidStrongholdSize::Meagre;
		TestFalse(TEXT("X is still not a stronghold"), Map->StrongholdAtCell(10, 10, HoldType, HoldSize));
	}

	{
		FString Grid = SolidLairTestPrivate::MountainGrid();
		SolidLairTestPrivate::SetCell(Grid, 4, 4, USolidWorldMap::XMarker);
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		TestTrue(TEXT("a letter loads"), Map->LoadFromString(Grid));
		TArray<SolidLair::FEntrance> Entrances;
		TestEqual(TEXT("a sealed letter is not a mouth"), SolidLair::CollectEntrances(Map, WorldMin, WorldMax, Entrances), 0);
	}

	ASolidLairEntrance* Unmarked = NewObject<ASolidLairEntrance>();
	ASolidLairEntrance* Tunnel = NewObject<ASolidLairEntrance>();
	TestNotNull(TEXT("unmarked frame"), Unmarked);
	TestNotNull(TEXT("tunnel frame"), Tunnel);
	if (!Unmarked || !Tunnel)
	{
		return false;
	}
	TestFalse(TEXT("a fresh frame is unmarked"), Unmarked->IsLairMouth());
	Tunnel->SetLairEnd(USolidWorldMap::YMarker);
	TestTrue(TEXT("a tunnel frame is marked"), Tunnel->IsLairMouth());
	TestEqual(TEXT("the end is Y"), static_cast<int32>(Tunnel->GetEndMarker()), static_cast<int32>(USolidWorldMap::YMarker));
	Unmarked->BuildVisuals();
	Tunnel->BuildVisuals();
	UStaticMeshComponent* UnmarkedMouth = Unmarked->GetMouthMesh();
	UStaticMeshComponent* TunnelMouth = Tunnel->GetMouthMesh();
	TestNotNull(TEXT("unmarked opening"), UnmarkedMouth);
	TestNotNull(TEXT("tunnel opening"), TunnelMouth);
	if (UnmarkedMouth && TunnelMouth)
	{
		TestTrue(TEXT("the openings are the same size"), UnmarkedMouth->GetRelativeScale3D().Equals(TunnelMouth->GetRelativeScale3D(), 0.01f));
	}

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}

	const FVector2D ShippedMin(-51200.f, -51200.f);
	const FVector2D ShippedMax(51200.f, 51200.f);
	TArray<SolidLair::FEntrance> ShippedMouths;
	TestEqual(TEXT("the shipped map has two tunnel mouths"), SolidLair::CollectEntrances(Shipped, ShippedMin, ShippedMax, ShippedMouths), 2);

	FVector2D IglinXY = FVector2D::ZeroVector;
	FVector2D RelionXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Iglin"), Shipped->GetLocationWorldXY(SolidTowns::Iglin, ShippedMin, ShippedMax, IglinXY));
	TestTrue(TEXT("Relion"), Shipped->GetLocationWorldXY(SolidTowns::Relion, ShippedMin, ShippedMax, RelionXY));
	bool bSawX = false;
	bool bSawY = false;
	for (const SolidLair::FEntrance& Entrance : ShippedMouths)
	{
		TestEqual(
			TEXT("the mouth cell is mountain"),
			static_cast<uint8>(Shipped->GetBiomeAtCell(Entrance.Cell.X, Entrance.Cell.Y)),
			static_cast<uint8>(ESolidBiome::Mountain));
		TestEqual(
			TEXT("the face is grass"),
			static_cast<uint8>(Shipped->GetBiomeAtCell(Entrance.GrassCell.X, Entrance.GrassCell.Y)),
			static_cast<uint8>(ESolidBiome::Grassland));
		FVector2D World = FVector2D::ZeroVector;
		TestTrue(TEXT("mouth cell projects"), Shipped->CellToWorldXY(Entrance.Cell, ShippedMin, ShippedMax, World));
		if (Entrance.EndMarker == USolidWorldMap::XMarker)
		{
			bSawX = true;
			TestTrue(TEXT("X is south of Iglin"), World.Y < IglinXY.Y);
		}
		if (Entrance.EndMarker == USolidWorldMap::YMarker)
		{
			bSawY = true;
			TestTrue(TEXT("Y is east of Relion"), World.X > RelionXY.X);
		}
	}
	TestTrue(TEXT("X is on the shipped map"), bSawX);
	TestTrue(TEXT("Y is on the shipped map"), bSawY);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidLairBangMouthTest,
	"SolidCore1.Lair.BangMouth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairBangMouthTest::RunTest(const FString& Parameters)
{
	const FVector2D WorldMin(0.f, 0.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	FString Grid = SolidLairTestPrivate::MountainGrid();
	SolidLairTestPrivate::SetCell(Grid, 10, 10, USolidWorldMap::LocationMarker);
	SolidLairTestPrivate::SetCell(Grid, 10, 9, USolidWorldMap::GrassMarker);
	SolidLairTestPrivate::SetCell(Grid, 12, 10, USolidWorldMap::LocationMarker);
	SolidLairTestPrivate::SetCell(Grid, 11, 10, USolidWorldMap::GrassMarker);
	SolidLairTestPrivate::SetCell(Grid, 4, 4, USolidWorldMap::LocationMarker);
	Grid += TEXT("! 11,11 = LairEntrance (Iglin tunnels)\n");
	Grid += TEXT("! 11,13 = LairExit (Iglin tunnels)\n");
	Grid += TEXT("! 5,5 = LairEntrance (Iglin tunnels)\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("bang map"), Map);
	if (!Map || !TestTrue(TEXT("bang mouths load"), Map->LoadFromString(Grid)))
	{
		return false;
	}

	TArray<SolidLair::FEntrance> Entrances;
	TestEqual(TEXT("a sealed mouth is left out"), SolidLair::CollectEntrances(Map, WorldMin, WorldMax, Entrances), 2);
	if (Entrances.Num() == 2)
	{
		TestEqual(TEXT("entrance is the X end"), static_cast<int32>(Entrances[0].EndMarker), static_cast<int32>(USolidWorldMap::XMarker));
		TestEqual(TEXT("entrance faces north"), Entrances[0].GrassCell.Y, 9);
		TestEqual(TEXT("exit is the Y end"), static_cast<int32>(Entrances[1].EndMarker), static_cast<int32>(USolidWorldMap::YMarker));
		TestEqual(TEXT("exit faces east"), Entrances[1].GrassCell.X, 11);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidLairInteriorVisualTest,
	"SolidCore1.Lair.Interior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairInteriorVisualTest::RunTest(const FString& Parameters)
{
	ASolidLairInterior* Interior = NewObject<ASolidLairInterior>();
	TestNotNull(TEXT("interior"), Interior);
	if (!Interior)
	{
		return false;
	}
	Interior->BuildVisuals();

	UInstancedStaticMeshComponent* Floor = Interior->GetFloorTiles();
	UInstancedStaticMeshComponent* Walls = Interior->GetWallShell();
	UStaticMeshComponent* MouthX = Interior->GetEndMouth(USolidWorldMap::XMarker);
	UStaticMeshComponent* MouthY = Interior->GetEndMouth(USolidWorldMap::YMarker);
	TestNotNull(TEXT("floor"), Floor);
	TestNotNull(TEXT("walls"), Walls);
	TestNotNull(TEXT("X mouth"), MouthX);
	TestNotNull(TEXT("Y mouth"), MouthY);
	if (!Floor || !Walls || !MouthX || !MouthY)
	{
		return false;
	}
	int32 OpenCells = 0;
	for (int32 Row = 0; Row < SolidLair::MazeRows; ++Row)
	{
		for (int32 Col = 0; Col < SolidLair::MazeCols; ++Col)
		{
			if (SolidLair::IsOpenCell(Col, Row))
			{
				++OpenCells;
			}
		}
	}
	TestEqual(TEXT("one floor tile per open cell"), Floor->GetInstanceCount(), OpenCells);
	TestTrue(TEXT("the maze has walls"), Walls->GetInstanceCount() > 0);
	UStaticMesh* XMesh = MouthX->GetStaticMesh().Get();
	UStaticMesh* YMesh = MouthY->GetStaticMesh().Get();
	TestNotNull(TEXT("X mouth mesh"), XMesh);
	TestNotNull(TEXT("Y mouth mesh"), YMesh);
	TestTrue(TEXT("spawn at X is inside the actor"), SolidLair::ContainsLocal(Interior->GetCaptainSpawnWorld(USolidWorldMap::XMarker)));
	TestTrue(TEXT("the X view looks into the maze"), FMath::IsNearlyZero(Interior->GetCaptainView(USolidWorldMap::XMarker).Pitch, 0.1f));

	UPointLightComponent* LightX = Interior->GetMouthLight(USolidWorldMap::XMarker);
	UPointLightComponent* LightY = Interior->GetMouthLight(USolidWorldMap::YMarker);
	TestNotNull(TEXT("X mouth light"), LightX);
	TestNotNull(TEXT("Y mouth light"), LightY);
	if (LightX && LightY)
	{
		TestTrue(TEXT("the X mouth is dim"), FMath::IsNearlyEqual(LightX->Intensity, SolidLair::MouthLightIntensity));
		TestTrue(TEXT("the Y mouth is dim"), FMath::IsNearlyEqual(LightY->Intensity, SolidLair::MouthLightIntensity));
		TestTrue(TEXT("a mouth is dimmer than a torch"), SolidLair::MouthLightIntensity < SolidLair::TorchLightIntensity);
		const FIntPoint XCell = SolidLair::CellOf(USolidWorldMap::XMarker);
		const FVector2D XPlan = SolidLair::CellCenter(XCell.X, XCell.Y);
		TestTrue(TEXT("the X light sits in the X cell"), LightX->GetRelativeLocation().Equals(FVector(XPlan.X, XPlan.Y, SolidLair::RoomHeightCm * 0.62f), 1.f));
	}

	bool bMiddleLight = false;
	ForEachObjectWithOuter(Interior, [&bMiddleLight](UObject* Object)
	{
		if (Object && Object->GetName() == TEXT("LightMid"))
		{
			bMiddleLight = true;
		}
	});
	TestFalse(TEXT("the middle of the tunnels has no light"), bMiddleLight);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidLairTorchLightTest,
	"SolidCore1.Lair.TorchLight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairTorchLightTest::RunTest(const FString& Parameters)
{
	ASolidLairInterior* Interior = NewObject<ASolidLairInterior>();
	TestNotNull(TEXT("interior"), Interior);
	if (!Interior)
	{
		return false;
	}

	UPointLightComponent* Torch = Interior->GetTorchLight();
	TestNotNull(TEXT("torch light"), Torch);
	if (!Torch)
	{
		return false;
	}
	TestTrue(TEXT("the torch starts out"), FMath::IsNearlyZero(Torch->Intensity));

	const FVector Spot(120.f, 340.f, 96.f);
	SolidInventory::FLoadout Spear = SolidInventory::StartingLoadout();
	TestFalse(TEXT("the spear is not a torch"), SolidLair::HandsHoldTorch(Spear));
	Interior->ApplyTorchLight(Spot, false);
	TestTrue(TEXT("the spear leaves the tunnels dark"), FMath::IsNearlyZero(Torch->Intensity));
	TestFalse(TEXT("an unlit torch stays hidden"), Torch->IsVisible());

	SolidInventory::FLoadout Main = SolidInventory::StartingLoadout();
	TestTrue(TEXT("the main hand can hold the torch"), SolidInventory::PlaceWorn(Main, SolidInventory::ESlot::MainHand, SolidItemType::IdTorch));
	TestTrue(TEXT("the main hand holds a torch"), SolidLair::HandsHoldTorch(Main));
	Interior->ApplyTorchLight(Spot, true);
	TestTrue(TEXT("a torch in the main hand burns"), FMath::IsNearlyEqual(Torch->Intensity, SolidLair::TorchLightIntensity));
	TestTrue(TEXT("the flame is wood and pitch"), Torch->GetLightColor().Equals(SolidLair::TorchLightColor(), 0.02f));
	TestTrue(TEXT("the torch reaches a few cells"), FMath::IsNearlyEqual(Torch->AttenuationRadius, SolidLair::TorchLightRadiusCm));
	TestTrue(TEXT("the torch light sits on the captain"), Torch->GetRelativeLocation().Equals(Spot, 1.f));
	TestTrue(TEXT("a burning torch is visible"), Torch->IsVisible());

	SolidInventory::FLoadout Off = SolidInventory::StartingLoadout();
	TestTrue(TEXT("the off hand can hold the torch"), SolidInventory::PlaceWorn(Off, SolidInventory::ESlot::OffHand, SolidItemType::IdTorch));
	TestTrue(TEXT("the off hand holds a torch"), SolidLair::HandsHoldTorch(Off));
	const FVector Moved(480.f, 210.f, 96.f);
	Interior->ApplyTorchLight(Moved, SolidLair::HandsHoldTorch(Off));
	TestTrue(TEXT("a torch in the off hand burns"), FMath::IsNearlyEqual(Torch->Intensity, SolidLair::TorchLightIntensity));
	TestTrue(TEXT("the torch follows the captain"), Torch->GetRelativeLocation().Equals(Moved, 1.f));

	SolidInventory::FLoadout Packed = SolidInventory::StartingLoadout();
	TestTrue(TEXT("the pack can hold the torch"), SolidInventory::PlacePack(Packed, 0, 0, SolidItemType::IdTorch));
	TestFalse(TEXT("the body does not shadow the tunnels"), SolidLair::BodyCastsShadow(true));
	TestTrue(TEXT("the body still shadows the open world"), SolidLair::BodyCastsShadow(false));
	TestFalse(TEXT("a torch in the pack is not in a hand"), SolidLair::HandsHoldTorch(Packed));
	Interior->ApplyTorchLight(Spot, SolidLair::HandsHoldTorch(Packed));
	TestTrue(TEXT("a packed torch stays unlit"), FMath::IsNearlyZero(Torch->Intensity));
	TestFalse(TEXT("a packed torch stays hidden"), Torch->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidLairEntranceVisualsTest,
	"SolidCore1.Lair.EntranceVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidLairEntranceVisualsTest::RunTest(const FString& Parameters)
{
	ASolidLairEntrance* Entrance = NewObject<ASolidLairEntrance>();
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

#endif
