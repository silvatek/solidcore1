#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Company/SolidCaptain.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Inventory/SolidStorage.h"
#include "Menus/SolidMainMenu.h"
#include "Playthrough/SolidAutoPlaythrough.h"
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
		TEXT("Walk to the stand"),
		TEXT("Craft the torch"),
		TEXT("Put the torch in the main hand"),
		TEXT("Walk to the entrance"),
		TEXT("Leave the cave"),
		TEXT("Walk to Iglin"),
		TEXT("Close the driven-out message"),
	};
	const FSolidAutoPlaythrough::EStepType Types[] = {
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::WakeInStronghold,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::CraftItem,
		FSolidAutoPlaythrough::EStepType::CraftItem,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::ExitStronghold,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::CloseMessage,
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
	const int32 Hand = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 Stick = SolidStorage::Index(1, 1);
	const int32 Pitch = SolidStorage::Index(0, 2);
	const int32 Sword = SolidStorage::Index(0, 0);
	const int32 Boots = SolidStorage::Index(1, 0);
	const int32 Coins = SolidStorage::Index(0, 1);
	const int32 Empty = SolidStorage::Index(1, 2);
	if (Body == INDEX_NONE || Hand == INDEX_NONE || Stick == INDEX_NONE || Pitch == INDEX_NONE
		|| Sword == INDEX_NONE || Boots == INDEX_NONE || Coins == INDEX_NONE || Empty == INDEX_NONE)
	{
		AddError(TEXT("a slot index is missing"));
		return false;
	}

	TestTrue(TEXT("the body starts in rags"), Loadout.Worn[Body].Is(SolidItemType::IdRags));
	TestTrue(TEXT("the hands start empty"), Loadout.Worn[Hand].IsEmpty());
	TestTrue(TEXT("one stick and one pitch make a torch"), FSolidAutoPlaythrough::CraftTorch(Loadout, Chest));
	TestTrue(TEXT("the torch goes to the main hand"), FSolidAutoPlaythrough::PutTorchInMainHand(Loadout));

	TestTrue(TEXT("rags stay on the body"), Loadout.Worn[Body].Is(SolidItemType::IdRags));
	TestEqual(TEXT("one set of rags"), Loadout.Worn[Body].Count, 1);
	TestTrue(TEXT("the main hand holds the torch"), Loadout.Worn[Hand].Is(SolidItemType::IdTorch));
	TestEqual(TEXT("one torch"), Loadout.Worn[Hand].Count, 1);
	for (int32 Index = 0; Index < SolidInventory::WornCount; ++Index)
	{
		if (Index == Body || Index == Hand)
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

	TestTrue(TEXT("the sword stays"), Chest.Slots[Sword].Is(SolidItemType::IdRustySword));
	TestEqual(TEXT("one sword"), Chest.Slots[Sword].Count, 1);
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

#endif
