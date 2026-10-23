#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Company/SolidCaptain.h"
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
	TArray<FSolidAutoPlaythrough::EStep> Steps;
	FSolidAutoPlaythrough::CollectSteps(Steps);
	TestEqual(TEXT("stage 1 has two steps"), Steps.Num(), 2);
	if (Steps.Num() < 2)
	{
		return false;
	}

	const TCHAR* Names[] = {
		TEXT("Walk to the monster encounter"),
		TEXT("Wake in the cave"),
	};
	const FSolidAutoPlaythrough::EStep Kinds[] = {
		FSolidAutoPlaythrough::EStep::WalkToMonsterEncounter,
		FSolidAutoPlaythrough::EStep::WakeInCave,
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		TestEqual(
			TEXT("step kind"),
			static_cast<uint8>(Steps[Index]),
			static_cast<uint8>(Kinds[Index]));
		TestEqual(TEXT("step name"), FString(FSolidAutoPlaythrough::NameOf(Steps[Index])), FString(Names[Index]));
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

	Character->FaceTrueSight(90.f);
	TestTrue(
		TEXT("true sight faces the walk"),
		FMath::IsNearlyEqual(Character->GetActorRotation().Yaw, 90.f, 0.1f));
	return true;
}

#endif
