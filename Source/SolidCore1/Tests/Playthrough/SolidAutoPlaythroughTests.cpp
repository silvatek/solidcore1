#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Company/SolidCaptain.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Inventory/SolidStorage.h"
#include "Menus/SolidMainMenu.h"
#include "Events/SolidEncounter.h"
#include "Events/SolidEncounters.h"
#include "Lairs/SolidLair.h"
#include "Playthrough/SolidAutoPlaythrough.h"
#include "Sight/SolidSight.h"
#include "SolidCharacter.h"
#include "SolidGameState.h"
#include "Towns/SolidTowns.h"
#include "Strongholds/SolidStronghold.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughStepsTest,
	"SolidCore1.AutoPlaythrough.Steps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughStepsTest::RunTest(const FString& Parameters)
{
	const TCHAR* Names[] = {
		TEXT("Walk to Iglin Bridge encounter"),
		TEXT("Wake in the cave"),
		TEXT("Disable the Iglin Bridge encounter"),
		TEXT("Move to the stand"),
		TEXT("Craft the torch"),
		TEXT("Equip the torch"),
		TEXT("Equip the sword"),
		TEXT("Move to the entrance"),
		TEXT("Leave the cave"),
		TEXT("Take playshot"),
		TEXT("Walk to Iglin"),
		TEXT("Close the driven-out message"),
		TEXT("Walk to the Iglin tunnels"),
		TEXT("Enter the Iglin tunnels"),
		TEXT("Move to the Iglin tunnels exit"),
		TEXT("Leave the Iglin tunnels"),
		TEXT("Walk to Relion"),
		TEXT("Walk to Kanfold"),
		TEXT("Walk to Dragongate"),
	};
	const FSolidAutoPlaythrough::EStepType Types[] = {
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::WakeInStronghold,
		FSolidAutoPlaythrough::EStepType::DisableEncounter,
		FSolidAutoPlaythrough::EStepType::MoveTo,
		FSolidAutoPlaythrough::EStepType::CraftItem,
		FSolidAutoPlaythrough::EStepType::EquipItem,
		FSolidAutoPlaythrough::EStepType::EquipItem,
		FSolidAutoPlaythrough::EStepType::MoveTo,
		FSolidAutoPlaythrough::EStepType::ExitStronghold,
		FSolidAutoPlaythrough::EStepType::TakePlayshot,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::CloseMessage,
		FSolidAutoPlaythrough::EStepType::WalkTo,
		FSolidAutoPlaythrough::EStepType::EnterLair,
		FSolidAutoPlaythrough::EStepType::MoveTo,
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

	const FSolidAutoPlaythrough::FStep& EncounterWalk = FSolidAutoPlaythrough::Script[0];
	TestEqual(
		TEXT("the opening row walks to a mark"),
		static_cast<uint8>(EncounterWalk.Walk),
		static_cast<uint8>(FSolidAutoPlaythrough::EWalk::Marker));
	TestEqual(
		TEXT("that mark is an encounter"),
		static_cast<uint8>(EncounterWalk.MarkKind),
		static_cast<uint8>(ESolidWorldChartMarkKind::Encounter));
	TestEqual(
		TEXT("the mark is Iglin Bridge"),
		FString(EncounterWalk.MarkName),
		FString(TEXT("Iglin Bridge")));

	const TCHAR* TownWalks[] = {
		TEXT("Walk to Iglin"),
		TEXT("Walk to Relion"),
		TEXT("Walk to Kanfold"),
		TEXT("Walk to Dragongate"),
	};
	const TCHAR* TownMarks[] = {
		TEXT("Iglin"),
		TEXT("Relion"),
		TEXT("Kanfold"),
		TEXT("Dragongate"),
	};
	for (int32 TownIndex = 0; TownIndex < UE_ARRAY_COUNT(TownWalks); ++TownIndex)
	{
		const FSolidAutoPlaythrough::FStep* TownWalk = nullptr;
		for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
		{
			if (FCString::Strcmp(FSolidAutoPlaythrough::Script[Index].Name, TownWalks[TownIndex]) == 0)
			{
				TownWalk = &FSolidAutoPlaythrough::Script[Index];
				break;
			}
		}
		TestTrue(TEXT("the script walks to that town"), TownWalk != nullptr);
		TestEqual(
			TEXT("the town walk is a marker walk"),
			static_cast<uint8>(TownWalk ? TownWalk->Walk : FSolidAutoPlaythrough::EWalk::Hexes),
			static_cast<uint8>(FSolidAutoPlaythrough::EWalk::Marker));
		TestEqual(
			TEXT("that mark is a town"),
			static_cast<uint8>(TownWalk ? TownWalk->MarkKind : ESolidWorldChartMarkKind::Encounter),
			static_cast<uint8>(ESolidWorldChartMarkKind::Town));
		TestEqual(
			TEXT("the mark name is the town"),
			TownWalk ? FString(TownWalk->MarkName) : FString(),
			FString(TownMarks[TownIndex]));
	}

	const FSolidAutoPlaythrough::FStep* TunnelWalk = nullptr;
	for (int32 Index = 0; Index < FSolidAutoPlaythrough::ScriptCount; ++Index)
	{
		if (FCString::Strcmp(FSolidAutoPlaythrough::Script[Index].Name, TEXT("Walk to the Iglin tunnels")) == 0)
		{
			TunnelWalk = &FSolidAutoPlaythrough::Script[Index];
			break;
		}
	}
	TestTrue(TEXT("the script walks to the Iglin tunnels"), TunnelWalk != nullptr);
	TestEqual(
		TEXT("the tunnels walk is a marker walk"),
		static_cast<uint8>(TunnelWalk ? TunnelWalk->Walk : FSolidAutoPlaythrough::EWalk::Hexes),
		static_cast<uint8>(FSolidAutoPlaythrough::EWalk::Marker));
	TestEqual(
		TEXT("that mark is a lair entrance"),
		static_cast<uint8>(TunnelWalk ? TunnelWalk->MarkKind : ESolidWorldChartMarkKind::Town),
		static_cast<uint8>(ESolidWorldChartMarkKind::LairEntrance));
	TestEqual(
		TEXT("the mark is Iglin tunnels"),
		TunnelWalk ? FString(TunnelWalk->MarkName) : FString(),
		FString(TEXT("Iglin tunnels")));

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
		else if (FCString::Strcmp(Step.Name, TEXT("Move to the Iglin tunnels exit")) == 0)
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
		TEXT("Full-playthrough"),
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

	TestEqual(TEXT("the HUD line"), FString(FSolidAutoPlaythrough::Label()), FString(TEXT("Auto playthrough")));
	TestEqual(TEXT("key 5 is Full-playthrough"), Entries[4].Label, FString(TEXT("Full-playthrough")));
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

	USolidGameState* Stepped = NewObject<USolidGameState>();
	USolidCaptain* SteppedCaptain = NewObject<USolidCaptain>();
	TestNotNull(TEXT("step state"), Stepped);
	TestNotNull(TEXT("step captain"), SteppedCaptain);
	if (!Stepped || !SteppedCaptain)
	{
		return false;
	}
	FSolidAutoPlaythrough Steps;
	Steps.NoteCaptainPlace(*Stepped, FVector::ZeroVector);
	Steps.NoteCaptainPlace(*Stepped, FVector(0.f, 5000.f, 0.f));
	TestFalse(TEXT("a place before arrival is not a move"), Stepped->bAutoPlaythroughDim);
	SolidEvents::NotifyOccupiedTown(*Stepped, *SteppedCaptain, SolidTowns::Iglin);
	const FVector Arrival(100.f, 200.f, 0.f);
	Steps.NoteCaptainPlace(*Stepped, Arrival);
	TestFalse(TEXT("the arrival place is not a move"), Stepped->bAutoPlaythroughDim);
	Steps.NoteCaptainPlace(*Stepped, Arrival + FVector(FSolidAutoPlaythrough::MovedCm + 1.f, 0.f, 0.f));
	TestTrue(TEXT("a step away dims the row"), Stepped->bAutoPlaythroughDim);
	TestFalse(TEXT("Full-playthrough cannot start after a move"), FSolidAutoPlaythrough::CanStart(*Stepped));
	TestTrue(TEXT("Companion parade can start after a move"), FSolidAutoPlaythrough::CanStart(*Stepped, 1));

	USolidGameState* Started = NewObject<USolidGameState>();
	USolidCaptain* StartedCaptain = NewObject<USolidCaptain>();
	FSolidAutoPlaythrough Play;
	TestFalse(TEXT("the script is idle"), Play.IsRunning());
	Play.Start(*Started);
	TestTrue(TEXT("choosing the row starts the script"), Play.IsRunning());
	TestFalse(TEXT("choosing the row leaves it bright"), Started->bAutoPlaythroughDim);
	TestTrue(TEXT("a second start is still allowed"), FSolidAutoPlaythrough::CanStart(*Started));
	Play.Stop();
	TestFalse(TEXT("Esc clears the script"), Play.IsRunning());
	TestFalse(TEXT("Esc before a move leaves the flag clear"), Started->bAutoPlaythroughDim);
	TestTrue(TEXT("Esc before a move leaves the row available"), FSolidAutoPlaythrough::CanStart(*Started));

	Play.Start(*Started);
	SolidEvents::NotifyOccupiedTown(*Started, *StartedCaptain, SolidTowns::Iglin);
	Play.NoteCaptainPlace(*Started, Arrival);
	Play.NoteCaptainPlace(*Started, Arrival + FVector(FSolidAutoPlaythrough::MovedCm + 10.f, 0.f, 0.f));
	TestTrue(TEXT("a script move leaves the script running"), Play.IsRunning());
	TestTrue(TEXT("a script move dims a later start"), Started->bAutoPlaythroughDim);
	SolidEvents::NotifyOccupiedTown(*Started, *StartedCaptain, INDEX_NONE);
	TestTrue(TEXT("leaving Iglin does not stop a running script"), Play.IsRunning());

	Play.Stop();
	TestFalse(TEXT("Esc clears the script after a move"), Play.IsRunning());
	TestFalse(TEXT("the dim row does not start again"), FSolidAutoPlaythrough::CanStart(*Started));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughLaunchTest,
	"SolidCore1.AutoPlaythrough.Launch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughLaunchTest::RunTest(const FString& Parameters)
{
	FString Name;
	TestTrue(
		TEXT("-Autoplay=Full-playthrough names that script"),
		FSolidAutoPlaythrough::LaunchName(TEXT("-Autoplay=Full-playthrough"), Name));
	TestEqual(TEXT("the name is Full-playthrough"), Name, FString(TEXT("Full-playthrough")));
	TestEqual(TEXT("that name is the first script"), FSolidAutoPlaythrough::FindScript(*Name), 0);

	TestTrue(
		TEXT("a quoted name keeps its space"),
		FSolidAutoPlaythrough::LaunchName(TEXT("-Autoplay=\"Companion parade\""), Name));
	TestEqual(TEXT("the name is Companion parade"), Name, FString(TEXT("Companion parade")));
	TestEqual(TEXT("that name is the second script"), FSolidAutoPlaythrough::FindScript(*Name), 1);

	TestFalse(TEXT("an ordinary launch names nothing"), FSolidAutoPlaythrough::LaunchName(TEXT("-log"), Name));
	TestFalse(TEXT("a missing command line names nothing"), FSolidAutoPlaythrough::LaunchName(nullptr, Name));
	TestEqual(TEXT("an unknown name has no script"), FSolidAutoPlaythrough::FindScript(TEXT("Missing")), INDEX_NONE);

	USolidGameState* State = NewObject<USolidGameState>();
	TestNotNull(TEXT("play state"), State);
	if (!State)
	{
		return false;
	}
	FSolidAutoPlaythrough Play;
	Play.Start(*State, 1);
	TestTrue(TEXT("the named script is running"), Play.IsRunning());
	TestEqual(
		TEXT("the running script is Companion parade"),
		FString(Play.RunningName()),
		FString(TEXT("Companion parade")));
	Play.Start(*State);
	TestEqual(
		TEXT("the menu starts Full-playthrough"),
		FString(Play.RunningName()),
		FString(TEXT("Full-playthrough")));
	TestFalse(TEXT("the menu start is not a launch"), Play.HasHandledLaunch());

	TestTrue(TEXT("the launch starts its script"), Play.BeginLaunch(*State, 1));
	TestTrue(TEXT("the launch is remembered"), Play.HasHandledLaunch());
	TestEqual(
		TEXT("the launch selected Companion parade"),
		FString(Play.RunningName()),
		FString(TEXT("Companion parade")));
	TestFalse(TEXT("a running launch does not quit yet"), Play.ShouldQuit());
	TestFalse(TEXT("a second launch leaves the script"), Play.BeginLaunch(*State, 0));
	TestEqual(
		TEXT("the running script stays Companion parade"),
		FString(Play.RunningName()),
		FString(TEXT("Companion parade")));
	Play.Stop();
	TestTrue(TEXT("a finished launch quits"), Play.ShouldQuit());
	Play.ClearQuit();
	TestFalse(TEXT("the quit is asked for once"), Play.ShouldQuit());

	FSolidAutoPlaythrough Unknown;
	TestFalse(TEXT("an unknown script does not start"), Unknown.BeginLaunch(*State, INDEX_NONE));
	TestFalse(TEXT("an unknown script leaves the runner idle"), Unknown.IsRunning());
	TestTrue(TEXT("an unknown script is still remembered"), Unknown.HasHandledLaunch());
	TestFalse(TEXT("a later launch does not start after that"), Unknown.BeginLaunch(*State, 0));
	TestFalse(TEXT("the runner stays idle"), Unknown.IsRunning());
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
		TEXT("the site walk is a marker walk"),
		static_cast<uint8>(Site->Walk),
		static_cast<uint8>(FSolidAutoPlaythrough::EWalk::Marker));
	TestEqual(
		TEXT("that mark is a stronghold site"),
		static_cast<uint8>(Site->MarkKind),
		static_cast<uint8>(ESolidWorldChartMarkKind::StrongholdSite));
	TestEqual(
		TEXT("the mark is Imperious"),
		FString(Site->MarkName),
		FString(TEXT("Imperious")));

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
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughScriptsTest,
	"SolidCore1.AutoPlaythrough.Scripts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughScriptsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("two scripts"), FSolidAutoPlaythrough::ScriptsCount, 2);

	const FSolidAutoPlaythrough::FScript& Full = FSolidAutoPlaythrough::Scripts[0];
	TestEqual(TEXT("the first script is Full-playthrough"), FString(Full.Name), FString(TEXT("Full-playthrough")));
	TestTrue(TEXT("Full-playthrough uses the opening list"), Full.Steps == FSolidAutoPlaythrough::Script);
	TestEqual(TEXT("Full-playthrough has every opening step"), Full.StepCount, FSolidAutoPlaythrough::ScriptCount);
	TestTrue(TEXT("Full-playthrough is on the menu"), Full.bOnMenu);
	TestTrue(TEXT("Full-playthrough disables after a move"), Full.bDisableAfterMove);

	const FSolidAutoPlaythrough::FScript& Parade = FSolidAutoPlaythrough::Scripts[1];
	TestEqual(TEXT("the second script is Companion parade"), FString(Parade.Name), FString(TEXT("Companion parade")));
	TestTrue(TEXT("Companion parade uses its own list"), Parade.Steps == FSolidAutoPlaythrough::Parade);
	TestEqual(TEXT("Companion parade has its steps"), Parade.StepCount, FSolidAutoPlaythrough::ParadeCount);
	TestFalse(TEXT("Companion parade stays off the menu"), Parade.bOnMenu);
	TestFalse(TEXT("Companion parade stays available after a move"), Parade.bDisableAfterMove);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidAutoPlaythroughParadeTest,
	"SolidCore1.AutoPlaythrough.Parade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidAutoPlaythroughParadeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ten parade steps"), FSolidAutoPlaythrough::ParadeCount, 10);
	const FSolidAutoPlaythrough::FStep* Steps = FSolidAutoPlaythrough::Parade;

	TestEqual(
		TEXT("portal comes first"),
		static_cast<uint8>(Steps[0].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::PortalToMarker));
	TestEqual(
		TEXT("the mark is a town"),
		static_cast<uint8>(Steps[0].MarkKind),
		static_cast<uint8>(ESolidWorldChartMarkKind::Town));
	TestEqual(TEXT("the mark is Dragongate"), FString(Steps[0].MarkName), FString(TEXT("Dragongate")));
	TestTrue(TEXT("no east or west offset"), FMath::IsNearlyEqual(Steps[0].OffsetXCm, 0.f));
	TestTrue(TEXT("ten metres south"), FMath::IsNearlyEqual(Steps[0].OffsetYCm, -1000.f));
	TestEqual(
		TEXT("a playshot follows the portal"),
		static_cast<uint8>(Steps[1].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::TakePlayshot));

	TestEqual(
		TEXT("Sam is placed"),
		static_cast<uint8>(Steps[2].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::BuildSquad));
	TestEqual(TEXT("the companion is Sam"), FString(Steps[2].CompanionName), FString(TEXT("Sam")));
	TestTrue(TEXT("Sam closes the squad builder"), Steps[2].bClosePage);
	TestEqual(
		TEXT("a playshot follows Sam"),
		static_cast<uint8>(Steps[3].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::TakePlayshot));

	TestEqual(
		TEXT("the formation is selected"),
		static_cast<uint8>(Steps[4].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::SelectBattlePlan));
	TestEqual(TEXT("the formation is Parade"), FString(Steps[4].PlanName), FString(TEXT("Parade")));
	TestEqual(
		TEXT("a playshot follows Parade"),
		static_cast<uint8>(Steps[5].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::TakePlayshot));

	TestEqual(
		TEXT("sight is enabled"),
		static_cast<uint8>(Steps[6].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::EnableSight));
	TestEqual(
		TEXT("the sight is True"),
		static_cast<uint8>(Steps[6].Sight),
		static_cast<uint8>(ESolidSight::True));

	TestEqual(
		TEXT("the facing is set"),
		static_cast<uint8>(Steps[7].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::FaceDirection));
	TestTrue(
		TEXT("facing south"),
		FMath::IsNearlyEqual(Steps[7].FacingYawDeg, SolidTowns::SignFacesSouthYawDeg));

	TestEqual(
		TEXT("the wait is its own step"),
		static_cast<uint8>(Steps[8].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::Pause));
	TestTrue(TEXT("the wait is three seconds"), FMath::IsNearlyEqual(Steps[8].PauseSeconds, 3.f));

	TestEqual(
		TEXT("the playshot is last"),
		static_cast<uint8>(Steps[9].Type),
		static_cast<uint8>(FSolidAutoPlaythrough::EStepType::TakePlayshot));
	return true;
}

#endif
