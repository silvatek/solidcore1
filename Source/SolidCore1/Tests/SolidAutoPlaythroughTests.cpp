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
	TestEqual(
		TEXT("the script is the static list"),
		FSolidAutoPlaythrough::ScriptCount,
		static_cast<int32>(UE_ARRAY_COUNT(Names)));
	TestEqual(
		TEXT("each row has a type"),
		static_cast<int32>(UE_ARRAY_COUNT(Types)),
		static_cast<int32>(UE_ARRAY_COUNT(Names)));
	if (FSolidAutoPlaythrough::ScriptCount != static_cast<int32>(UE_ARRAY_COUNT(Names)))
	{
		return false;
	}

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		const FSolidAutoPlaythrough::FStep& Step = FSolidAutoPlaythrough::Script[Index];
		TestEqual(
			TEXT("step type"),
			static_cast<uint8>(Step.Type),
			static_cast<uint8>(Types[Index]));
		TestEqual(TEXT("step name"), FString(Step.Name), FString(Names[Index]));
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
		if (Step.Type == FSolidAutoPlaythrough::EStepType::EnterLair)
		{
			Enter = &Step;
		}
		else if (Step.Type == FSolidAutoPlaythrough::EStepType::ExitLair)
		{
			Leave = &Step;
		}
		else if (Step.Type == FSolidAutoPlaythrough::EStepType::WalkTo
			&& Step.Walk == FSolidAutoPlaythrough::EWalk::Lair)
		{
			Run = &Step;
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
		TEXT("enter is the X mouth"),
		static_cast<int32>(Enter ? Enter->EndMarker : 0),
		static_cast<int32>(USolidWorldChart::XMarker));
	TestEqual(
		TEXT("the run walks to the Y mouth"),
		static_cast<int32>(Run ? Run->EndMarker : 0),
		static_cast<int32>(USolidWorldChart::YMarker));
	TestEqual(
		TEXT("leave is the Y mouth"),
		static_cast<int32>(Leave ? Leave->EndMarker : 0),
		static_cast<int32>(USolidWorldChart::YMarker));

	const FIntPoint MouthX = SolidLair::CellOf(USolidWorldChart::XMarker);
	const FIntPoint MouthY = SolidLair::CellOf(USolidWorldChart::YMarker);
	TArray<FIntPoint> RunCells;
	TestTrue(
		TEXT("the straight run joins the mouths"),
		FSolidAutoPlaythrough::LairCells(
			SolidLair::IdIglinTunnels,
			USolidWorldChart::XMarker,
			USolidWorldChart::YMarker,
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
		TEXT("the run walks back to the X mouth"),
		FSolidAutoPlaythrough::LairCells(
			SolidLair::IdIglinTunnels,
			USolidWorldChart::YMarker,
			USolidWorldChart::XMarker,
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
			USolidWorldChart::XMarker,
			USolidWorldChart::XMarker,
			Same));
	TestEqual(TEXT("one cell"), Same.Num(), 1);

	TArray<FIntPoint> Missing;
	TestFalse(
		TEXT("an unknown lair has no path"),
		FSolidAutoPlaythrough::LairCells(
			TEXT("00000000"),
			USolidWorldChart::XMarker,
			USolidWorldChart::YMarker,
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
		TestEqual(
			TEXT("the town walks are the last rows"),
			Index,
			FSolidAutoPlaythrough::ScriptCount - static_cast<int32>(UE_ARRAY_COUNT(TownIds)) + TownSteps);
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

#endif
