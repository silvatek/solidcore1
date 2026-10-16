#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidEvents.h"
#include "SolidGameState.h"
#include "SolidGameMode.h"
#include "Journal/SolidJournal.h"
#include "SolidSight.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidEventTestPrivate
{
	const SolidEvents::FEvent* FindEvent(const TCHAR* EventName)
	{
		if (!EventName)
		{
			return nullptr;
		}
		for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
		{
			const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
			if (FCString::Strcmp(Event.Name, EventName) == 0)
			{
				return &Event;
			}
		}
		return nullptr;
	}

	bool EventEnablesRavenSight(const TCHAR* EventName)
	{
		for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
		{
			const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
			if (FCString::Strcmp(Event.Name, EventName) != 0)
			{
				continue;
			}
			const int32 Count = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
			for (int32 ChangeIndex = 0; ChangeIndex < Count; ++ChangeIndex)
			{
				const SolidEvents::FChange& Change = Event.Changes[ChangeIndex];
				if (Change.Kind == SolidEvents::EChange::EnableSight && Change.Sight == ESolidSight::Raven)
				{
					return true;
				}
			}
			return false;
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsStartAndTownsTest,
	"SolidCore1.Events.EnterTown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsStartAndTownsTest::RunTest(const FString& Parameters)
{
	int32 EnterTownCount = 0;
	for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
	{
		if (SolidEvents::Definitions[Index].Trigger == SolidEvents::ETrigger::EnterTown)
		{
			++EnterTownCount;
		}
	}
	TestEqual(TEXT("four enter-town events"), EnterTownCount, 4);
	TestEqual(TEXT("five events"), SolidEvents::DefinitionCount, 5);
	TestEqual(TEXT("roster is the company"), SolidEvents::RosterCount, 5);
	TestEqual(TEXT("slot 0 is Sam"), FString(SolidEvents::RosterName(0)), FString(TEXT("Sam")));
	TestEqual(TEXT("slot 1 is Alex"), FString(SolidEvents::RosterName(1)), FString(TEXT("Alex")));
	TestEqual(TEXT("slot 2 is Max"), FString(SolidEvents::RosterName(2)), FString(TEXT("Max")));
	TestEqual(TEXT("slot 3 is Morgan"), FString(SolidEvents::RosterName(3)), FString(TEXT("Morgan")));
	TestEqual(TEXT("slot 4 is Quinn"), FString(SolidEvents::RosterName(4)), FString(TEXT("Quinn")));
	TestNull(TEXT("slot 5 is empty"), SolidEvents::RosterName(5));

	FGameState State;
	TestEqual(TEXT("party starts empty"), State.MaxPartySize, 0);
	TestEqual(TEXT("no companions yet"), SolidEvents::PartyCount(State), 0);
	TestTrue(TEXT("true sight is on"), SolidEvents::IsSightEnabled(State, ESolidSight::True));
	TestFalse(TEXT("raven sight is locked"), SolidEvents::IsSightEnabled(State, ESolidSight::Raven));
	TestFalse(
		TEXT("F9 cannot reach raven sight yet"),
		SolidEvents::IsSightEnabled(State, SolidSight::Toggle(ESolidSight::True)));

	const SolidEvents::FResult Nowhere = SolidEvents::NotifyOccupiedTown(State, INDEX_NONE);
	TestFalse(TEXT("open ground fires nothing"), Nowhere.bFired);
	TestEqual(TEXT("still an empty party"), State.MaxPartySize, 0);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(State, 0);
	TestTrue(TEXT("entering town 0 fires"), Iglin.bFired);
	TestFalse(TEXT("Iglin does not switch sight"), Iglin.bSetSight);
	TestFalse(TEXT("Iglin leaves the bridge broken"), Iglin.bRepairBridge);
	TestFalse(TEXT("the Iglin bridge starts broken"), State.bIglinBridgeRepaired);
	TestFalse(TEXT("Iglin leaves raven sight locked"), State.bRavenSightEnabled);
	TestEqual(TEXT("Iglin leaves the party empty"), State.MaxPartySize, 0);
	TestEqual(TEXT("Iglin is recorded"), State.FiredCount, 1);

	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(State, 1);
	TestTrue(TEXT("entering town 1 fires"), Relion.bFired);
	TestTrue(TEXT("town 1 switches sight"), Relion.bSetSight);
	TestEqual(
		TEXT("town 1 enables raven sight"),
		static_cast<uint8>(Relion.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("raven sight unlocked"), SolidEvents::IsSightEnabled(State, ESolidSight::Raven));
	TestTrue(TEXT("true sight stays available"), SolidEvents::IsSightEnabled(State, ESolidSight::True));
	TestTrue(TEXT("Relion repairs the bridge"), Relion.bRepairBridge);
	TestTrue(TEXT("the Iglin bridge is repaired"), State.bIglinBridgeRepaired);
	TestEqual(TEXT("town 1 party size"), State.MaxPartySize, 1);
	TestEqual(TEXT("town 1 companion count"), SolidEvents::PartyCount(State), 1);
	TestEqual(TEXT("Sam joins at Relion"), FString(SolidEvents::RosterName(0)), FString(TEXT("Sam")));

	const SolidEvents::FResult StillRelion = SolidEvents::NotifyOccupiedTown(State, 1);
	TestFalse(TEXT("staying in town 1 does not fire again"), StillRelion.bFired);

	const SolidEvents::FResult Leave = SolidEvents::NotifyOccupiedTown(State, INDEX_NONE);
	TestFalse(TEXT("leaving town fires nothing"), Leave.bFired);
	TestEqual(TEXT("party size sticks after leaving"), State.MaxPartySize, 1);

	const SolidEvents::FResult RelionAgain = SolidEvents::NotifyOccupiedTown(State, 1);
	TestFalse(TEXT("town 1 event already fired"), RelionAgain.bFired);
	TestFalse(TEXT("re-entering Relion does not repair again"), RelionAgain.bRepairBridge);
	TestTrue(TEXT("the repaired bridge stays"), State.bIglinBridgeRepaired);
	TestEqual(TEXT("re-entering Relion does not shrink the party"), State.MaxPartySize, 1);

	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(State, 2);
	TestTrue(TEXT("entering town 2 fires"), Kanfold.bFired);
	TestEqual(
		TEXT("town 2 sight follows the Kanfold event"),
		Kanfold.bSetSight,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Kanfold")));
	TestTrue(TEXT("raven sight still unlocked"), SolidEvents::IsSightEnabled(State, ESolidSight::Raven));
	TestEqual(TEXT("town 2 party size"), State.MaxPartySize, 2);
	TestEqual(TEXT("Sam and Alex"), SolidEvents::PartyCount(State), 2);
	TestEqual(TEXT("Alex is the second companion"), FString(SolidEvents::RosterName(1)), FString(TEXT("Alex")));

	FGameState Skipped;
	const SolidEvents::FResult OtherTown = SolidEvents::NotifyOccupiedTown(Skipped, 4);
	TestFalse(TEXT("Jethan has no event"), OtherTown.bFired);
	TestEqual(TEXT("Jethan leaves the party empty"), Skipped.MaxPartySize, 0);
	TestFalse(TEXT("Jethan does not unlock raven sight"), Skipped.bRavenSightEnabled);

	const SolidEvents::FResult KanfoldFirst = SolidEvents::NotifyOccupiedTown(Skipped, 2);
	TestTrue(TEXT("town 2 still fires if Relion was skipped"), KanfoldFirst.bFired);
	TestEqual(
		TEXT("Kanfold sight follows the event when Relion was skipped"),
		Skipped.bRavenSightEnabled,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Kanfold")));
	TestEqual(TEXT("town 2 still fields Sam and Alex"), SolidEvents::PartyCount(Skipped), 2);

	const SolidEvents::FResult Dragongate = SolidEvents::NotifyOccupiedTown(State, 5);
	TestTrue(TEXT("entering Dragongate fires"), Dragongate.bFired);
	TestEqual(
		TEXT("Dragongate sight follows the event"),
		Dragongate.bSetSight,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Dragongate")));
	TestTrue(TEXT("raven sight stays unlocked"), SolidEvents::IsSightEnabled(State, ESolidSight::Raven));
	TestEqual(TEXT("Dragongate party size"), State.MaxPartySize, SolidEvents::RosterCount);
	TestEqual(TEXT("the whole company"), SolidEvents::PartyCount(State), 5);

	SolidEvents::NotifyOccupiedTown(State, INDEX_NONE);
	const SolidEvents::FResult DragongateAgain = SolidEvents::NotifyOccupiedTown(State, 5);
	TestFalse(TEXT("re-entering Dragongate does not fire again"), DragongateAgain.bFired);
	TestEqual(TEXT("re-entering Dragongate keeps the party"), State.MaxPartySize, 5);

	FGameState GateFirst;
	const SolidEvents::FResult GateOnly = SolidEvents::NotifyOccupiedTown(GateFirst, 5);
	TestTrue(TEXT("Dragongate fires on its own"), GateOnly.bFired);
	TestEqual(
		TEXT("Dragongate sight follows the event on its own"),
		GateFirst.bRavenSightEnabled,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Dragongate")));
	TestEqual(TEXT("Dragongate fields the company"), SolidEvents::PartyCount(GateFirst), 5);
	SolidEvents::NotifyOccupiedTown(GateFirst, INDEX_NONE);
	const SolidEvents::FResult GateReturn = SolidEvents::NotifyOccupiedTown(GateFirst, 5);
	TestFalse(TEXT("Dragongate event already fired"), GateReturn.bFired);
	const SolidEvents::FResult KanfoldLater = SolidEvents::NotifyOccupiedTown(GateFirst, 2);
	TestTrue(TEXT("Kanfold still fires after Dragongate"), KanfoldLater.bFired);
	TestEqual(TEXT("Kanfold sets the party to Sam and Alex"), GateFirst.MaxPartySize, 2);
	const bool bEitherTownEnablesSight =
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Dragongate"))
		|| SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Kanfold"));
	TestEqual(
		TEXT("sight follows the Kanfold and Dragongate events"),
		GateFirst.bRavenSightEnabled,
		bEitherTownEnablesSight);

	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);
	if (GameMode)
	{
		TestEqual(TEXT("game starts with max party 0"), GameMode->GetMaxPartySize(), 0);
		TestTrue(TEXT("game starts in true sight"), GameMode->IsSightEnabled(ESolidSight::True));
		TestFalse(TEXT("game starts with raven sight locked"), GameMode->IsSightEnabled(ESolidSight::Raven));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsCaptainTitleTest,
	"SolidCore1.Events.CaptainTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsCaptainTitleTest::RunTest(const FString& Parameters)
{
	FGameState State;
	TestEqual(TEXT("starts as Outcast"), FString(State.CaptainTitle), FString(SolidEvents::TitleOutcast));

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(State, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	TestFalse(TEXT("Iglin does not retitle"), Iglin.bSetTitle);
	TestEqual(TEXT("Iglin stays Outcast"), FString(State.CaptainTitle), FString(SolidEvents::TitleOutcast));

	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(State, 1);
	TestTrue(TEXT("Relion sets the title"), Relion.bSetTitle);
	TestEqual(TEXT("Relion result is Vagrant"), FString(Relion.CaptainTitle), FString(SolidEvents::TitleVagrant));
	TestEqual(TEXT("Relion state is Vagrant"), FString(State.CaptainTitle), FString(SolidEvents::TitleVagrant));

	SolidEvents::NotifyOccupiedTown(State, INDEX_NONE);
	const SolidEvents::FResult RelionAgain = SolidEvents::NotifyOccupiedTown(State, 1);
	TestFalse(TEXT("Relion does not fire twice"), RelionAgain.bFired);
	TestEqual(TEXT("title stays Vagrant"), FString(State.CaptainTitle), FString(SolidEvents::TitleVagrant));

	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(State, 2);
	TestTrue(TEXT("Kanfold sets the title"), Kanfold.bSetTitle);
	TestEqual(TEXT("Kanfold is Ranger"), FString(State.CaptainTitle), FString(SolidEvents::TitleRanger));

	FGameState Skipped;
	const SolidEvents::FResult KanfoldFirst = SolidEvents::NotifyOccupiedTown(Skipped, 2);
	TestTrue(TEXT("Kanfold titles Ranger without Relion"), KanfoldFirst.bSetTitle);
	TestFalse(TEXT("Kanfold leaves the bridge broken"), KanfoldFirst.bRepairBridge);
	TestFalse(TEXT("skipping Relion leaves the bridge broken"), Skipped.bIglinBridgeRepaired);
	TestEqual(TEXT("skipped path is Ranger"), FString(Skipped.CaptainTitle), FString(SolidEvents::TitleRanger));
	SolidEvents::NotifyOccupiedTown(Skipped, 0);
	const SolidEvents::FResult RelionLater = SolidEvents::NotifyOccupiedTown(Skipped, 1);
	TestTrue(TEXT("Relion still fires later"), RelionLater.bSetTitle);
	TestTrue(TEXT("Relion later repairs the bridge"), RelionLater.bRepairBridge);
	TestTrue(TEXT("the skipped path repairs the bridge"), Skipped.bIglinBridgeRepaired);
	TestEqual(TEXT("Relion titles Vagrant"), FString(Skipped.CaptainTitle), FString(SolidEvents::TitleVagrant));

	const SolidEvents::FResult Dragongate = SolidEvents::NotifyOccupiedTown(State, 5);
	TestTrue(TEXT("Dragongate sets the title"), Dragongate.bSetTitle);
	TestEqual(TEXT("Dragongate is Captain"), FString(State.CaptainTitle), FString(SolidEvents::TitleCaptain));
	SolidEvents::NotifyOccupiedTown(State, INDEX_NONE);
	const SolidEvents::FResult DragongateAgain = SolidEvents::NotifyOccupiedTown(State, 5);
	TestFalse(TEXT("Dragongate does not fire twice"), DragongateAgain.bFired);
	TestEqual(TEXT("title stays Captain"), FString(State.CaptainTitle), FString(SolidEvents::TitleCaptain));

	FGameState GateFirst;
	const SolidEvents::FResult GateOnly = SolidEvents::NotifyOccupiedTown(GateFirst, 5);
	TestTrue(TEXT("Dragongate titles Captain on its own"), GateOnly.bSetTitle);
	TestEqual(TEXT("first visit is Captain"), FString(GateFirst.CaptainTitle), FString(SolidEvents::TitleCaptain));
	const SolidEvents::FResult KanfoldLater = SolidEvents::NotifyOccupiedTown(GateFirst, 2);
	TestTrue(TEXT("Kanfold still sets the title"), KanfoldLater.bSetTitle);
	TestEqual(TEXT("Kanfold after Dragongate is Ranger"), FString(GateFirst.CaptainTitle), FString(SolidEvents::TitleRanger));

	const SolidEvents::FEvent* RelionEvent = SolidEventTestPrivate::FindEvent(TEXT("Enter Relion"));
	TestNotNull(TEXT("Relion is listed"), RelionEvent);
	TestTrue(
		TEXT("journal records the repair"),
		RelionEvent && SolidJournal::Describe(*RelionEvent).Contains(TEXT("Iglin bridge repaired")));

	const SolidEvents::FEvent* DragongateEvent = SolidEventTestPrivate::FindEvent(TEXT("Enter Dragongate"));
	TestNotNull(TEXT("Dragongate is listed"), DragongateEvent);
	const FString Detail = DragongateEvent ? SolidJournal::Describe(*DragongateEvent) : FString();
	TestTrue(TEXT("journal titles Captain"), Detail.Contains(TEXT("Title Captain")));
	TestTrue(
		TEXT("journal lists the company"),
		Detail.Contains(TEXT("Party 5: Sam, Alex, Max, Morgan, Quinn")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsNumberedCellTest,
	"SolidCore1.Events.NumberedCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsNumberedCellTest::RunTest(const FString& Parameters)
{
	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 2 && Col == 4)
			{
				Line.AppendChar(TEXT('0'));
			}
			else if (Row == 4 && Col == 6)
			{
				Line.AppendChar(TEXT('1'));
			}
			else if (Row == 2 && Col == 8)
			{
				Line.AppendChar(TEXT('Z'));
			}
			else
			{
				Line.AppendChar(USolidWorldMap::GrassMarker);
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("G = Grassland (light green)\n");
	Text += TEXT("0 = Starting village, biome=Town\n");
	Text += TEXT("1 = Watch, biome=Town\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("synthetic map loads"), Map->LoadFromString(Text));

	const FVector2D WorldMin(0.f, 0.f);
	const FVector2D WorldMax(6400.f, 6400.f);

	FVector2D Iglin = FVector2D::ZeroVector;
	FVector2D Relion = FVector2D::ZeroVector;
	FVector2D Grass = FVector2D::ZeroVector;
	FVector2D Legacy = FVector2D::ZeroVector;
	TestTrue(TEXT("cell 0"), Map->CellToWorldXY(FIntPoint(4, 2), WorldMin, WorldMax, Iglin));
	TestTrue(TEXT("cell 1"), Map->CellToWorldXY(FIntPoint(6, 4), WorldMin, WorldMax, Relion));
	TestTrue(TEXT("grass cell"), Map->CellToWorldXY(FIntPoint(0, 0), WorldMin, WorldMax, Grass));
	TestTrue(TEXT("Z cell"), Map->CellToWorldXY(FIntPoint(8, 2), WorldMin, WorldMax, Legacy));

	TestEqual(TEXT("digit 0 is town 0"), Map->SampleLocationId(Iglin.X, Iglin.Y, WorldMin, WorldMax), 0);
	TestEqual(TEXT("digit 1 is town 1"), Map->SampleLocationId(Relion.X, Relion.Y, WorldMin, WorldMax), 1);
	TestEqual(TEXT("grass is not a town"), Map->SampleLocationId(Grass.X, Grass.Y, WorldMin, WorldMax), INDEX_NONE);
	TestEqual(TEXT("Z is not a numbered town"), Map->SampleLocationId(Legacy.X, Legacy.Y, WorldMin, WorldMax), INDEX_NONE);
	TestEqual(TEXT("cell query matches"), Map->GetLocationIdAtCell(4, 2), 0);

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	const float Half = 0.5f * 512.f * 200.f;
	const FVector2D GameMin(-Half, -Half);
	const FVector2D GameMax(Half, Half);
	FVector2D Start = FVector2D::ZeroVector;
	TestTrue(TEXT("start town"), Shipped->GetStartTownWorldXY(GameMin, GameMax, Start));
	TestEqual(
		TEXT("start centroid is town 0"),
		Shipped->SampleLocationId(Start.X, Start.Y, GameMin, GameMax),
		0);

	ASolidTerrainStreamer* Streamer = NewObject<ASolidTerrainStreamer>();
	TestNotNull(TEXT("streamer"), Streamer);
	if (Streamer)
	{
		const FVector2D Pawn = Start + Streamer->StartTownPawnOffsetXY;
		TestEqual(
			TEXT("captain spawn is still inside town 0"),
			Shipped->SampleLocationId(Pawn.X, Pawn.Y, GameMin, GameMax),
			0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsLaterTownsEnableRavenTest,
	"SolidCore1.Events.LaterTownsEnableRaven",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsLaterTownsEnableRavenTest::RunTest(const FString& Parameters)
{
	FGameState KanfoldState;
	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(KanfoldState, 2);
	TestTrue(TEXT("Kanfold fires"), Kanfold.bFired);
	TestTrue(TEXT("Kanfold switches sight"), Kanfold.bSetSight);
	TestEqual(
		TEXT("Kanfold enables raven sight"),
		static_cast<uint8>(Kanfold.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("Kanfold unlocks raven sight"), KanfoldState.bRavenSightEnabled);
	TestEqual(TEXT("Kanfold still fields Sam and Alex"), KanfoldState.MaxPartySize, 2);
	TestEqual(
		TEXT("Kanfold still titles Ranger"),
		FString(KanfoldState.CaptainTitle),
		FString(SolidEvents::TitleRanger));

	FGameState GateState;
	const SolidEvents::FResult Dragongate = SolidEvents::NotifyOccupiedTown(GateState, 5);
	TestTrue(TEXT("Dragongate fires"), Dragongate.bFired);
	TestTrue(TEXT("Dragongate switches sight"), Dragongate.bSetSight);
	TestEqual(
		TEXT("Dragongate enables raven sight"),
		static_cast<uint8>(Dragongate.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("Dragongate unlocks raven sight"), GateState.bRavenSightEnabled);
	TestEqual(TEXT("Dragongate still fields the company"), GateState.MaxPartySize, SolidEvents::RosterCount);
	TestEqual(
		TEXT("Dragongate still titles Captain"),
		FString(GateState.CaptainTitle),
		FString(SolidEvents::TitleCaptain));

	for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
	{
		const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
		if (FCString::Strcmp(Event.Name, TEXT("Enter Kanfold")) != 0
			&& FCString::Strcmp(Event.Name, TEXT("Enter Dragongate")) != 0)
		{
			continue;
		}
		TestTrue(
			*FString::Printf(TEXT("%s journal enables raven sight"), Event.Name),
			SolidJournal::Describe(Event).Contains(TEXT("Raven sight enabled")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsRequiresTest,
	"SolidCore1.Events.Requires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsRequiresTest::RunTest(const FString& Parameters)
{
	TArray<FString> Ids;
	for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
	{
		const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
		const FString Id = Event.Id ? FString(Event.Id) : FString();
		TestEqual(TEXT("id is eight digits"), Id.Len(), SolidEvents::EventIdLength);
		bool bHex = Id.Len() == SolidEvents::EventIdLength;
		for (const TCHAR Char : Id)
		{
			bHex = bHex && FChar::IsHexDigit(Char);
		}
		TestTrue(*FString::Printf(TEXT("%s id is hex"), Event.Name), bHex);
		TestFalse(*FString::Printf(TEXT("%s id is unique"), Event.Name), Ids.Contains(Id));
		Ids.Add(Id);
		TestEqual(TEXT("id finds the event"), SolidEvents::FindIndexById(Event.Id), Index);
	}

	const int32 IglinIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterIglin);
	const int32 BridgeIndex = SolidEvents::FindIndexById(SolidEvents::IdReachBrokenBridge);
	const int32 RelionIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterRelion);
	TestTrue(TEXT("Relion id is listed"), RelionIndex != INDEX_NONE);
	TestTrue(TEXT("broken bridge id is listed"), BridgeIndex != INDEX_NONE);
	TestTrue(
		TEXT("Reach broken bridge sits between Iglin and Relion"),
		IglinIndex != INDEX_NONE && BridgeIndex == IglinIndex + 1 && RelionIndex == BridgeIndex + 1);
	if (RelionIndex != INDEX_NONE)
	{
		const SolidEvents::FEvent& Relion = SolidEvents::Definitions[RelionIndex];
		TestEqual(TEXT("Relion requires one event"), Relion.RequireCount, 1);
		if (Relion.RequireCount > 0)
		{
			TestEqual(TEXT("Relion requires Enter Iglin"), FString(Relion.Requires[0]), FString(SolidEvents::IdEnterIglin));
		}
	}
	if (BridgeIndex != INDEX_NONE)
	{
		const SolidEvents::FEvent& BridgeEvent = SolidEvents::Definitions[BridgeIndex];
		TestEqual(TEXT("Reach broken bridge requires one event"), BridgeEvent.RequireCount, 1);
		if (BridgeEvent.RequireCount > 0)
		{
			TestEqual(
				TEXT("Reach broken bridge requires Enter Iglin"),
				FString(BridgeEvent.Requires[0]),
				FString(SolidEvents::IdEnterIglin));
		}
		const FString BridgeChanges = SolidJournal::Describe(BridgeEvent);
		TestEqual(
			TEXT("Reach broken bridge journal follows its changes"),
			BridgeEvent.ChangeCount > 0,
			!BridgeChanges.IsEmpty());
	}
	for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
	{
		const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
		if (FCString::Strcmp(Event.Id, SolidEvents::IdEnterRelion) == 0
			|| FCString::Strcmp(Event.Id, SolidEvents::IdReachBrokenBridge) == 0)
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s has no requirement"), Event.Name), Event.RequireCount, 0);
	}

	FGameState Early;
	const SolidEvents::FResult Blocked = SolidEvents::NotifyOccupiedTown(Early, 1);
	TestFalse(TEXT("Relion waits for Iglin"), Blocked.bFired);
	TestFalse(TEXT("an early visit does not repair the bridge"), Early.bIglinBridgeRepaired);
	TestEqual(TEXT("Relion is not recorded yet"), Early.FiredCount, 0);
	TestEqual(TEXT("the party stays empty"), Early.MaxPartySize, 0);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(Early, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	const SolidEvents::FResult RelionNow = SolidEvents::NotifyOccupiedTown(Early, 1);
	TestTrue(TEXT("Relion fires after Iglin"), RelionNow.bFired);
	TestTrue(TEXT("Relion repairs the bridge after Iglin"), RelionNow.bRepairBridge);
	TestTrue(TEXT("the bridge is repaired"), Early.bIglinBridgeRepaired);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsReachBrokenBridgeTest,
	"SolidCore1.Events.ReachBrokenBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsReachBrokenBridgeTest::RunTest(const FString& Parameters)
{
	const SolidEvents::FEvent* Event = SolidEventTestPrivate::FindEvent(TEXT("Reach broken bridge"));
	TestNotNull(TEXT("Reach broken bridge is listed"), Event);
	FString RampChanges;
	if (Event)
	{
		TestEqual(
			TEXT("the ramp trigger"),
			static_cast<uint8>(Event->Trigger),
			static_cast<uint8>(SolidEvents::ETrigger::ReachBrokenBridge));
		RampChanges = SolidJournal::Describe(*Event);
	}

	FGameState Early;
	const SolidEvents::FResult Blocked = SolidEvents::NotifyReachBrokenBridge(Early);
	TestFalse(TEXT("the ramp waits for Iglin"), Blocked.bFired);
	TestEqual(TEXT("an early stand is not recorded"), Early.FiredCount, 0);
	TestEqual(TEXT("an early stand is not journaled"), Early.Journal.Num(), 0);
	TestEqual(TEXT("the ramp does not occupy a town"), Early.OccupiedTown, INDEX_NONE);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(Early, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	const SolidEvents::FResult Reached = SolidEvents::NotifyReachBrokenBridge(Early);
	TestTrue(TEXT("the ramp fires after Iglin"), Reached.bFired);
	TestFalse(TEXT("the ramp does not switch sight"), Reached.bSetSight);
	TestFalse(TEXT("the ramp does not retitle"), Reached.bSetTitle);
	TestFalse(TEXT("the ramp does not repair the bridge"), Reached.bRepairBridge);
	TestFalse(TEXT("the Iglin bridge stays broken"), Early.bIglinBridgeRepaired);
	TestEqual(TEXT("the party stays empty"), Early.MaxPartySize, 0);
	TestEqual(TEXT("Iglin and the ramp are recorded"), Early.FiredCount, 2);

	const int32 BridgeIndex = SolidEvents::FindIndexById(SolidEvents::IdReachBrokenBridge);
	TestTrue(TEXT("the ramp is journaled"), Early.Journal.Num() == 2 && BridgeIndex != INDEX_NONE
		&& Early.Journal[1].EventIndex == BridgeIndex
		&& Early.Journal[1].Type == SolidJournal::EJournalType::Event);

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(Early, Rows);
	int32 RampRow = INDEX_NONE;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (Rows[Index].Kind == SolidJournal::ERowKind::Entry && Rows[Index].Text == TEXT("Reach broken bridge"))
		{
			RampRow = Index;
			break;
		}
	}
	TestTrue(TEXT("the journal lists Reach broken bridge"), RampRow != INDEX_NONE);
	const bool bDetail = RampRow != INDEX_NONE
		&& RampRow + 1 < Rows.Num()
		&& Rows[RampRow + 1].Kind == SolidJournal::ERowKind::Detail;
	TestEqual(TEXT("the ramp detail follows the event"), bDetail, !RampChanges.IsEmpty());
	if (bDetail && RampRow + 1 < Rows.Num())
	{
		TestEqual(TEXT("the ramp change line is the event"), Rows[RampRow + 1].Text, RampChanges);
	}

	const int32 JournalCount = Early.Journal.Num();
	const SolidEvents::FResult Again = SolidEvents::NotifyReachBrokenBridge(Early);
	TestFalse(TEXT("the ramp fires once"), Again.bFired);
	TestEqual(TEXT("a second stand is not recorded"), Early.FiredCount, 2);
	TestEqual(TEXT("a second stand is not journaled"), Early.Journal.Num(), JournalCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsActTest,
	"SolidCore1.Events.Act",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsActTest::RunTest(const FString& Parameters)
{
	auto ActOf = [](const TCHAR* Id) -> int32
	{
		const int32 Index = SolidEvents::FindIndexById(Id);
		return Index == INDEX_NONE ? INDEX_NONE : SolidEvents::Definitions[Index].Act;
	};

	TestEqual(TEXT("ACT_0"), SolidEvents::ACT_0, 0);
	TestEqual(TEXT("ACT_2"), SolidEvents::ACT_2, 2);
	TestEqual(TEXT("ACT_3"), SolidEvents::ACT_3, 3);
	TestEqual(TEXT("ACT_4"), SolidEvents::ACT_4, 4);
	TestEqual(TEXT("Iglin is ACT_0"), ActOf(SolidEvents::IdEnterIglin), SolidEvents::ACT_0);
	TestEqual(TEXT("Reach broken bridge is ACT_0"), ActOf(SolidEvents::IdReachBrokenBridge), SolidEvents::ACT_0);
	TestEqual(TEXT("Relion is ACT_2"), ActOf(SolidEvents::IdEnterRelion), SolidEvents::ACT_2);
	TestEqual(TEXT("Kanfold is ACT_3"), ActOf(SolidEvents::IdEnterKanfold), SolidEvents::ACT_3);
	TestEqual(TEXT("Dragongate is ACT_4"), ActOf(SolidEvents::IdEnterDragongate), SolidEvents::ACT_4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsCaveTest,
	"SolidCore1.Events.Cave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsCaveTest::RunTest(const FString& Parameters)
{
	FGameState State;
	TestFalse(TEXT("the cave starts closed"), State.bCaveEnabled);

	const SolidEvents::FResult Blocked = SolidEvents::NotifyReachBrokenBridge(State);
	TestFalse(TEXT("an early stand does not enter"), Blocked.bEnterCave);
	TestFalse(TEXT("an early stand leaves the cave closed"), State.bCaveEnabled);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(State, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	TestFalse(TEXT("Iglin leaves the cave closed"), Iglin.bEnterCave);
	TestFalse(TEXT("Iglin does not open the cave"), State.bCaveEnabled);

	const SolidEvents::FResult Reached = SolidEvents::NotifyReachBrokenBridge(State);
	TestTrue(TEXT("the ramp sends the captain inside"), Reached.bEnterCave);
	TestTrue(TEXT("the cave is open"), State.bCaveEnabled);

	const SolidEvents::FEvent* Event = SolidEventTestPrivate::FindEvent(TEXT("Reach broken bridge"));
	TestNotNull(TEXT("Reach broken bridge is listed"), Event);
	TestTrue(
		TEXT("the journal records the cave"),
		Event && SolidJournal::Describe(*Event).Contains(TEXT("Cave enabled")));

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	bool bSawCave = false;
	for (const SolidJournal::FRow& Row : Rows)
	{
		if (Row.Kind == SolidJournal::ERowKind::Detail && Row.Text.Contains(TEXT("Cave enabled")))
		{
			bSawCave = true;
		}
	}
	TestTrue(TEXT("the journal lists Cave enabled"), bSawCave);

	const SolidEvents::FResult Again = SolidEvents::NotifyReachBrokenBridge(State);
	TestFalse(TEXT("the cave trip happens once"), Again.bEnterCave);
	TestTrue(TEXT("the cave stays open"), State.bCaveEnabled);

	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);
	if (GameMode)
	{
		TestFalse(TEXT("a new game keeps the cave closed"), GameMode->IsCaveEnabled());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
