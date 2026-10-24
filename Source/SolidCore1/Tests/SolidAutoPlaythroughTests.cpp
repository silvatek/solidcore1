#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Company/SolidCaptain.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Inventory/SolidStorage.h"
#include "Menus/SolidMainMenu.h"
#include "Monster/SolidEncounter.h"
#include "Monster/SolidEncounters.h"
#include "Lairs/SolidLair.h"
#include "Playthrough/SolidAutoPlaythrough.h"
#include "SolidWorldChart.h"
#include "SolidCharacter.h"
#include "SolidGameState.h"
#include "Towns/SolidTowns.h"
#include "Towns/SolidWallgate.h"
#include "Strongholds/SolidStronghold.h"
#include "Strongholds/Tower/SolidTower.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughStepsTest,
	"SolidCore1.AutoPlaythrough.Steps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughStepsTest::RunTest(const FString& Parameters)
{
	const TCHAR* Names[] = {
		TEXT("Walk to the monster encounter"),
		TEXT("Wake in the cave"),
		TEXT("Disable the Iglin Bridge encounter"),
		TEXT("Walk to the stand"),
		TEXT("Craft the torch"),
		TEXT("Equip the torch"),
		TEXT("Equip the sword"),
		TEXT("Walk to the entrance"),
		TEXT("Leave the cave"),
		TEXT("Walk to Iglin"),
		TEXT("Close the driven-out message"),
		TEXT("Walk to the Iglin tunnels"),
		TEXT("Enter the Iglin tunnels"),
		TEXT("Walk the Iglin tunnels"),
		TEXT("Leave the Iglin tunnels"),
		TEXT("Walk to Relion"),
		TEXT("Walk to Kanfold"),
		TEXT("Walk to Dragongate"),
	};
	const FSolidAutoPlaythrough::EStepType Types[] = {
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::WakeInStronghold,
		FSolidAutoPlaythrough::EStepType::DisableEncounter,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::CraftItem,
		FSolidAutoPlaythrough::EStepType::EquipItem,
		FSolidAutoPlaythrough::EStepType::EquipItem,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::ExitStronghold,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::CloseMessage,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::EnterLair,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::ExitLair,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::WalkTo,
	};
	TestTrue(
		TEXT("each named row has a type"),
		UE_ARRAY_COUNT(Types) == UE_ARRAY_COUNT(Names));
	TestTrue(
		TEXT("the script holds those rows"),
		FSolidAutoPlaythrough::ScriptCount >= static_cast<int32>(UE_ARRAY_COUNT(Names)));

	int32 Search = 0;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		int32 Found = INDEX_NONE;
		for (int32 StepIndex = Search; StepIndex < FSolidAutoPlaythrough::ScriptCount; ++StepIndex)
		{
			if (FCString::Strcmp(FSolidAutoPlaythrough::Script[StepIndex].Name, Names[Index]) == 0)
			{
				Found = StepIndex;
				break;
			}
		}
		TestTrue(TEXT("step name"), Found != INDEX_NONE);
		if (Found == INDEX_NONE)
		{
			return false;
		}
		TestEqual(
			TEXT("step type"),
			static_cast<uint8>(FSolidAutoPlaythrough::Script[Found].Type),
			static_cast<uint8>(Types[Index]));
		Search = Found + 1;
	}

	TestTrue(
		TEXT("a stuck step stops after 4 seconds"),
		FMath::IsNearlyEqual(FSolidAutoPlaythrough::StallSeconds, 4.f));
	TestTrue(
		TEXT("a message stays up for one second"),
		FMath::IsNearlyEqual(FSolidAutoPlaythrough::MessagePauseSeconds, 1.f));
	const FSolidAutoPlaythrough::FStep* Close = nullptr;
	for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
	{
		if (FSolidAutoPlaythrough::Script[Index].Type == FSolidAutoPlaythrough::EStepType::CloseMessage)
		{
			Close = &FSolidAutoPlaythrough::Script[Index];
			break;
		}
	}
	TestTrue(TEXT("the script closes a message"), Close != nullptr);
	TestTrue(
		TEXT("the driven-out close uses that pause"),
		Close && FMath::IsNearlyEqual(Close->PauseSeconds, 1.f));
	const FSolidAutoPlaythrough::FStep* Disable = nullptr;
	for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
	{
		if (FSolidAutoPlaythrough::Script[Index].Type == FSolidAutoPlaythrough::EStepType::DisableEncounter)
		{
			Disable = &FSolidAutoPlaythrough::Script[Index];
			break;
		}
	}
	TestTrue(TEXT("the script disables an encounter"), Disable != nullptr);
	TestEqual(
		TEXT("that encounter is Iglin Bridge"),
		Disable ? FString(Disable->EncounterName) : FString(),
		FString(TEXT("Iglin Bridge")));

	const FSolidAutoPlaythrough::FStep* Enter = nullptr;
	const FSolidAutoPlaythrough::FStep* Run = nullptr;
	const FSolidAutoPlaythrough::FStep* Leave = nullptr;
	for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
	{
		const FSolidAutoPlaythrough::FStep& Step = FSolidAutoPlaythrough::Script[Index];
		if (FCString::Strcmp(Step.Name, TEXT("Enter the Iglin tunnels")) == 0)
		{
			Enter = &Step;
		}
		else if (FCString::Strcmp(Step.Name, TEXT("Walk the Iglin tunnels")) == 0)
		{
			Run = &Step;
		}
		else if (FCString::Strcmp(Step.Name, TEXT("Leave the Iglin tunnels")) == 0)
		{
			Leave = &Step;
		}
	}
	TestTrue(TEXT("the script enters a lair"), Enter != nullptr);
	TestTrue(TEXT("the script walks a lair"), Run != nullptr);
	TestTrue(TEXT("the script leaves a lair"), Leave != nullptr);
	TestEqual(
		TEXT("enter uses the Iglin tunnels id"),
		Enter ? FString(Enter->LairId) : FString(),
		FString(SolidLair::IdIglinTunnels));
	TestEqual(
		TEXT("the run uses the Iglin tunnels id"),
		Run ? FString(Run->LairId) : FString(),
		FString(SolidLair::IdIglinTunnels));
	TestEqual(
		TEXT("leave uses the Iglin tunnels id"),
		Leave ? FString(Leave->LairId) : FString(),
		FString(SolidLair::IdIglinTunnels));
	TestEqual(
		TEXT("enter is the Iglin Tunnels Entrance"),
		static_cast<int32>(Enter ? Enter->EndMarker : 0),
		static_cast<int32>(SolidLair::Entrance));
	TestEqual(
		TEXT("the run walks to the Iglin Tunnels Exit"),
		static_cast<int32>(Run ? Run->EndMarker : 0),
		static_cast<int32>(SolidLair::Exit));
	TestEqual(
		TEXT("leave is the Iglin Tunnels Exit"),
		static_cast<int32>(Leave ? Leave->EndMarker : 0),
		static_cast<int32>(SolidLair::Exit));

	const FIntPoint MouthX = SolidLair::CellOf(SolidLair::Entrance);
	const FIntPoint MouthY = SolidLair::CellOf(SolidLair::Exit);
	TArray<FIntPoint> RunCells;
	TestTrue(
		TEXT("the straight run joins the mouths"),
		FSolidAutoPlaythrough::LairCells(
			SolidLair::IdIglinTunnels,
			SolidLair::Entrance,
			SolidLair::Exit,
			RunCells));
	TestTrue(TEXT("the mouths share a row"), MouthX.Y == MouthY.Y && MouthX.X < MouthY.X);
	TestEqual(TEXT("the run is the straight line"), RunCells.Num(), MouthY.X - MouthX.X + 1);
	for (int32 Index = 0; Index < RunCells.Num(); ++Index)
	{
		TestEqual(TEXT("run column"), RunCells[Index].X, MouthX.X + Index);
		TestEqual(TEXT("run row"), RunCells[Index].Y, MouthX.Y);
	}

	TArray<FIntPoint> Back;
	TestTrue(
		TEXT("the run walks back to the Iglin Tunnels Entrance"),
		FSolidAutoPlaythrough::LairCells(
			SolidLair::IdIglinTunnels,
			SolidLair::Exit,
			SolidLair::Entrance,
			Back));
	TestEqual(TEXT("the way back is the same length"), Back.Num(), RunCells.Num());
	if (Back.Num() == RunCells.Num())
	{
		for (int32 Index = 0; Index < RunCells.Num(); ++Index)
		{
			TestEqual(TEXT("way back column"), Back[Index].X, RunCells[RunCells.Num() - 1 - Index].X);
			TestEqual(TEXT("way back row"), Back[Index].Y, RunCells[RunCells.Num() - 1 - Index].Y);
		}
	}

	TArray<FIntPoint> Same;
	TestTrue(
		TEXT("a mouth path can stay on that mouth"),
		FSolidAutoPlaythrough::LairCells(
			SolidLair::IdIglinTunnels,
			SolidLair::Entrance,
			SolidLair::Entrance,
			Same));
	TestEqual(TEXT("one cell"), Same.Num(), 1);

	TArray<FIntPoint> Missing;
	TestFalse(
		TEXT("an unknown lair has no path"),
		FSolidAutoPlaythrough::LairCells(
			TEXT("00000000"),
			SolidLair::Entrance,
			SolidLair::Exit,
			Missing));
	TArray<FIntPoint> Rock;
	TestFalse(
		TEXT("rock is not a path"),
		FSolidAutoPlaythrough::LairCells(
			SolidLair::IdIglinTunnels,
			FIntPoint(MouthX.X - 1, MouthX.Y),
			MouthY,
			Rock));

	const int32 TownIds[] = {
		SolidTowns::Relion,
		SolidTowns::Kanfold,
		SolidTowns::Dragongate,
	};
	int32 TownSteps = 0;
	for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
	{
		const FSolidAutoPlaythrough::FStep& Step = FSolidAutoPlaythrough::Script[Index];
		if (Step.Type != FSolidAutoPlaythrough::EStepType::WalkTo
			|| Step.Walk != FSolidAutoPlaythrough::EWalk::Town)
		{
			continue;
		}
		if (TownSteps < UE_ARRAY_COUNT(TownIds))
		{
			TestEqual(TEXT("town id"), Step.TownId, TownIds[TownSteps]);
		}
		++TownSteps;
	}
	TestEqual(TEXT("three town walks"), TownSteps, static_cast<int32>(UE_ARRAY_COUNT(TownIds)));

	TArray<SolidRoad::FRoute> Routes;
	{
		SolidRoad::FRoute Highway;
		Highway.Definition.From.LocationId = SolidTowns::Relion;
		Highway.Definition.To.LocationId = SolidTowns::Kanfold;
		Highway.Cells = { FIntPoint(1, 1), FIntPoint(1, 2), FIntPoint(1, 3) };
		Routes.Add(Highway);

		SolidRoad::FRoute Road;
		Road.Definition.From.LocationId = SolidTowns::Relion;
		Road.Definition.To.LocationId = SolidTowns::Dragongate;
		Road.Cells = { FIntPoint(1, 1), FIntPoint(2, 1), FIntPoint(3, 1) };
		Routes.Add(Road);

		SolidRoad::FRoute Track;
		Track.Definition.From.LocationId = SolidTowns::Iglin;
		Track.Definition.To.LocationId = SolidTowns::Relion;
		Track.Cells = { FIntPoint(0, 0), FIntPoint(1, 1) };
		Routes.Add(Track);

		SolidRoad::FRoute Hold;
		Hold.Definition.From.bStronghold = true;
		Hold.Definition.To.LocationId = SolidTowns::Dragongate;
		Hold.Cells = { FIntPoint(9, 9), FIntPoint(3, 1) };
		Routes.Add(Hold);

		SolidRoad::FRoute Empty;
		Empty.Definition.From.LocationId = SolidTowns::Visolar;
		Empty.Definition.To.LocationId = SolidTowns::Dragongate;
		Routes.Add(Empty);
	}

	const auto ExpectCells = [this](const TArray<FIntPoint>& Cells, const FIntPoint* Expected, const int32 Count)
	{
		TestEqual(TEXT("chain length"), Cells.Num(), Count);
		if (Cells.Num() != Count)
		{
			return;
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			TestEqual(TEXT("chain column"), Cells[Index].X, Expected[Index].X);
			TestEqual(TEXT("chain row"), Cells[Index].Y, Expected[Index].Y);
		}
	};

	TArray<FIntPoint> ToKanfold;
	const FIntPoint KanfoldCells[] = { FIntPoint(1, 1), FIntPoint(1, 2), FIntPoint(1, 3) };
	TestTrue(
		TEXT("Relion reaches Kanfold"),
		FSolidAutoPlaythrough::TownRoads(Routes, SolidTowns::Relion, SolidTowns::Kanfold, ToKanfold));
	ExpectCells(ToKanfold, KanfoldCells, UE_ARRAY_COUNT(KanfoldCells));

	TArray<FIntPoint> ToGate;
	const FIntPoint GateCells[] = {
		FIntPoint(1, 3), FIntPoint(1, 2), FIntPoint(1, 1), FIntPoint(2, 1), FIntPoint(3, 1),
	};
	TestTrue(
		TEXT("Kanfold reaches Dragongate through Relion"),
		FSolidAutoPlaythrough::TownRoads(Routes, SolidTowns::Kanfold, SolidTowns::Dragongate, ToGate));
	ExpectCells(ToGate, GateCells, UE_ARRAY_COUNT(GateCells));

	TArray<FIntPoint> SameTown;
	TestFalse(
		TEXT("a town does not road to itself"),
		FSolidAutoPlaythrough::TownRoads(Routes, SolidTowns::Relion, SolidTowns::Relion, SameTown));
	TestEqual(TEXT("that path is empty"), SameTown.Num(), 0);

	TArray<FIntPoint> MissingTown;
	TestFalse(
		TEXT("Visolar has no chain to Dragongate"),
		FSolidAutoPlaythrough::TownRoads(Routes, SolidTowns::Visolar, SolidTowns::Dragongate, MissingTown));
	TestEqual(TEXT("the missing chain is empty"), MissingTown.Num(), 0);
	TestEqual(
		TEXT("a Relion cell starts from Relion"),
		FSolidAutoPlaythrough::TownWalkFrom(SolidTowns::Relion, SolidTowns::Relion),
		SolidTowns::Relion);
	TestEqual(
		TEXT("grass west of Relion still starts from Relion"),
		FSolidAutoPlaythrough::TownWalkFrom(INDEX_NONE, SolidTowns::Relion),
		SolidTowns::Relion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughMenuTest,
	"SolidCore1.AutoPlaythrough.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughMenuTest::RunTest(const FString& Parameters)
{
	const TCHAR* Labels[] = {
		TEXT("Use portal"),
		TEXT("Journal"),
		TEXT("Build squad"),
		TEXT("Character editor"),
		TEXT("Auto playthrough"),
		TEXT("Credits"),
		TEXT("Exit Game"),
	};
	const ESolidMainMenuItem Items[] = {
		ESolidMainMenuItem::UsePortal,
		ESolidMainMenuItem::Journal,
		ESolidMainMenuItem::BuildSquad,
		ESolidMainMenuItem::CharacterEditor,
		ESolidMainMenuItem::AutoPlaythrough,
		ESolidMainMenuItem::Credits,
		ESolidMainMenuItem::ExitGame,
	};
	const ESolidMainMenuAction Actions[] = {
		ESolidMainMenuAction::OpenPortal,
		ESolidMainMenuAction::ShowJournal,
		ESolidMainMenuAction::OpenSquad,
		ESolidMainMenuAction::OpenCharacterEditor,
		ESolidMainMenuAction::StartAutoPlaythrough,
		ESolidMainMenuAction::ShowCredits,
		ESolidMainMenuAction::Quit,
	};

	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	TestEqual(TEXT("seven rows"), Entries.Num(), static_cast<int32>(UE_ARRAY_COUNT(Labels)));
	if (Entries.Num() != UE_ARRAY_COUNT(Labels))
	{
		return false;
	}

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Labels); ++Index)
	{
		TestEqual(TEXT("row label"), Entries[Index].Label, FString(Labels[Index]));
		TestEqual(
			TEXT("row item"),
			static_cast<uint8>(Entries[Index].Item),
			static_cast<uint8>(Items[Index]));
		TestEqual(
			TEXT("row action"),
			static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[Index].Item)),
			static_cast<uint8>(Actions[Index]));
	}

	TestEqual(TEXT("the row text"), FString(FSolidAutoPlaythrough::Label()), FString(TEXT("Auto playthrough")));
	TestEqual(TEXT("key 5 is Auto playthrough"), Entries[4].Label, FString(TEXT("Auto playthrough")));
	TestEqual(TEXT("key 6 is Credits"), Entries[5].Label, FString(TEXT("Credits")));
	TestEqual(TEXT("key 7 is Exit Game"), Entries[6].Label, FString(TEXT("Exit Game")));
	TestEqual(
		TEXT("Credits stays immediately before Exit Game"),
		static_cast<uint8>(Entries[5].Item),
		static_cast<uint8>(ESolidMainMenuItem::Credits));
	FSolidMainMenuEntry PastEnd;
	TestFalse(TEXT("key 8 has no row"), SolidMainMenu::FindEntry(7, PastEnd));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughDimTest,
	"SolidCore1.AutoPlaythrough.Dim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughDimTest::RunTest(const FString& Parameters)
{
	USolidGameState* Spawned = NewObject<USolidGameState>();
	USolidCaptain* SpawnedCaptain = NewObject<USolidCaptain>();
	TestNotNull(TEXT("play state"), Spawned);
	TestNotNull(TEXT("captain"), SpawnedCaptain);
	if (!Spawned || !SpawnedCaptain)
	{
		return false;
	}

	TestTrue(TEXT("a new game can start"), FSolidAutoPlaythrough::CanStart(*Spawned));
	TestFalse(TEXT("the row starts bright"), Spawned->bAutoPlaythroughDim);
	SolidEvents::NotifyOccupiedTown(*Spawned, *SpawnedCaptain, SolidTowns::Iglin);
	TestTrue(TEXT("arriving in Iglin leaves the row available"), FSolidAutoPlaythrough::CanStart(*Spawned));
	TestFalse(TEXT("spawn does not dim the row"), Spawned->bAutoPlaythroughDim);

	SolidEvents::NotifyOccupiedTown(*Spawned, *SpawnedCaptain, INDEX_NONE);
	TestFalse(TEXT("leaving Iglin dims the row"), FSolidAutoPlaythrough::CanStart(*Spawned));
	TestTrue(TEXT("the flag stays set"), Spawned->bAutoPlaythroughDim);
	SolidEvents::NotifyOccupiedTown(*Spawned, *SpawnedCaptain, SolidTowns::Iglin);
	TestFalse(TEXT("walking back into Iglin leaves the row dim"), FSolidAutoPlaythrough::CanStart(*Spawned));

	USolidGameState* Portaled = NewObject<USolidGameState>();
	USolidCaptain* PortaledCaptain = NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(*Portaled, *PortaledCaptain, SolidTowns::Iglin);
	SolidEvents::NotifyOccupiedTown(*Portaled, *PortaledCaptain, SolidTowns::Relion);
	TestTrue(TEXT("a portal out of Iglin dims the row"), Portaled->bAutoPlaythroughDim);

	USolidGameState* Started = NewObject<USolidGameState>();
	USolidCaptain* StartedCaptain = NewObject<USolidCaptain>();
	FSolidAutoPlaythrough Play;
	TestFalse(TEXT("the script is idle"), Play.IsRunning());
	Play.Start(*Started);
	TestTrue(TEXT("choosing the row starts the script"), Play.IsRunning());
	TestTrue(TEXT("choosing the row dims it"), Started->bAutoPlaythroughDim);
	TestFalse(TEXT("a second start is refused"), FSolidAutoPlaythrough::CanStart(*Started));

	SolidEvents::NotifyOccupiedTown(*Started, *StartedCaptain, SolidTowns::Iglin);
	SolidEvents::NotifyOccupiedTown(*Started, *StartedCaptain, INDEX_NONE);
	TestTrue(TEXT("leaving Iglin does not stop a running script"), Play.IsRunning());

	Play.Stop();
	TestFalse(TEXT("Esc clears the script"), Play.IsRunning());
	TestTrue(TEXT("Esc leaves the row dim"), Started->bAutoPlaythroughDim);
	TestFalse(TEXT("the dim row does not start again"), FSolidAutoPlaythrough::CanStart(*Started));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughKeysTest,
	"SolidCore1.AutoPlaythrough.Keys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughKeysTest::RunTest(const FString& Parameters)
{
	ASolidCharacter* Character = NewObject<ASolidCharacter>();
	TestNotNull(TEXT("character"), Character);
	if (!Character)
	{
		return false;
	}

	FVector2D Stick = FVector2D::ZeroVector;
	TestTrue(TEXT("W is mapped"), Character->StickForKey(EKeys::W, Stick));
	TestTrue(TEXT("W is forward"), Stick.Equals(FVector2D(0.f, 1.f)));
	TestTrue(TEXT("S is mapped"), Character->StickForKey(EKeys::S, Stick));
	TestTrue(TEXT("S is back"), Stick.Equals(FVector2D(0.f, -1.f)));
	TestTrue(TEXT("D is mapped"), Character->StickForKey(EKeys::D, Stick));
	TestTrue(TEXT("D is right"), Stick.Equals(FVector2D(1.f, 0.f)));
	TestTrue(TEXT("A is mapped"), Character->StickForKey(EKeys::A, Stick));
	TestTrue(TEXT("A is left"), Stick.Equals(FVector2D(-1.f, 0.f)));
	TestFalse(TEXT("a click is not a move key"), Character->StickForKey(EKeys::LeftMouseButton, Stick));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughTorchTest,
	"SolidCore1.AutoPlaythrough.Torch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughTorchTest::RunTest(const FString& Parameters)
{
	SolidInventory::FLoadout Loadout;
	SolidInventory::Clear(Loadout);
	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	const int32 Body = SolidInventory::WornIndex(SolidInventory::ESlot::Body);
	const int32 Main = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 Off = SolidInventory::WornIndex(SolidInventory::ESlot::OffHand);
	const int32 Stick = SolidStorage::Index(1, 1);
	const int32 Pitch = SolidStorage::Index(0, 2);
	const int32 Sword = SolidStorage::Index(0, 0);
	const int32 Boots = SolidStorage::Index(1, 0);
	const int32 Coins = SolidStorage::Index(0, 1);
	const int32 Empty = SolidStorage::Index(1, 2);
	if (Body == INDEX_NONE || Main == INDEX_NONE || Off == INDEX_NONE || Stick == INDEX_NONE
		|| Pitch == INDEX_NONE || Sword == INDEX_NONE || Boots == INDEX_NONE || Coins == INDEX_NONE
		|| Empty == INDEX_NONE)
	{
		AddError(TEXT("a slot index is missing"));
		return false;
	}

	TestTrue(TEXT("the body starts in rags"), Loadout.Worn[Body].Is(SolidItemType::IdRags));
	TestTrue(TEXT("the hands start empty"), Loadout.Worn[Main].IsEmpty());
	TestTrue(TEXT("one stick and one pitch make a torch"), FSolidAutoPlaythrough::CraftTorch(Loadout, Chest));
	TestTrue(
		TEXT("the torch goes to the off-hand"),
		FSolidAutoPlaythrough::EquipItem(
			Loadout,
			&Chest,
			SolidItemType::IdTorch,
			SolidInventory::ESlot::OffHand));
	TestTrue(
		TEXT("the rusty sword goes to the main hand"),
		FSolidAutoPlaythrough::EquipItem(
			Loadout,
			&Chest,
			SolidItemType::IdRustySword,
			SolidInventory::ESlot::MainHand));

	TestTrue(TEXT("rags stay on the body"), Loadout.Worn[Body].Is(SolidItemType::IdRags));
	TestEqual(TEXT("one set of rags"), Loadout.Worn[Body].Count, 1);
	TestTrue(TEXT("the off-hand holds the torch"), Loadout.Worn[Off].Is(SolidItemType::IdTorch));
	TestEqual(TEXT("one torch"), Loadout.Worn[Off].Count, 1);
	TestTrue(TEXT("the main hand holds the rusty sword"), Loadout.Worn[Main].Is(SolidItemType::IdRustySword));
	TestEqual(TEXT("one rusty sword"), Loadout.Worn[Main].Count, 1);
	for (int32 Index = 0; Index < SolidInventory::WornCount; ++Index)
	{
		if (Index == Body || Index == Main || Index == Off)
		{
			continue;
		}
		TestTrue(TEXT("the other worn boxes stay empty"), Loadout.Worn[Index].IsEmpty());
	}
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		TestTrue(TEXT("the pack is empty"), Loadout.Pack[Index].IsEmpty());
	}
	for (int32 Index = 0; Index < SolidInventory::CraftCount; ++Index)
	{
		TestTrue(TEXT("the craft slots are empty"), Loadout.Craft[Index].IsEmpty());
	}

	TestTrue(TEXT("the sword left the chest"), Chest.Slots[Sword].IsEmpty());
	TestTrue(TEXT("the boots stay"), Chest.Slots[Boots].Is(SolidItemType::IdOldBoots));
	TestEqual(TEXT("one pair of boots"), Chest.Slots[Boots].Count, 1);
	TestTrue(TEXT("the coins stay"), Chest.Slots[Coins].Is(SolidItemType::IdCoins));
	TestEqual(TEXT("the coin stack stays"), Chest.Slots[Coins].Count, SolidStorage::StartingCoinCount);
	TestTrue(TEXT("the sticks are back"), Chest.Slots[Stick].Is(SolidItemType::IdStick));
	TestEqual(TEXT("four sticks remain"), Chest.Slots[Stick].Count, SolidStorage::StartingStickCount - 1);
	TestTrue(TEXT("the pitch is back"), Chest.Slots[Pitch].Is(SolidItemType::IdPitch));
	TestEqual(TEXT("two pitch remain"), Chest.Slots[Pitch].Count, SolidStorage::StartingPitchCount - 1);
	TestTrue(TEXT("the last chest cell stays empty"), Chest.Slots[Empty].IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughDisableEncounterTest,
	"SolidCore1.AutoPlaythrough.DisableEncounter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughDisableEncounterTest::RunTest(const FString& Parameters)
{
	USolidGameState* State = NewObject<USolidGameState>();
	TestNotNull(TEXT("play state"), State);
	if (!State)
	{
		return false;
	}

	TestFalse(
		TEXT("a new game places Iglin Bridge"),
		SolidEncounter::IsDisabled(*State, TEXT("Iglin Bridge")));
	SolidEncounter::Disable(*State, TEXT("Iglin Bridge"));
	TestTrue(
		TEXT("Disable records Iglin Bridge"),
		SolidEncounter::IsDisabled(*State, TEXT("Iglin Bridge")));
	TestTrue(
		TEXT("the catalog name matches without case"),
		SolidEncounter::IsDisabled(*State, TEXT("iglin bridge")));
	SolidEncounter::Disable(*State, TEXT("Iglin Bridge"));
	TestEqual(TEXT("a second disable is ignored"), State->DisabledEncounters.Num(), 1);
	TestFalse(TEXT("an empty name is not disabled"), SolidEncounter::IsDisabled(*State, TEXT("")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughKanfoldShowTest,
	"SolidCore1.AutoPlaythrough.KanfoldShow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughKanfoldShowTest::RunTest(const FString& Parameters)
{
	const FSolidAutoPlaythrough::FStep Aim = FSolidAutoPlaythrough::FStep::AimRaven(
		TEXT("Aim raven high behind the squad"),
		FSolidAutoPlaythrough::HighTrailPitchDeg,
		FSolidAutoPlaythrough::HighTrailArmCm);
	TestEqual(
		TEXT("AimRaven type"),
		static_cast<uint8>(Aim.Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::AimRaven));
	TestTrue(
		TEXT("high trail looks down"),
		Aim.PitchDeg < 0.f
		&& FMath::IsNearlyEqual(Aim.PitchDeg, FSolidAutoPlaythrough::HighTrailPitchDeg));
	TestTrue(
		TEXT("high trail pulls the boom out"),
		Aim.ArmCm > 450.f
		&& FMath::IsNearlyEqual(Aim.ArmCm, FSolidAutoPlaythrough::HighTrailArmCm));
	TestTrue(TEXT("high trail stays behind the captain"), FMath::IsNearlyZero(Aim.YawOffsetDeg));

	const FSolidAutoPlaythrough::FStep Form = FSolidAutoPlaythrough::FStep::SelectBattlePlan(
		TEXT("Form Tight mob"),
		TEXT("Tight mob"));
	TestEqual(
		TEXT("SelectBattlePlan type"),
		static_cast<uint8>(Form.Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::SelectBattlePlan));
	TestEqual(TEXT("plan name"), FString(Form.PlanName), FString(TEXT("Tight mob")));

	const TCHAR* Plans[] = {
		TEXT("Line"),
		TEXT("Column"),
		TEXT("Loose mob"),
		TEXT("Tight mob"),
	};
	int32 Kanfold = INDEX_NONE;
	int32 Dragongate = INDEX_NONE;
	int32 AimIndex = INDEX_NONE;
	int32 PlanIndex = 0;
	int32 LastPlan = INDEX_NONE;
	for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
	{
		const FSolidAutoPlaythrough::FStep& Step = FSolidAutoPlaythrough::Script[Index];
		if (FCString::Strcmp(Step.Name, TEXT("Walk to Kanfold")) == 0)
		{
			Kanfold = Index;
		}
		else if (Step.Type == FSolidAutoPlaythrough::EStepType::AimRaven && AimIndex == INDEX_NONE)
		{
			AimIndex = Index;
			TestTrue(
				TEXT("Kanfold aims the high trail"),
				FMath::IsNearlyEqual(Step.PitchDeg, FSolidAutoPlaythrough::HighTrailPitchDeg)
				&& FMath::IsNearlyEqual(Step.ArmCm, FSolidAutoPlaythrough::HighTrailArmCm));
		}
		else if (Step.Type == FSolidAutoPlaythrough::EStepType::SelectBattlePlan)
		{
			TestTrue(TEXT("those plans follow Kanfold"), Kanfold != INDEX_NONE && Index > Kanfold);
			if (PlanIndex < UE_ARRAY_COUNT(Plans))
			{
				TestEqual(TEXT("plan order"), FString(Step.PlanName), FString(Plans[PlanIndex]));
			}
			++PlanIndex;
			LastPlan = Index;
		}
		else if (FCString::Strcmp(Step.Name, TEXT("Walk to Dragongate")) == 0)
		{
			Dragongate = Index;
		}
	}
	TestTrue(TEXT("Kanfold is in the script"), Kanfold != INDEX_NONE);
	TestTrue(TEXT("the high trail is after Kanfold"), AimIndex != INDEX_NONE && AimIndex > Kanfold);
	TestEqual(TEXT("four plans"), PlanIndex, static_cast<int32>(UE_ARRAY_COUNT(Plans)));
	TestTrue(TEXT("Tight mob is last"), LastPlan != INDEX_NONE && LastPlan < Dragongate);
	TestTrue(TEXT("Dragongate follows Tight mob"), Dragongate == LastPlan + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughPausesTest,
	"SolidCore1.AutoPlaythrough.Pauses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughPausesTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("the default hold is one second"),
		FMath::IsNearlyEqual(FSolidAutoPlaythrough::DefaultPauseSeconds, 1.f));
	TestTrue(
		TEXT("a message uses that hold"),
		FMath::IsNearlyEqual(
			FSolidAutoPlaythrough::MessagePauseSeconds,
			FSolidAutoPlaythrough::DefaultPauseSeconds));

	const FSolidAutoPlaythrough::FStep Sword = FSolidAutoPlaythrough::FStep::EquipItem(
		TEXT("Equip the sword"),
		SolidItemType::IdRustySword,
		SolidInventory::ESlot::MainHand)
		.ClosePage();
	TestTrue(TEXT("ClosePage marks the close"), Sword.bClosePage);
	TestTrue(
		TEXT("ClosePage holds one second"),
		FMath::IsNearlyEqual(Sword.PauseSeconds, FSolidAutoPlaythrough::DefaultPauseSeconds));

	const FSolidAutoPlaythrough::FStep Aim = FSolidAutoPlaythrough::FStep::AimRaven(
		TEXT("Aim raven high behind the squad"),
		FSolidAutoPlaythrough::HighTrailPitchDeg,
		FSolidAutoPlaythrough::HighTrailArmCm);
	TestTrue(
		TEXT("AimRaven holds one second"),
		FMath::IsNearlyEqual(Aim.PauseSeconds, FSolidAutoPlaythrough::DefaultPauseSeconds));

	const FSolidAutoPlaythrough::FStep Form = FSolidAutoPlaythrough::FStep::SelectBattlePlan(
		TEXT("Form Tight mob"),
		TEXT("Tight mob"));
	TestTrue(
		TEXT("SelectBattlePlan holds one second"),
		FMath::IsNearlyEqual(Form.PauseSeconds, FSolidAutoPlaythrough::DefaultPauseSeconds));

	bool bFoundSword = false;
	for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
	{
		const FSolidAutoPlaythrough::FStep& Step = FSolidAutoPlaythrough::Script[Index];
		if (Step.PauseSeconds > 0.f)
		{
			TestTrue(
				TEXT("a hold is one second"),
				FMath::IsNearlyEqual(Step.PauseSeconds, FSolidAutoPlaythrough::DefaultPauseSeconds));
		}
		if (FCString::Strcmp(Step.Name, TEXT("Equip the sword")) == 0)
		{
			bFoundSword = true;
			TestTrue(TEXT("the cave sword closes the page"), Step.bClosePage);
			TestTrue(
				TEXT("the cave sword waits before that close"),
				FMath::IsNearlyEqual(Step.PauseSeconds, FSolidAutoPlaythrough::DefaultPauseSeconds));
		}
	}
	TestTrue(TEXT("the cave sword is in the script"), bFoundSword);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughTrailYawTest,
	"SolidCore1.AutoPlaythrough.TrailYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughTrailYawTest::RunTest(const FString& Parameters)
{
	float Yaw = 45.f;
	TestFalse(
		TEXT("a stand leaves the yaw"),
		FSolidAutoPlaythrough::TrailYawDeg(FVector2D::ZeroVector, 0.f, Yaw));
	TestTrue(TEXT("that stand yaw stays"), FMath::IsNearlyEqual(Yaw, 45.f));

	TestTrue(
		TEXT("east looks east"),
		FSolidAutoPlaythrough::TrailYawDeg(FVector2D(1.f, 0.f), 0.f, Yaw));
	TestTrue(TEXT("east yaw"), FMath::IsNearlyEqual(Yaw, 0.f));

	TestTrue(
		TEXT("north looks north"),
		FSolidAutoPlaythrough::TrailYawDeg(FVector2D(0.f, 1.f), 0.f, Yaw));
	TestTrue(TEXT("north yaw"), FMath::IsNearlyEqual(Yaw, 90.f));

	TestTrue(
		TEXT("an offset turns from the walk"),
		FSolidAutoPlaythrough::TrailYawDeg(FVector2D(1.f, 0.f), 10.f, Yaw));
	TestTrue(TEXT("offset yaw"), FMath::IsNearlyEqual(Yaw, 10.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughStage4Test,
	"SolidCore1.AutoPlaythrough.Stage4",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughStage4Test::RunTest(const FString& Parameters)
{
	const TCHAR* Names[] = {
		TEXT("Walk to Dragongate"),
		TEXT("Walk to the Imperious site"),
		TEXT("Choose Imperious Tower"),
		TEXT("Wake in the tower"),
		TEXT("Leave the tower"),
		TEXT("Build squad Max"),
		TEXT("Build squad Morgan"),
		TEXT("Build squad Quinn"),
	};
	const FSolidAutoPlaythrough::EStepType Types[] = {
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::ChooseStronghold,
		FSolidAutoPlaythrough::EStepType::WakeInStronghold,
		FSolidAutoPlaythrough::EStepType::ExitStronghold,
		FSolidAutoPlaythrough::EStepType::BuildSquad,
		FSolidAutoPlaythrough::EStepType::BuildSquad,
		FSolidAutoPlaythrough::EStepType::BuildSquad,
	};
	TestTrue(
		TEXT("each named row has a type"),
		UE_ARRAY_COUNT(Types) == UE_ARRAY_COUNT(Names));

	int32 Search = 0;
	const FSolidAutoPlaythrough::FStep* FoundSteps[UE_ARRAY_COUNT(Names)] = {};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		int32 Found = INDEX_NONE;
		for (int32 StepIndex = Search; StepIndex < FSolidAutoPlaythrough::ScriptCount; ++StepIndex)
		{
			if (FCString::Strcmp(FSolidAutoPlaythrough::Script[StepIndex].Name, Names[Index]) == 0)
			{
				Found = StepIndex;
				break;
			}
		}
		TestTrue(TEXT("stage 4 step name"), Found != INDEX_NONE);
		if (Found == INDEX_NONE)
		{
			return false;
		}
		FoundSteps[Index] = &FSolidAutoPlaythrough::Script[Found];
		TestEqual(
			TEXT("stage 4 step type"),
			static_cast<uint8>(FoundSteps[Index]->Type),
			static_cast<uint8>(Types[Index]));
		Search = Found + 1;
	}

	const FSolidAutoPlaythrough::FStep* Site = FoundSteps[1];
	TestEqual(
		TEXT("the site walk is a StrongholdSite"),
		static_cast<uint8>(Site->Walk),
		static_cast<uint8>(FSolidAutoPlaythrough::EWalk::StrongholdSite));
	TestEqual(
		TEXT("that site is Imperious"),
		static_cast<uint8>(Site->StrongholdSize),
		static_cast<uint8>(ESolidStrongholdSize::Imperious));

	const FSolidAutoPlaythrough::FStep* Choice = FoundSteps[2];
	TestEqual(
		TEXT("the choice is Imperious"),
		static_cast<uint8>(Choice->StrongholdSize),
		static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestEqual(
		TEXT("the choice is a Tower"),
		static_cast<uint8>(Choice->StrongholdType),
		static_cast<uint8>(ESolidStrongholdType::Tower));

	const FSolidAutoPlaythrough::FStep* LeaveTower = FoundSteps[4];
	TestEqual(
		TEXT("wake goes straight to the leave"),
		static_cast<uint8>(LeaveTower->Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::ExitStronghold));
	float DoorYaw = 0.f;
	TestTrue(
		TEXT("a door on -X has a look"),
		FSolidAutoPlaythrough::TrailYawDeg(FVector2D(-1.f, 0.f), 0.f, DoorYaw));
	TestTrue(TEXT("the leave looks back at that door"), FMath::IsNearlyEqual(FMath::Abs(DoorYaw), 180.f, 0.1f));

	const TCHAR* Companions[] = { TEXT("Max"), TEXT("Morgan"), TEXT("Quinn") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Companions); ++Index)
	{
		const FSolidAutoPlaythrough::FStep* Place = FoundSteps[5 + Index];
		TestEqual(
			TEXT("squad companion"),
			FString(Place->CompanionName),
			FString(Companions[Index]));
		TestTrue(
			TEXT("Quinn is the last page close"),
			Index < 2 ? !Place->bClosePage : Place->bClosePage);
	}
	TestEqual(TEXT("Quinn is the last row"), Search, FSolidAutoPlaythrough::ScriptCount);

	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("world chart"), Chart);
	TestTrue(TEXT("the shipped chart loads"), Chart && Chart->LoadDefault());
	if (Chart && Chart->IsLoaded())
	{
		FIntPoint Cell = FIntPoint::ZeroValue;
		TestTrue(
			TEXT("the chart has an Imperious site"),
			FSolidAutoPlaythrough::StrongholdSiteCell(
				*Chart,
				ESolidStrongholdSize::Imperious,
				Cell));
		ESolidStrongholdSize SiteSize = ESolidStrongholdSize::Meagre;
		TestTrue(TEXT("that cell is a site"), Chart->StrongholdSiteAtCell(Cell.X, Cell.Y, SiteSize));
		TestEqual(
			TEXT("that site is Imperious"),
			static_cast<uint8>(SiteSize),
			static_cast<uint8>(ESolidStrongholdSize::Imperious));
		FIntPoint Missing = FIntPoint::ZeroValue;
		TestFalse(
			TEXT("Meagre is not a site"),
			FSolidAutoPlaythrough::StrongholdSiteCell(
				*Chart,
				ESolidStrongholdSize::Meagre,
				Missing));

		FSolidWorldLocation Gate;
		TestTrue(TEXT("Dragongate is on the chart"), Chart->FindLocation(SolidTowns::Dragongate, Gate));
		TestTrue(TEXT("Dragongate has cells"), Gate.Cells.Num() > 0);
		if (Gate.Cells.Num() > 0)
		{
			const FVector2D WorldMin(0.f, 0.f);
			const FVector2D WorldMax(102400.f, 102400.f);
			FVector2D Opening = FVector2D::ZeroVector;
			FVector2D Walk = FVector2D::ZeroVector;
			FVector2D Center = FVector2D::ZeroVector;
			TestTrue(
				TEXT("the gate has an opening"),
				SolidWallgate::GateCenterXY(
					Chart,
					SolidTowns::Dragongate,
					WorldMin,
					WorldMax,
					Opening));
			TestTrue(
				TEXT("a Dragongate cell walks the opening"),
				FSolidAutoPlaythrough::WalkPointForCell(
					*Chart,
					Gate.Cells[0],
					WorldMin,
					WorldMax,
					Walk));
			TestTrue(TEXT("that walk is the opening"), Walk.Equals(Opening, 0.1f));
			TestTrue(
				TEXT("that cell also has a centre"),
				Chart->CellToWorldXY(Gate.Cells[0], WorldMin, WorldMax, Center));
			if (Gate.Cells.Num() > 1)
			{
				TestFalse(
					TEXT("the opening is not a cell centre"),
					Opening.Equals(Center, 1.f));
			}
			TestTrue(
				TEXT("grass at the site keeps its centre"),
				FSolidAutoPlaythrough::WalkPointForCell(*Chart, Cell, WorldMin, WorldMax, Walk));
			TestTrue(
				TEXT("the site centre is not the gate"),
				Chart->CellToWorldXY(Cell, WorldMin, WorldMax, Center)
				&& Walk.Equals(Center, 0.1f));
		}
	}

	SolidStronghold::FSolidTower Tower;
	Tower.Size = ESolidStrongholdSize::Imperious;
	Tower.BindPlan();
	TestEqual(TEXT("Imperious has ten floors"), Tower.FloorCount(), 10);

	TArray<FVector2D> Up;
	TArray<FVector2D> Down;
	TestTrue(
		TEXT("the climb walks floor 1 to 10"),
		FSolidAutoPlaythrough::TowerWalkPoints(Tower, 1, 10, Up));
	TestTrue(
		TEXT("the descent walks floor 10 to 1"),
		FSolidAutoPlaythrough::TowerWalkPoints(Tower, 10, 1, Down));
	TestTrue(TEXT("the climb has more than one flight"), Up.Num() > Tower.Plan().StairTreadCount);
	TestEqual(TEXT("the way down is the same length"), Down.Num(), Up.Num());
	if (Up.Num() > 0)
	{
		TestTrue(
			TEXT("the climb starts at the door"),
			Up[0].Equals(Tower.DoorHexCenter(), 0.1f));
	}
	if (Up.Num() == Down.Num())
	{
		for (int32 Index = 0; Index < Up.Num(); ++Index)
		{
			TestTrue(
				TEXT("the descent is the climb reversed"),
				Down[Index].Equals(Up[Up.Num() - 1 - Index], 0.1f));
		}
	}

	TArray<FVector2D> Bad;
	TestFalse(TEXT("floor 0 is not a story floor"), FSolidAutoPlaythrough::TowerWalkPoints(Tower, 0, 10, Bad));
	TestFalse(TEXT("floor 11 is past the cap"), FSolidAutoPlaythrough::TowerWalkPoints(Tower, 1, 11, Bad));
	TestTrue(
		TEXT("a stair point is closer than a road cell"),
		FSolidAutoPlaythrough::TowerArriveCm < FSolidAutoPlaythrough::ArriveCm);
	return true;
}

#endif
