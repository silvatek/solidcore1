#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidEvents.h"
#include "Events/SolidMessages.h"
#include "SolidGameInstance.h"
#include "SolidGameState.h"
#include "Squad/SolidCaptain.h"
#include "SolidHUD.h"
#include "SolidGameMode.h"
#include "Journal/SolidJournal.h"
#include "Lairs/SolidLair.h"
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

	int32 CountSizeUnlocks(const SolidEvents::FEvent& Event, const ESolidStrongholdSize Size)
	{
		int32 Count = 0;
		const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			const SolidEvents::FChange& Change = Event.Changes[Index];
			if (Change.Kind == SolidEvents::EChange::EnableStrongholdSize && Change.StrongholdSize == Size)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountAnySizeUnlock(const SolidEvents::FEvent& Event)
	{
		int32 Count = 0;
		const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			if (Event.Changes[Index].Kind == SolidEvents::EChange::EnableStrongholdSize)
			{
				++Count;
			}
		}
		return Count;
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
	TestTrue(TEXT("at least the four enter-town events"), EnterTownCount >= 4);
	TestTrue(TEXT("events cover the enter-town list"), SolidEvents::DefinitionCount >= EnterTownCount);
	TestEqual(TEXT("roster is the company"), SolidEvents::RosterCount, 5);
	TestEqual(TEXT("slot 0 is Sam"), FString(SolidEvents::RosterName(0)), FString(TEXT("Sam")));
	TestEqual(TEXT("slot 1 is Alex"), FString(SolidEvents::RosterName(1)), FString(TEXT("Alex")));
	TestEqual(TEXT("slot 2 is Max"), FString(SolidEvents::RosterName(2)), FString(TEXT("Max")));
	TestEqual(TEXT("slot 3 is Morgan"), FString(SolidEvents::RosterName(3)), FString(TEXT("Morgan")));
	TestEqual(TEXT("slot 4 is Quinn"), FString(SolidEvents::RosterName(4)), FString(TEXT("Quinn")));
	TestNull(TEXT("slot 5 is empty"), SolidEvents::RosterName(5));

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	TestEqual(TEXT("squad starts empty"), State.MaxSquadSize, 0);
	TestEqual(TEXT("no companions yet"), SolidEvents::SquadCount(State), 0);
	TestTrue(TEXT("true sight is on"), SolidEvents::IsSightEnabled(StateCaptain, ESolidSight::True));
	TestFalse(TEXT("raven sight is locked"), SolidEvents::IsSightEnabled(StateCaptain, ESolidSight::Raven));
	TestFalse(
		TEXT("F9 cannot reach raven sight yet"),
		SolidEvents::IsSightEnabled(StateCaptain, SolidSight::Toggle(ESolidSight::True)));

	const SolidEvents::FResult Nowhere = SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	TestFalse(TEXT("open ground fires nothing"), Nowhere.bFired);
	TestEqual(TEXT("still an empty squad"), State.MaxSquadSize, 0);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("entering town 0 fires"), Iglin.bFired);
	TestFalse(TEXT("Iglin does not switch sight"), Iglin.bSetSight);
	TestFalse(TEXT("Iglin leaves the bridge broken"), Iglin.bRepairBridge);
	TestFalse(TEXT("the Iglin bridge starts broken"), State.bIglinBridgeRepaired);
	TestFalse(TEXT("Iglin leaves raven sight locked"), StateCaptain.bRavenSightEnabled);
	TestEqual(TEXT("Iglin leaves the squad empty"), State.MaxSquadSize, 0);
	TestEqual(TEXT("Iglin is recorded"), State.FiredOrder.Num(), 1);

	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TestTrue(TEXT("entering town 1 fires"), Relion.bFired);
	TestTrue(TEXT("town 1 switches sight"), Relion.bSetSight);
	TestEqual(
		TEXT("town 1 enables raven sight"),
		static_cast<uint8>(Relion.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("raven sight unlocked"), SolidEvents::IsSightEnabled(StateCaptain, ESolidSight::Raven));
	TestTrue(TEXT("true sight stays available"), SolidEvents::IsSightEnabled(StateCaptain, ESolidSight::True));
	TestTrue(TEXT("Relion repairs the bridge"), Relion.bRepairBridge);
	TestTrue(TEXT("the Iglin bridge is repaired"), State.bIglinBridgeRepaired);
	TestEqual(TEXT("town 1 squad size"), State.MaxSquadSize, 1);
	TestEqual(TEXT("town 1 companion count"), SolidEvents::SquadCount(State), 1);
	TestEqual(TEXT("Sam joins at Relion"), FString(SolidEvents::RosterName(0)), FString(TEXT("Sam")));

	const SolidEvents::FResult StillRelion = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TestFalse(TEXT("staying in town 1 does not fire again"), StillRelion.bFired);

	const SolidEvents::FResult Leave = SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	TestFalse(TEXT("leaving town fires nothing"), Leave.bFired);
	TestEqual(TEXT("squad size sticks after leaving"), State.MaxSquadSize, 1);

	const SolidEvents::FResult RelionAgain = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TestFalse(TEXT("town 1 event already fired"), RelionAgain.bFired);
	TestFalse(TEXT("re-entering Relion does not repair again"), RelionAgain.bRepairBridge);
	TestTrue(TEXT("the repaired bridge stays"), State.bIglinBridgeRepaired);
	TestEqual(TEXT("re-entering Relion does not shrink the squad"), State.MaxSquadSize, 1);

	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 2);
	TestTrue(TEXT("entering town 2 fires"), Kanfold.bFired);
	TestEqual(
		TEXT("town 2 sight follows the Kanfold event"),
		Kanfold.bSetSight,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Kanfold")));
	TestTrue(TEXT("raven sight still unlocked"), SolidEvents::IsSightEnabled(StateCaptain, ESolidSight::Raven));
	TestEqual(TEXT("town 2 squad size"), State.MaxSquadSize, 2);
	TestEqual(TEXT("Sam and Alex"), SolidEvents::SquadCount(State), 2);
	TestEqual(TEXT("Alex is the second companion"), FString(SolidEvents::RosterName(1)), FString(TEXT("Alex")));

	USolidGameState& Skipped = *NewObject<USolidGameState>();
	USolidCaptain& SkippedCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult OtherTown = SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 4);
	TestFalse(TEXT("Jethan has no event"), OtherTown.bFired);
	TestEqual(TEXT("Jethan leaves the squad empty"), Skipped.MaxSquadSize, 0);
	TestFalse(TEXT("Jethan does not unlock raven sight"), SkippedCaptain.bRavenSightEnabled);

	const SolidEvents::FResult KanfoldFirst = SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 2);
	TestTrue(TEXT("town 2 still fires if Relion was skipped"), KanfoldFirst.bFired);
	TestEqual(
		TEXT("Kanfold sight follows the event when Relion was skipped"),
		SkippedCaptain.bRavenSightEnabled,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Kanfold")));
	TestEqual(TEXT("town 2 still fields Sam and Alex"), SolidEvents::SquadCount(Skipped), 2);

	const SolidEvents::FResult Dragongate = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 5);
	TestTrue(TEXT("entering Dragongate fires"), Dragongate.bFired);
	TestEqual(
		TEXT("Dragongate sight follows the event"),
		Dragongate.bSetSight,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Dragongate")));
	TestTrue(TEXT("raven sight stays unlocked"), SolidEvents::IsSightEnabled(StateCaptain, ESolidSight::Raven));
	TestEqual(TEXT("Dragongate squad size"), State.MaxSquadSize, SolidEvents::RosterCount);
	TestEqual(TEXT("the whole company"), SolidEvents::SquadCount(State), 5);

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult DragongateAgain = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 5);
	TestFalse(TEXT("re-entering Dragongate does not fire again"), DragongateAgain.bFired);
	TestEqual(TEXT("re-entering Dragongate keeps the squad"), State.MaxSquadSize, 5);

	USolidGameState& GateFirst = *NewObject<USolidGameState>();
	USolidCaptain& GateFirstCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult GateOnly = SolidEvents::NotifyOccupiedTown(GateFirst, GateFirstCaptain, 5);
	TestTrue(TEXT("Dragongate fires on its own"), GateOnly.bFired);
	TestEqual(
		TEXT("Dragongate sight follows the event on its own"),
		GateFirstCaptain.bRavenSightEnabled,
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Dragongate")));
	TestEqual(TEXT("Dragongate fields the company"), SolidEvents::SquadCount(GateFirst), 5);
	SolidEvents::NotifyOccupiedTown(GateFirst, GateFirstCaptain, INDEX_NONE);
	const SolidEvents::FResult GateReturn = SolidEvents::NotifyOccupiedTown(GateFirst, GateFirstCaptain, 5);
	TestFalse(TEXT("Dragongate event already fired"), GateReturn.bFired);
	const SolidEvents::FResult KanfoldLater = SolidEvents::NotifyOccupiedTown(GateFirst, GateFirstCaptain, 2);
	TestTrue(TEXT("Kanfold still fires after Dragongate"), KanfoldLater.bFired);
	TestEqual(TEXT("Kanfold sets the squad to Sam and Alex"), GateFirst.MaxSquadSize, 2);
	const bool bEitherTownEnablesSight =
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Dragongate"))
		|| SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Kanfold"));
	TestEqual(
		TEXT("sight follows the Kanfold and Dragongate events"),
		GateFirstCaptain.bRavenSightEnabled,
		bEitherTownEnablesSight);

	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>(Instance);
	TestNotNull(TEXT("gamemode"), GameMode);
	if (GameMode)
	{
		TestEqual(TEXT("game starts with max squad 0"), GameMode->GetMaxSquadSize(), 0);
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
	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	TestEqual(TEXT("starts as Outcast"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Outcast)));

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	if (Iglin.bSetTitle)
	{
		TestEqual(
			TEXT("Iglin result matches the state"),
			FString(CaptainTitleName(Iglin.CaptainTitle)),
			FString(CaptainTitleName(StateCaptain.Title)));
	}
	else
	{
		TestEqual(
			TEXT("Iglin keeps the starting title"),
			FString(CaptainTitleName(StateCaptain.Title)),
			FString(CaptainTitleName(ESolidCaptainTitle::Outcast)));
	}

	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TestTrue(TEXT("Relion sets the title"), Relion.bSetTitle);
	TestEqual(TEXT("Relion result is Vagrant"), FString(CaptainTitleName(Relion.CaptainTitle)), FString(CaptainTitleName(ESolidCaptainTitle::Vagrant)));
	TestEqual(TEXT("Relion state is Vagrant"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Vagrant)));

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult RelionAgain = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TestFalse(TEXT("Relion does not fire twice"), RelionAgain.bFired);
	TestEqual(TEXT("title stays Vagrant"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Vagrant)));

	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 2);
	TestTrue(TEXT("Kanfold sets the title"), Kanfold.bSetTitle);
	TestEqual(TEXT("Kanfold is Ranger"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Ranger)));

	USolidGameState& Skipped = *NewObject<USolidGameState>();
	USolidCaptain& SkippedCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult KanfoldFirst = SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 2);
	TestTrue(TEXT("Kanfold titles Ranger without Relion"), KanfoldFirst.bSetTitle);
	TestFalse(TEXT("Kanfold leaves the bridge broken"), KanfoldFirst.bRepairBridge);
	TestFalse(TEXT("skipping Relion leaves the bridge broken"), Skipped.bIglinBridgeRepaired);
	TestEqual(TEXT("skipped path is Ranger"), FString(CaptainTitleName(SkippedCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Ranger)));
	SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 0);
	const SolidEvents::FResult RelionLater = SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 1);
	TestTrue(TEXT("Relion still fires later"), RelionLater.bSetTitle);
	TestTrue(TEXT("Relion later repairs the bridge"), RelionLater.bRepairBridge);
	TestTrue(TEXT("the skipped path repairs the bridge"), Skipped.bIglinBridgeRepaired);
	TestEqual(TEXT("Relion titles Vagrant"), FString(CaptainTitleName(SkippedCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Vagrant)));

	const SolidEvents::FResult Dragongate = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 5);
	TestTrue(TEXT("Dragongate sets the title"), Dragongate.bSetTitle);
	TestEqual(TEXT("Dragongate is Captain"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Captain)));
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult DragongateAgain = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 5);
	TestFalse(TEXT("Dragongate does not fire twice"), DragongateAgain.bFired);
	TestEqual(TEXT("title stays Captain"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Captain)));

	USolidGameState& GateFirst = *NewObject<USolidGameState>();
	USolidCaptain& GateFirstCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult GateOnly = SolidEvents::NotifyOccupiedTown(GateFirst, GateFirstCaptain, 5);
	TestTrue(TEXT("Dragongate titles Captain on its own"), GateOnly.bSetTitle);
	TestEqual(TEXT("first visit is Captain"), FString(CaptainTitleName(GateFirstCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Captain)));
	const SolidEvents::FResult KanfoldLater = SolidEvents::NotifyOccupiedTown(GateFirst, GateFirstCaptain, 2);
	TestTrue(TEXT("Kanfold still sets the title"), KanfoldLater.bSetTitle);
	TestEqual(TEXT("Kanfold after Dragongate is Ranger"), FString(CaptainTitleName(GateFirstCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Ranger)));

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
		Detail.Contains(TEXT("Squad 5: Sam, Alex, Max, Morgan, Quinn")));
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
				Line.AppendChar(USolidWorldMap::LocationMarker);
			}
			else if (Row == 4 && Col == 6)
			{
				Line.AppendChar(USolidWorldMap::LocationMarker);
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
	Text += TEXT("! 3,5 = Town Iglin\n");
	Text += TEXT("! 5,7 = Town Relion\n");
	Text += TEXT("0 = Starting village, biome=Town\n");
	Text += TEXT("Z = Starting town (brown)\n");

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

	TestEqual(TEXT("Iglin is town 0"), Map->SampleLocationId(Iglin.X, Iglin.Y, WorldMin, WorldMax), 0);
	TestEqual(TEXT("Relion is town 1"), Map->SampleLocationId(Relion.X, Relion.Y, WorldMin, WorldMax), 1);
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
	USolidGameState& KanfoldState = *NewObject<USolidGameState>();
	USolidCaptain& KanfoldStateCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(KanfoldState, KanfoldStateCaptain, 2);
	TestTrue(TEXT("Kanfold fires"), Kanfold.bFired);
	TestTrue(TEXT("Kanfold switches sight"), Kanfold.bSetSight);
	TestEqual(
		TEXT("Kanfold enables raven sight"),
		static_cast<uint8>(Kanfold.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("Kanfold unlocks raven sight"), KanfoldStateCaptain.bRavenSightEnabled);
	TestEqual(TEXT("Kanfold still fields Sam and Alex"), KanfoldState.MaxSquadSize, 2);
	TestEqual(
		TEXT("Kanfold still titles Ranger"),
		FString(CaptainTitleName(KanfoldStateCaptain.Title)),
		FString(CaptainTitleName(ESolidCaptainTitle::Ranger)));

	USolidGameState& GateState = *NewObject<USolidGameState>();
	USolidCaptain& GateStateCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult Dragongate = SolidEvents::NotifyOccupiedTown(GateState, GateStateCaptain, 5);
	TestTrue(TEXT("Dragongate fires"), Dragongate.bFired);
	TestTrue(TEXT("Dragongate switches sight"), Dragongate.bSetSight);
	TestEqual(
		TEXT("Dragongate enables raven sight"),
		static_cast<uint8>(Dragongate.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("Dragongate unlocks raven sight"), GateStateCaptain.bRavenSightEnabled);
	TestEqual(TEXT("Dragongate still fields the company"), GateState.MaxSquadSize, SolidEvents::RosterCount);
	TestEqual(
		TEXT("Dragongate still titles Captain"),
		FString(CaptainTitleName(GateStateCaptain.Title)),
		FString(CaptainTitleName(ESolidCaptainTitle::Captain)));

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
		const int32 IdLength = FCString::Strlen(SolidEvents::IdEnterIglin);
		TestEqual(TEXT("id is eight digits"), Id.Len(), IdLength);
		bool bHex = Id.Len() == IdLength;
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
		IglinIndex != INDEX_NONE && BridgeIndex > IglinIndex && RelionIndex > BridgeIndex);
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
		const bool bNamesOne = Event.RequireCount > 0 && Event.Requires[0] && Event.Requires[0][0];
		TestEqual(
			*FString::Printf(TEXT("%s names a requirement when it has one"), Event.Name),
			Event.RequireCount > 0,
			bNamesOne);
	}

	USolidGameState& Early = *NewObject<USolidGameState>();
	USolidCaptain& EarlyCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult Blocked = SolidEvents::NotifyOccupiedTown(Early, EarlyCaptain, 1);
	TestFalse(TEXT("Relion waits for Iglin"), Blocked.bFired);
	TestFalse(TEXT("an early visit does not repair the bridge"), Early.bIglinBridgeRepaired);
	TestEqual(TEXT("Relion is not recorded yet"), Early.FiredOrder.Num(), 0);
	TestEqual(TEXT("the squad stays empty"), Early.MaxSquadSize, 0);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(Early, EarlyCaptain, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	const SolidEvents::FResult RelionNow = SolidEvents::NotifyOccupiedTown(Early, EarlyCaptain, 1);
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

	USolidGameState& Early = *NewObject<USolidGameState>();
	USolidCaptain& EarlyCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult Blocked = SolidEvents::NotifyReachBrokenBridge(Early, EarlyCaptain);
	TestFalse(TEXT("the ramp waits for Iglin"), Blocked.bFired);
	TestEqual(TEXT("an early stand is not recorded"), Early.FiredOrder.Num(), 0);
	TestEqual(TEXT("an early stand is not journaled"), Early.Journal.Num(), 0);
	TestEqual(TEXT("the ramp does not occupy a town"), EarlyCaptain.OccupiedTown, INDEX_NONE);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(Early, EarlyCaptain, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	const SolidEvents::FResult Reached = SolidEvents::NotifyReachBrokenBridge(Early, EarlyCaptain);
	TestTrue(TEXT("the ramp fires after Iglin"), Reached.bFired);
	TestFalse(TEXT("the ramp does not switch sight"), Reached.bSetSight);
	TestFalse(TEXT("the ramp does not retitle"), Reached.bSetTitle);
	TestFalse(TEXT("the ramp does not repair the bridge"), Reached.bRepairBridge);
	TestFalse(TEXT("the Iglin bridge stays broken"), Early.bIglinBridgeRepaired);
	TestEqual(TEXT("the squad stays empty"), Early.MaxSquadSize, 0);
	TestEqual(TEXT("Iglin and the ramp are recorded"), Early.FiredOrder.Num(), 2);

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
	const SolidEvents::FResult Again = SolidEvents::NotifyReachBrokenBridge(Early, EarlyCaptain);
	TestFalse(TEXT("the ramp fires once"), Again.bFired);
	TestEqual(TEXT("a second stand is not recorded"), Early.FiredOrder.Num(), 2);
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
	TestEqual(TEXT("ACT_1"), SolidEvents::ACT_1, 1);
	TestEqual(TEXT("ACT_2"), SolidEvents::ACT_2, 2);
	TestEqual(TEXT("ACT_3"), SolidEvents::ACT_3, 3);
	TestEqual(TEXT("ACT_4"), SolidEvents::ACT_4, 4);
	TestEqual(TEXT("Iglin is ACT_0"), ActOf(SolidEvents::IdEnterIglin), SolidEvents::ACT_0);
	TestEqual(TEXT("Reach broken bridge is ACT_0"), ActOf(SolidEvents::IdReachBrokenBridge), SolidEvents::ACT_0);
	TestEqual(TEXT("Driven from the village is ACT_1"), ActOf(SolidEvents::IdDrivenFromVillage), SolidEvents::ACT_1);
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
	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	TestFalse(TEXT("the cave starts closed"), State.bStrongholdsEnabled);

	const SolidEvents::FResult Blocked = SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	TestFalse(TEXT("an early stand does not enter"), Blocked.bEnterStronghold);
	TestFalse(TEXT("an early stand leaves the cave closed"), State.bStrongholdsEnabled);

	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	TestFalse(TEXT("Iglin leaves the cave closed"), Iglin.bEnterStronghold);
	TestFalse(TEXT("Iglin does not open the cave"), State.bStrongholdsEnabled);

	const SolidEvents::FEvent* Event = SolidEventTestPrivate::FindEvent(TEXT("Reach broken bridge"));
	TestNotNull(TEXT("Reach broken bridge is listed"), Event);
	bool bReachOpensCave = false;
	if (Event)
	{
		const int32 ChangeCount = FMath::Min(Event->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			if (Event->Changes[Index].Kind == SolidEvents::EChange::EnableCave)
			{
				bReachOpensCave = true;
			}
		}
	}

	const SolidEvents::FResult Reached = SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	TestEqual(TEXT("the ramp follows its cave change"), Reached.bEnterStronghold, bReachOpensCave);
	TestEqual(TEXT("the cave follows the ramp event"), State.bStrongholdsEnabled, bReachOpensCave);
	TestEqual(
		TEXT("the journal records the cave when the ramp opens it"),
		Event && SolidJournal::Describe(*Event).Contains(TEXT("Cave enabled")),
		bReachOpensCave);

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
	TestEqual(TEXT("the journal lists the cave when the ramp opens it"), bSawCave, bReachOpensCave);

	const SolidEvents::FResult Again = SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	TestFalse(TEXT("the cave trip happens once"), Again.bEnterStronghold);
	TestEqual(TEXT("the cave stays with the ramp event"), State.bStrongholdsEnabled, bReachOpensCave);

	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>(Instance);
	TestNotNull(TEXT("gamemode"), GameMode);
	if (GameMode)
	{
		TestFalse(
			TEXT("a new game keeps strongholds closed"),
			GameMode->GetConfiguration()->bStrongholdsEnabled);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsMessageTest,
	"SolidCore1.Events.Message",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsMessageTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("at least one message"), SolidMessages::DefinitionCount >= 1);
	const TCHAR* KilledText = SolidMessages::TextFor(SolidMessages::IdKilledByMonsters);
	TestNotNull(TEXT("the monster message is listed"), KilledText);
	TestEqual(
		TEXT("the monster message text"),
		FString(KilledText ? KilledText : TEXT("")),
		FString(TEXT("You are killed by monsters")));
	TestNull(TEXT("an unknown message is empty"), SolidMessages::TextFor(TEXT("missing")));

	const FString MessageId = SolidMessages::IdKilledByMonsters;
	TestEqual(TEXT("message id is eight digits"), MessageId.Len(), SolidMessages::IdLength);
	bool bHex = MessageId.Len() == SolidMessages::IdLength;
	for (const TCHAR Char : MessageId)
	{
		bHex = bHex && FChar::IsHexDigit(Char);
	}
	TestTrue(TEXT("message id is hex"), bHex);

	TestTrue(TEXT("at least the six events"), SolidEvents::DefinitionCount >= 6);
	const int32 ReachIndex = SolidEvents::FindIndexById(SolidEvents::IdReachBrokenBridge);
	const int32 KilledIndex = SolidEvents::FindIndexById(SolidEvents::IdKilledByMonsters);
	const int32 RelionIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterRelion);
	TestTrue(TEXT("the close follows the ramp"), ReachIndex != INDEX_NONE && KilledIndex == ReachIndex + 1);
	TestTrue(TEXT("Relion follows the close"), KilledIndex != INDEX_NONE && RelionIndex > KilledIndex);

	const SolidEvents::FEvent* Reach = SolidEventTestPrivate::FindEvent(TEXT("Reach broken bridge"));
	TestNotNull(TEXT("Reach broken bridge is listed"), Reach);
	if (Reach)
	{
		TestEqual(TEXT("the ramp shows one message"), Reach->ChangeCount, 1);
		TestEqual(
			TEXT("the ramp change is ShowMessage"),
			static_cast<uint8>(Reach->Changes[0].Kind),
			static_cast<uint8>(SolidEvents::EChange::ShowMessage));
		TestEqual(
			TEXT("the ramp names the monster message"),
			FString(Reach->Changes[0].MessageId),
			FString(SolidMessages::IdKilledByMonsters));
		TestTrue(
			TEXT("the ramp journal is the message"),
			SolidJournal::Describe(*Reach).Contains(TEXT("You are killed by monsters")));
	}

	const SolidEvents::FEvent* Killed = SolidEventTestPrivate::FindEvent(TEXT("Killed by monsters"));
	TestNotNull(TEXT("Killed by monsters is listed"), Killed);
	if (Killed)
	{
		TestEqual(
			TEXT("closing the message is the trigger"),
			static_cast<uint8>(Killed->Trigger),
			static_cast<uint8>(SolidEvents::ETrigger::MessageClosed));
		TestEqual(
			TEXT("the close names the monster message"),
			FString(Killed->MessageId),
			FString(SolidMessages::IdKilledByMonsters));
		TestEqual(TEXT("the close is ACT_0"), Killed->Act, SolidEvents::ACT_0);
		bool bEnablesCave = false;
		const int32 ChangeCount = FMath::Min(Killed->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			if (Killed->Changes[Index].Kind == SolidEvents::EChange::EnableCave)
			{
				bEnablesCave = true;
			}
		}
		TestTrue(TEXT("the close enables the cave"), bEnablesCave);
		TestTrue(
			TEXT("the close journal records the cave"),
			SolidJournal::Describe(*Killed).Contains(TEXT("Cave enabled")));
	}

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	const SolidEvents::FResult Reached = SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	TestTrue(TEXT("the ramp shows the message"), Reached.bShowMessage);
	TestEqual(
		TEXT("the ramp result names the monster message"),
		FString(Reached.MessageId),
		FString(SolidMessages::IdKilledByMonsters));
	TestFalse(TEXT("the ramp leaves the cave closed"), Reached.bEnterStronghold);
	TestFalse(TEXT("showing the message does not open the cave"), State.bStrongholdsEnabled);

	const SolidEvents::FResult Other = SolidEvents::NotifyMessageClosed(State, StateCaptain, TEXT("missing"));
	TestFalse(TEXT("another message does not fire"), Other.bFired);
	TestFalse(TEXT("another message leaves the cave closed"), State.bStrongholdsEnabled);

	const SolidEvents::FResult Closed = SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestTrue(TEXT("closing the message fires"), Closed.bFired);
	TestTrue(TEXT("closing the message sends the captain inside"), Closed.bEnterStronghold);
	TestTrue(TEXT("the cave is open"), State.bStrongholdsEnabled);

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	bool bSawMessage = false;
	bool bSawCave = false;
	for (const SolidJournal::FRow& Row : Rows)
	{
		if (Row.Kind == SolidJournal::ERowKind::Detail && Row.Text.Contains(TEXT("You are killed by monsters")))
		{
			bSawMessage = true;
		}
		if (Row.Kind == SolidJournal::ERowKind::Detail && Row.Text.Contains(TEXT("Cave enabled")))
		{
			bSawCave = true;
		}
	}
	TestTrue(TEXT("the journal lists the message"), bSawMessage);
	TestTrue(TEXT("the journal lists Cave enabled"), bSawCave);

	const SolidEvents::FResult ClosedAgain = SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestFalse(TEXT("closing the message fires once"), ClosedAgain.bFired);
	TestFalse(TEXT("a second close does not travel again"), ClosedAgain.bEnterStronghold);
	TestTrue(TEXT("the cave stays open"), State.bStrongholdsEnabled);

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (HUD)
	{
		TestFalse(TEXT("no message yet"), HUD->IsMessageVisible());
		HUD->ShowMessage(SolidMessages::IdKilledByMonsters, SolidMessages::TextFor(SolidMessages::IdKilledByMonsters));
		TestTrue(TEXT("the message is showing"), HUD->IsMessageVisible());
		TestEqual(
			TEXT("the board shows the monster line"),
			HUD->GetMessageText(),
			FString(TEXT("You are killed by monsters")));
		TestEqual(TEXT("the board keeps the message id"), HUD->GetMessageId(), FString(SolidMessages::IdKilledByMonsters));
		HUD->CloseMessage();
		TestFalse(TEXT("the message closes"), HUD->IsMessageVisible());
		TestTrue(TEXT("a closed board clears the text"), HUD->GetMessageText().IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsDrivenFromVillageTest,
	"SolidCore1.Events.DrivenFromVillage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsDrivenFromVillageTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("two messages"), SolidMessages::DefinitionCount, 2);
	TestEqual(TEXT("seven events"), SolidEvents::DefinitionCount, 7);
	int32 EnterTownCount = 0;
	for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
	{
		if (SolidEvents::Definitions[Index].Trigger == SolidEvents::ETrigger::EnterTown)
		{
			++EnterTownCount;
		}
	}
	TestEqual(TEXT("five enter-town events"), EnterTownCount, 5);

	const TCHAR* DrivenText = SolidMessages::TextFor(SolidMessages::IdDrivenFromVillage);
	TestNotNull(TEXT("the village message is listed"), DrivenText);
	TestEqual(
		TEXT("the village message text"),
		FString(DrivenText ? DrivenText : TEXT("")),
		FString(TEXT("You are driven from the village")));
	TestTrue(
		TEXT("the two messages differ"),
		FCString::Strcmp(SolidMessages::IdKilledByMonsters, SolidMessages::IdDrivenFromVillage) != 0);
	const FString MessageId = SolidMessages::IdDrivenFromVillage;
	TestEqual(TEXT("village message id is eight digits"), MessageId.Len(), SolidMessages::IdLength);
	bool bHex = MessageId.Len() == SolidMessages::IdLength;
	for (const TCHAR Char : MessageId)
	{
		bHex = bHex && FChar::IsHexDigit(Char);
	}
	TestTrue(TEXT("village message id is hex"), bHex);

	const int32 KilledIndex = SolidEvents::FindIndexById(SolidEvents::IdKilledByMonsters);
	const int32 DrivenIndex = SolidEvents::FindIndexById(SolidEvents::IdDrivenFromVillage);
	const int32 RelionIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterRelion);
	TestTrue(TEXT("the return follows the close"), KilledIndex != INDEX_NONE && DrivenIndex == KilledIndex + 1);
	TestTrue(TEXT("Relion follows the return"), DrivenIndex != INDEX_NONE && RelionIndex == DrivenIndex + 1);

	const SolidEvents::FEvent* Driven = SolidEventTestPrivate::FindEvent(TEXT("Driven from the village"));
	TestNotNull(TEXT("Driven from the village is listed"), Driven);
	if (Driven)
	{
		TestEqual(
			TEXT("the return is an enter-town event"),
			static_cast<uint8>(Driven->Trigger),
			static_cast<uint8>(SolidEvents::ETrigger::EnterTown));
		TestEqual(TEXT("the return is Iglin"), Driven->Parameter, 0);
		TestEqual(TEXT("the return requires one event"), Driven->RequireCount, 1);
		if (Driven->RequireCount > 0)
		{
			TestEqual(
				TEXT("the return requires Killed by monsters"),
				FString(Driven->Requires[0]),
				FString(SolidEvents::IdKilledByMonsters));
		}
		TestTrue(TEXT("the return has an act"), Driven->Act >= SolidEvents::ACT_0);
		bool bShowsVillage = false;
		const int32 ChangeCount = FMath::Min(Driven->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			const SolidEvents::FChange& Change = Driven->Changes[Index];
			if (Change.Kind == SolidEvents::EChange::ShowMessage
				&& Change.MessageId
				&& FCString::Strcmp(Change.MessageId, SolidMessages::IdDrivenFromVillage) == 0)
			{
				bShowsVillage = true;
			}
		}
		TestTrue(TEXT("the return shows the village message"), bShowsVillage);
		TestTrue(
			TEXT("the return journal is the message"),
			SolidJournal::Describe(*Driven).Contains(TEXT("You are driven from the village")));
	}

	auto HasFiredDriven = [DrivenIndex](const USolidGameState& State)
	{
		return DrivenIndex != INDEX_NONE && DrivenIndex < SolidEvents::MaxEvents && State.bFired[DrivenIndex];
	};

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult First = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("the first visit records Iglin"), First.bFired);
	TestFalse(TEXT("the first visit shows no message"), First.bShowMessage);
	TestFalse(TEXT("the first visit is not the return"), HasFiredDriven(State));

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult EarlyReturn = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestFalse(TEXT("a return before the close shows no message"), EarlyReturn.bShowMessage);
	TestFalse(TEXT("a return before the close is not recorded"), HasFiredDriven(State));

	SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	const SolidEvents::FResult Closed = SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestTrue(TEXT("the close still opens the cave"), Closed.bEnterStronghold);
	TestFalse(TEXT("the close is not the return"), HasFiredDriven(State));

	const SolidEvents::FResult StillThere = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestFalse(TEXT("staying in Iglin does not show the message"), StillThere.bFired);
	TestFalse(TEXT("staying in Iglin is not the return"), HasFiredDriven(State));

	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TestTrue(TEXT("Relion still fires"), Relion.bFired);
	TestFalse(TEXT("Relion does not show the village message"), Relion.bShowMessage);
	TestFalse(TEXT("leaving for Relion is not the return"), HasFiredDriven(State));

	const SolidEvents::FResult Returned = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("entering Iglin after the close fires"), Returned.bFired);
	TestTrue(TEXT("entering Iglin after the close shows the message"), Returned.bShowMessage);
	TestEqual(
		TEXT("the return result names the village message"),
		FString(Returned.MessageId),
		FString(SolidMessages::IdDrivenFromVillage));
	TestFalse(TEXT("the return does not send the captain inside again"), Returned.bEnterStronghold);
	TestTrue(TEXT("the cave stays open"), State.bStrongholdsEnabled);
	TestTrue(TEXT("the return is recorded"), HasFiredDriven(State));

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	bool bSawDriven = false;
	for (const SolidJournal::FRow& Row : Rows)
	{
		if (Row.Kind == SolidJournal::ERowKind::Detail
			&& Row.Text.Contains(TEXT("You are driven from the village")))
		{
			bSawDriven = true;
		}
	}
	TestTrue(TEXT("the journal lists the village message"), bSawDriven);

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult ReturnedAgain = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestFalse(TEXT("the return fires once"), ReturnedAgain.bFired);
	TestFalse(TEXT("a second entry shows no message"), ReturnedAgain.bShowMessage);

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (HUD)
	{
		HUD->ShowMessage(SolidMessages::IdDrivenFromVillage, SolidMessages::TextFor(SolidMessages::IdDrivenFromVillage));
		TestEqual(
			TEXT("the board shows the village line"),
			HUD->GetMessageText(),
			FString(TEXT("You are driven from the village")));
		TestEqual(
			TEXT("the board keeps the village message id"),
			HUD->GetMessageId(),
			FString(SolidMessages::IdDrivenFromVillage));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsOrphanTitleTest,
	"SolidCore1.Events.OrphanTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsOrphanTitleTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Orphan"), FString(CaptainTitleName(ESolidCaptainTitle::Orphan)), FString(TEXT("Orphan")));

	const SolidEvents::FEvent* IglinEvent = SolidEventTestPrivate::FindEvent(TEXT("Enter Iglin"));
	TestNotNull(TEXT("Enter Iglin is listed"), IglinEvent);
	if (IglinEvent)
	{
		TestEqual(TEXT("Iglin has one change"), IglinEvent->ChangeCount, 1);
		TestEqual(
			TEXT("Iglin sets the title"),
			static_cast<uint8>(IglinEvent->Changes[0].Kind),
			static_cast<uint8>(SolidEvents::EChange::CaptainTitle));
		TestEqual(
			TEXT("Iglin names Orphan"),
			FString(CaptainTitleName(IglinEvent->Changes[0].Title)),
			FString(CaptainTitleName(ESolidCaptainTitle::Orphan)));
		TestTrue(
			TEXT("Iglin journal is Title Orphan"),
			SolidJournal::Describe(*IglinEvent).Contains(TEXT("Title Orphan")));
	}

	const SolidEvents::FEvent* Driven = SolidEventTestPrivate::FindEvent(TEXT("Driven from the village"));
	TestNotNull(TEXT("Driven from the village is listed"), Driven);
	if (Driven)
	{
		TestTrue(TEXT("the return keeps its changes"), Driven->ChangeCount >= 2);
		bool bTitlesOutcast = false;
		const int32 ChangeCount = FMath::Min(Driven->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			const SolidEvents::FChange& Change = Driven->Changes[Index];
			if (Change.Kind == SolidEvents::EChange::CaptainTitle
				&& Change.Title == ESolidCaptainTitle::Outcast)
			{
				bTitlesOutcast = true;
			}
		}
		TestTrue(TEXT("the return names Outcast"), bTitlesOutcast);
		TestTrue(
			TEXT("the return journal is Title Outcast"),
			SolidJournal::Describe(*Driven).Contains(TEXT("Title Outcast")));
	}

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	TestEqual(TEXT("starts as Outcast"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Outcast)));
	const SolidEvents::FResult Entered = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("Iglin sets the title"), Entered.bSetTitle);
	TestEqual(TEXT("Iglin result is Orphan"), FString(CaptainTitleName(Entered.CaptainTitle)), FString(CaptainTitleName(ESolidCaptainTitle::Orphan)));
	TestEqual(TEXT("Iglin state is Orphan"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Orphan)));

	SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestEqual(TEXT("the close leaves Orphan"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Orphan)));

	const SolidEvents::FResult StillThere = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestFalse(TEXT("staying in Iglin does not retitle"), StillThere.bFired);
	TestEqual(TEXT("staying leaves Orphan"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Orphan)));

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult Returned = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("the return sets the title"), Returned.bSetTitle);
	TestEqual(TEXT("the return result is Outcast"), FString(CaptainTitleName(Returned.CaptainTitle)), FString(CaptainTitleName(ESolidCaptainTitle::Outcast)));
	TestEqual(TEXT("the return state is Outcast"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Outcast)));
	TestTrue(TEXT("the return still shows the message"), Returned.bShowMessage);

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	bool bSawOrphan = false;
	bool bSawOutcast = false;
	for (const SolidJournal::FRow& Row : Rows)
	{
		if (Row.Kind != SolidJournal::ERowKind::Detail)
		{
			continue;
		}
		bSawOrphan = bSawOrphan || Row.Text.Contains(TEXT("Title Orphan"));
		bSawOutcast = bSawOutcast || Row.Text.Contains(TEXT("Title Outcast"));
	}
	TestTrue(TEXT("the journal lists Title Orphan"), bSawOrphan);
	TestTrue(TEXT("the journal lists Title Outcast"), bSawOutcast);

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult ReturnedAgain = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestFalse(TEXT("the return does not retitle twice"), ReturnedAgain.bFired);
	TestEqual(TEXT("the title stays Outcast"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Outcast)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsRavenSightWaitsForRelionTest,
	"SolidCore1.Events.RavenSightWaitsForRelion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsRavenSightWaitsForRelionTest::RunTest(const FString& Parameters)
{
	const int32 RelionIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterRelion);
	TestTrue(TEXT("Relion is listed"), RelionIndex != INDEX_NONE);
	for (int32 Index = 0; Index < RelionIndex; ++Index)
	{
		const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
		const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 ChangeIndex = 0; ChangeIndex < ChangeCount; ++ChangeIndex)
		{
			const SolidEvents::FChange& Change = Event.Changes[ChangeIndex];
			const bool bEnablesRaven = Change.Kind == SolidEvents::EChange::EnableSight
				&& Change.Sight == ESolidSight::Raven;
			TestFalse(
				*FString::Printf(TEXT("%s does not enable raven sight"), Event.Name),
				bEnablesRaven);
		}
	}
	TestTrue(
		TEXT("Relion enables raven sight"),
		SolidEventTestPrivate::EventEnablesRavenSight(TEXT("Enter Relion")));

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult Iglin = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("Iglin fires"), Iglin.bFired);
	TestFalse(TEXT("Iglin does not switch sight"), Iglin.bSetSight);
	TestFalse(TEXT("Iglin leaves raven sight locked"), StateCaptain.bRavenSightEnabled);

	const SolidEvents::FResult Ramp = SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	TestTrue(TEXT("the ramp fires"), Ramp.bFired);
	TestFalse(TEXT("the ramp does not switch sight"), Ramp.bSetSight);
	TestFalse(TEXT("the ramp leaves raven sight locked"), StateCaptain.bRavenSightEnabled);

	const SolidEvents::FResult Closed = SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestTrue(TEXT("the close fires"), Closed.bFired);
	TestFalse(TEXT("the close does not switch sight"), Closed.bSetSight);
	TestFalse(TEXT("the close leaves raven sight locked"), StateCaptain.bRavenSightEnabled);

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, INDEX_NONE);
	const SolidEvents::FResult Returned = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TestTrue(TEXT("the return fires"), Returned.bFired);
	TestFalse(TEXT("the return does not switch sight"), Returned.bSetSight);
	TestFalse(TEXT("the return leaves raven sight locked"), StateCaptain.bRavenSightEnabled);
	TestFalse(TEXT("F9 still cannot reach raven sight"), SolidEvents::IsSightEnabled(StateCaptain, ESolidSight::Raven));

	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TestTrue(TEXT("Relion switches sight"), Relion.bSetSight);
	TestEqual(
		TEXT("Relion enables raven sight"),
		static_cast<uint8>(Relion.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("Relion unlocks raven sight"), StateCaptain.bRavenSightEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsBuilderTest,
	"SolidCore1.Events.Builder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsBuilderTest::RunTest(const FString& Parameters)
{
	const SolidEvents::FEvent Town = SolidEvents::EnterTown(TEXT("aabbccdd"), TEXT("Sample town"), 4, SolidEvents::ACT_1)
		.EnableSight(ESolidSight::Raven)
		.SetMaxSquadSize(2)
		.SetPlayerTitle(ESolidCaptainTitle::Ranger)
		.Require(SolidEvents::IdEnterIglin);
	TestEqual(TEXT("sample id"), FString(Town.Id), FString(TEXT("aabbccdd")));
	TestEqual(TEXT("sample name"), FString(Town.Name), FString(TEXT("Sample town")));
	TestEqual(
		TEXT("sample trigger is enter town"),
		static_cast<uint8>(Town.Trigger),
		static_cast<uint8>(SolidEvents::ETrigger::EnterTown));
	TestEqual(TEXT("sample town id"), Town.Parameter, 4);
	TestEqual(TEXT("sample act"), Town.Act, SolidEvents::ACT_1);
	TestTrue(TEXT("a town event has no message id"), Town.MessageId == nullptr || !Town.MessageId[0]);
	TestEqual(TEXT("sample change count"), Town.ChangeCount, 3);
	TestEqual(
		TEXT("sample first change is sight"),
		static_cast<uint8>(Town.Changes[0].Kind),
		static_cast<uint8>(SolidEvents::EChange::EnableSight));
	TestEqual(
		TEXT("sample sight is raven"),
		static_cast<uint8>(Town.Changes[0].Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestEqual(TEXT("sample squad size"), Town.Changes[1].SquadSize, 2);
	TestEqual(
		TEXT("sample title"),
		FString(CaptainTitleName(Town.Changes[2].Title)),
		FString(CaptainTitleName(ESolidCaptainTitle::Ranger)));
	TestEqual(TEXT("sample requirement count"), Town.RequireCount, 1);
	TestEqual(TEXT("sample requirement"), FString(Town.Requires[0]), FString(SolidEvents::IdEnterIglin));

	const SolidEvents::FEvent Closed = SolidEvents::OnMessageClosed(
		TEXT("11223344"),
		TEXT("Sample close"),
		SolidMessages::IdKilledByMonsters,
		SolidEvents::ACT_0)
		.EnableCave();
	TestEqual(
		TEXT("sample close trigger"),
		static_cast<uint8>(Closed.Trigger),
		static_cast<uint8>(SolidEvents::ETrigger::MessageClosed));
	TestEqual(TEXT("sample close has no town"), Closed.Parameter, INDEX_NONE);
	TestEqual(TEXT("sample close message"), FString(Closed.MessageId), FString(SolidMessages::IdKilledByMonsters));
	TestEqual(TEXT("sample close has one change"), Closed.ChangeCount, 1);
	TestEqual(
		TEXT("sample close enables the cave"),
		static_cast<uint8>(Closed.Changes[0].Kind),
		static_cast<uint8>(SolidEvents::EChange::EnableCave));
	TestEqual(TEXT("sample close has no requirement"), Closed.RequireCount, 0);

	const SolidEvents::FEvent Ramp = SolidEvents::ReachBrokenBridge(TEXT("55667788"), TEXT("Sample ramp"), SolidEvents::ACT_0)
		.DisplayMessage(SolidMessages::IdDrivenFromVillage)
		.RepairBridge();
	TestEqual(
		TEXT("sample ramp trigger"),
		static_cast<uint8>(Ramp.Trigger),
		static_cast<uint8>(SolidEvents::ETrigger::ReachBrokenBridge));
	TestEqual(TEXT("sample ramp has no town"), Ramp.Parameter, INDEX_NONE);
	TestEqual(TEXT("sample ramp change count"), Ramp.ChangeCount, 2);
	TestEqual(
		TEXT("sample ramp message"),
		FString(Ramp.Changes[0].MessageId),
		FString(SolidMessages::IdDrivenFromVillage));
	TestEqual(
		TEXT("sample ramp repairs the bridge"),
		static_cast<uint8>(Ramp.Changes[1].Kind),
		static_cast<uint8>(SolidEvents::EChange::RepairBridge));
	TestEqual(TEXT("sample ramp has no requirement"), Ramp.RequireCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsClearInventoryTest,
	"SolidCore1.Events.ClearInventory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsClearInventoryTest::RunTest(const FString& Parameters)
{
	const SolidEvents::FEvent* Killed = SolidEventTestPrivate::FindEvent(TEXT("Killed by monsters"));
	TestNotNull(TEXT("Killed by monsters is listed"), Killed);
	bool bClearsInventory = false;
	if (Killed)
	{
		const int32 ChangeCount = FMath::Min(Killed->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			if (Killed->Changes[Index].Kind == SolidEvents::EChange::ClearInventory)
			{
				bClearsInventory = true;
			}
		}
		TestTrue(
			TEXT("the close journal records the clear"),
			SolidJournal::Describe(*Killed).Contains(TEXT("Inventory cleared")));
	}
	TestTrue(TEXT("the close clears the inventory"), bClearsInventory);

	const SolidEvents::FEvent Built = SolidEvents::OnMessageClosed(
		TEXT("99aabbcc"),
		TEXT("Sample clear"),
		SolidMessages::IdKilledByMonsters,
		SolidEvents::ACT_0)
		.ClearInventory();
	TestEqual(TEXT("a built clear has one change"), Built.ChangeCount, 1);
	TestEqual(
		TEXT("a built clear is ClearInventory"),
		static_cast<uint8>(Built.Changes[0].Kind),
		static_cast<uint8>(SolidEvents::EChange::ClearInventory));

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	State.Inventory = SolidInventory::StartingLoadout();
	SolidInventory::PlaceWorn(State.Inventory, SolidInventory::ESlot::Body, SolidItemType::IdTorch);
	SolidInventory::PlaceWorn(State.Inventory, SolidInventory::ESlot::Boots, SolidItemType::IdCrudeSpear);
	SolidInventory::PlacePack(State.Inventory, 0, 0, SolidItemType::IdDentedHelmet);
	SolidInventory::PlacePack(State.Inventory, 1, 2, SolidItemType::IdTorch);

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	const SolidEvents::FResult Reached = SolidEvents::NotifyReachBrokenBridge(State, StateCaptain);
	TestTrue(TEXT("the ramp still shows the message"), Reached.bShowMessage);
	TestEqual(
		TEXT("the ramp leaves the helmet"),
		FString(State.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Head)]),
		FString(SolidItemType::IdDentedHelmet));
	TestEqual(
		TEXT("the ramp leaves the pack"),
		FString(State.Inventory.Pack[SolidInventory::PackIndex(0, 0)]),
		FString(SolidItemType::IdDentedHelmet));

	const SolidEvents::FResult Closed = SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestTrue(TEXT("closing the message fires"), Closed.bFired);
	TestTrue(TEXT("closing the message still opens the cave"), Closed.bEnterStronghold);
	for (int32 Index = 0; Index < SolidInventory::WornCount; ++Index)
	{
		const SolidInventory::ESlot Slot = static_cast<SolidInventory::ESlot>(Index);
		if (Slot == SolidInventory::ESlot::Body)
		{
			TestEqual(
				TEXT("the body wears rags"),
				FString(State.Inventory.Worn[Index]),
				FString(SolidItemType::IdRags));
		}
		else
		{
			TestTrue(TEXT("the close empties the other worn boxes"), State.Inventory.Worn[Index] == nullptr);
		}
	}
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		TestTrue(TEXT("the close empties the pack"), State.Inventory.Pack[Index] == nullptr);
	}

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	bool bSawClear = false;
	for (const SolidJournal::FRow& Row : Rows)
	{
		if (Row.Kind == SolidJournal::ERowKind::Detail && Row.Text.Contains(TEXT("Inventory cleared")))
		{
			bSawClear = true;
		}
	}
	TestTrue(TEXT("the journal lists Inventory cleared"), bSawClear);

	SolidInventory::PlaceWorn(State.Inventory, SolidInventory::ESlot::Head, SolidItemType::IdDentedHelmet);
	const SolidEvents::FResult ClosedAgain = SolidEvents::NotifyMessageClosed(State, StateCaptain, SolidMessages::IdKilledByMonsters);
	TestFalse(TEXT("closing the message fires once"), ClosedAgain.bFired);
	TestEqual(
		TEXT("a second close leaves the helmet"),
		FString(State.Inventory.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Head)]),
		FString(SolidItemType::IdDentedHelmet));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsEnableStrongholdSizeTest,
	"SolidCore1.Events.EnableStrongholdSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsEnableStrongholdSizeTest::RunTest(const FString& Parameters)
{
	const SolidEvents::FEvent* Iglin = SolidEventTestPrivate::FindEvent(TEXT("Enter Iglin"));
	TestNotNull(TEXT("Enter Iglin is listed"), Iglin);
	if (Iglin)
	{
		const int32 ChangeCount = FMath::Min(Iglin->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			TestFalse(
				TEXT("Enter Iglin does not unlock a size"),
				Iglin->Changes[Index].Kind == SolidEvents::EChange::EnableStrongholdSize);
		}
	}
	USolidGameState& Played = *NewObject<USolidGameState>();
	USolidCaptain& PlayedCaptain = *NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(Played, PlayedCaptain, SolidTowns::Iglin);
	TestEqual(
		TEXT("Enter Iglin leaves the ceiling at Meagre"),
		static_cast<uint8>(Played.MaxStrongholdSize),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		if (Size == ESolidStrongholdSize::Meagre)
		{
			continue;
		}
		TestFalse(
			*FString::Printf(TEXT("Enter Iglin leaves %s locked"), SolidStronghold::SizeName(Size)),
			SolidEvents::IsStrongholdSizeUnlocked(Played, Size));
	}

	const SolidEvents::FEvent Built = SolidEvents::EnterTown(TEXT("aabbcc01"), TEXT("Sample unlock"), 0, SolidEvents::ACT_0)
		.EnableStrongholdSize(ESolidStrongholdSize::Humble);
	TestEqual(TEXT("a built enable has one change"), Built.ChangeCount, 1);
	TestEqual(
		TEXT("a built enable is EnableStrongholdSize"),
		static_cast<uint8>(Built.Changes[0].Kind),
		static_cast<uint8>(SolidEvents::EChange::EnableStrongholdSize));
	TestEqual(
		TEXT("a built unlock names Humble"),
		static_cast<uint8>(Built.Changes[0].StrongholdSize),
		static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestTrue(
		TEXT("the journal names the size"),
		SolidJournal::Describe(Built).Contains(TEXT("Humble size unlocked")));

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	SolidEvents::FResult Result;
	SolidEvents::ApplyChange(State, StateCaptain, Built.Changes[0], Result);
	TestTrue(TEXT("Humble is unlocked"), SolidEvents::IsStrongholdSizeUnlocked(State, ESolidStrongholdSize::Humble));
	TestTrue(TEXT("Meagre is unlocked with Humble"), SolidEvents::IsStrongholdSizeUnlocked(State, ESolidStrongholdSize::Meagre));
	TestFalse(TEXT("Average stays locked"), SolidEvents::IsStrongholdSizeUnlocked(State, ESolidStrongholdSize::Average));
	SolidEvents::ApplyChange(State, StateCaptain, Built.Changes[0], Result);
	TestTrue(TEXT("unlocking Humble again keeps it"), SolidEvents::IsStrongholdSizeUnlocked(State, ESolidStrongholdSize::Humble));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsEnableLairTest,
	"SolidCore1.Events.EnableLair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsEnableLairTest::RunTest(const FString& Parameters)
{
	const SolidEvents::FEvent* Driven = SolidEventTestPrivate::FindEvent(TEXT("Driven from the village"));
	TestNotNull(TEXT("Driven from the village is listed"), Driven);
	const SolidEvents::FChange* Enable = nullptr;
	if (Driven)
	{
		const int32 ChangeCount = FMath::Min(Driven->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			if (Driven->Changes[Index].Kind == SolidEvents::EChange::EnableLair)
			{
				Enable = &Driven->Changes[Index];
			}
		}
	}
	TestNotNull(TEXT("the return carries EnableLair"), Enable);
	if (Enable)
	{
		TestEqual(
			TEXT("the return carries the Iglin tunnels id"),
			FString(Enable->LairId ? Enable->LairId : TEXT("")),
			FString(SolidLair::IdIglinTunnels));
		TestTrue(
			TEXT("the change does not store the display name"),
			FCString::Strcmp(Enable->LairId, SolidLair::IglinTunnels.Name) != 0);
	}

	const TCHAR* MessageText = SolidMessages::TextFor(SolidMessages::IdDrivenFromVillage);
	const FString Detail = Driven ? SolidJournal::Describe(*Driven) : FString();
	TestTrue(TEXT("the journal names the lair"), Detail.Contains(FString::Printf(TEXT("%s enabled"), SolidLair::IglinTunnels.Name)));
	TestTrue(TEXT("the journal keeps the message"), MessageText && Detail.Contains(MessageText));
	TestTrue(TEXT("the journal keeps the title"), Detail.Contains(TEXT("Title Outcast")));

	auto LacksEnableLair = [](const TCHAR* EventName)
	{
		const SolidEvents::FEvent* Event = SolidEventTestPrivate::FindEvent(EventName);
		if (!Event)
		{
			return false;
		}
		const int32 ChangeCount = FMath::Min(Event->ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 Index = 0; Index < ChangeCount; ++Index)
		{
			if (Event->Changes[Index].Kind == SolidEvents::EChange::EnableLair)
			{
				return false;
			}
		}
		return true;
	};
	TestTrue(TEXT("Enter Iglin does not enable a lair"), LacksEnableLair(TEXT("Enter Iglin")));
	TestTrue(TEXT("Killed by monsters does not enable a lair"), LacksEnableLair(TEXT("Killed by monsters")));

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	TestFalse(TEXT("the tunnels start shut"), SolidLair::TunnelsOpen(State));
	SolidEvents::FResult Result;
	if (Enable)
	{
		SolidEvents::ApplyChange(State, StateCaptain, *Enable, Result);
	}
	TestTrue(TEXT("applying it enables the Iglin tunnels"), SolidEvents::IsLairEnabled(State, SolidLair::IdIglinTunnels));
	TestTrue(TEXT("the tunnels are open"), SolidLair::TunnelsOpen(State));
	TestFalse(TEXT("another lair stays shut"), SolidEvents::IsLairEnabled(State, TEXT("aabbcc99")));
	TestFalse(TEXT("the cave stays shut"), State.bStrongholdsEnabled);
	TestFalse(TEXT("EnableLair does not enter the cave"), Result.bEnterStronghold);

	const SolidEvents::FEvent Other = SolidEvents::EnterTown(TEXT("aabbcc02"), TEXT("Sample lair"), 0, SolidEvents::ACT_0)
		.EnableLair(TEXT("aabbcc99"));
	TestEqual(TEXT("a built enable has one change"), Other.ChangeCount, 1);
	TestEqual(
		TEXT("a built enable is EnableLair"),
		static_cast<uint8>(Other.Changes[0].Kind),
		static_cast<uint8>(SolidEvents::EChange::EnableLair));
	USolidGameState& OtherState = *NewObject<USolidGameState>();
	USolidCaptain& OtherStateCaptain = *NewObject<USolidCaptain>();
	SolidEvents::FResult OtherResult;
	SolidEvents::ApplyChange(OtherState, OtherStateCaptain, Other.Changes[0], OtherResult);
	TestTrue(TEXT("the id opens"), SolidEvents::IsLairEnabled(OtherState, TEXT("aabbcc99")));
	TestFalse(TEXT("that change leaves the Iglin tunnels shut"), SolidLair::TunnelsOpen(OtherState));
	TestFalse(TEXT("a different lair does not enter the cave"), OtherResult.bEnterStronghold);

	if (Enable)
	{
		SolidEvents::ApplyChange(State, StateCaptain, *Enable, Result);
	}
	TestTrue(TEXT("enabling the tunnels again keeps them open"), SolidLair::TunnelsOpen(State));
	TestEqual(TEXT("the tunnels are recorded once"), State.EnabledLairs.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsDragongateUnlocksSizesTest,
	"SolidCore1.Events.DragongateUnlocksSizes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsDragongateUnlocksSizesTest::RunTest(const FString& Parameters)
{
	const int32 EventIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterDragongate);
	TestTrue(TEXT("Enter Dragongate is listed"), EventIndex != INDEX_NONE);
	if (EventIndex == INDEX_NONE)
	{
		return false;
	}

	const SolidEvents::FEvent& Event = SolidEvents::Definitions[EventIndex];
	bool Seen[SolidStronghold::SizeCount] = {};
	int32 Unlocks = 0;
	const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
	for (int32 Index = 0; Index < ChangeCount; ++Index)
	{
		const SolidEvents::FChange& Change = Event.Changes[Index];
		if (Change.Kind != SolidEvents::EChange::EnableStrongholdSize)
		{
			continue;
		}
		++Unlocks;
		const int32 SizeIndex = static_cast<int32>(Change.StrongholdSize);
		if (SizeIndex >= 0 && SizeIndex < SolidStronghold::SizeCount)
		{
			Seen[SizeIndex] = true;
		}
	}
	TestEqual(TEXT("Enter Dragongate unlocks every size"), Unlocks, SolidStronghold::SizeCount);
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		TestTrue(
			*FString::Printf(TEXT("Enter Dragongate unlocks %s"), SolidStronghold::SizeName(Size)),
			Seen[static_cast<int32>(Size)]);
	}

	const FString Detail = SolidJournal::Describe(Event);
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		TestTrue(
			*FString::Printf(TEXT("the journal names %s"), SolidStronghold::SizeName(Size)),
			Detail.Contains(FString::Printf(TEXT("%s size unlocked"), SolidStronghold::SizeName(Size))));
	}

	USolidGameState& Before = *NewObject<USolidGameState>();
	USolidCaptain& BeforeCaptain = *NewObject<USolidCaptain>();
	TestEqual(
		TEXT("a new game stops at Meagre"),
		static_cast<uint8>(Before.MaxStrongholdSize),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		if (Size == ESolidStrongholdSize::Meagre)
		{
			continue;
		}
		TestFalse(
			*FString::Printf(TEXT("%s starts locked"), SolidStronghold::SizeName(Size)),
			SolidEvents::IsStrongholdSizeUnlocked(Before, Size));
	}

	USolidGameState& Played = *NewObject<USolidGameState>();
	USolidCaptain& PlayedCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult Fired = SolidEvents::NotifyOccupiedTown(Played, PlayedCaptain, SolidTowns::Dragongate);
	TestTrue(TEXT("Enter Dragongate fires"), Fired.bFired);
	TestTrue(TEXT("Enter Dragongate opens strongholds"), Played.bStrongholdsEnabled);
	TestFalse(TEXT("Enter Dragongate does not enter"), Fired.bEnterStronghold);
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		TestTrue(
			*FString::Printf(TEXT("Enter Dragongate unlocks %s on the company"), SolidStronghold::SizeName(Size)),
			SolidEvents::IsStrongholdSizeUnlocked(Played, Size));
	}

	SolidEvents::NotifyOccupiedTown(Played, PlayedCaptain, INDEX_NONE);
	const SolidEvents::FResult Again = SolidEvents::NotifyOccupiedTown(Played, PlayedCaptain, SolidTowns::Dragongate);
	TestFalse(TEXT("Enter Dragongate fires once"), Again.bFired);
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		TestTrue(
			*FString::Printf(TEXT("%s stays unlocked"), SolidStronghold::SizeName(Size)),
			SolidEvents::IsStrongholdSizeUnlocked(Played, Size));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsEarlySizeUnlocksTest,
	"SolidCore1.Events.EarlySizeUnlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsEarlySizeUnlocksTest::RunTest(const FString& Parameters)
{
	const TCHAR* Homes[] = {
		TEXT("Killed by monsters"),
		TEXT("Enter Relion"),
		TEXT("Enter Kanfold"),
	};
	const ESolidStrongholdSize HomeSize[] = {
		ESolidStrongholdSize::Meagre,
		ESolidStrongholdSize::Humble,
		ESolidStrongholdSize::Average,
	};
	for (int32 Home = 0; Home < UE_ARRAY_COUNT(Homes); ++Home)
	{
		const SolidEvents::FEvent* Event = SolidEventTestPrivate::FindEvent(Homes[Home]);
		TestNotNull(*FString::Printf(TEXT("%s is listed"), Homes[Home]), Event);
		if (!Event)
		{
			continue;
		}
		TestEqual(
			*FString::Printf(TEXT("%s unlocks one size"), Homes[Home]),
			SolidEventTestPrivate::CountAnySizeUnlock(*Event),
			1);
		TestEqual(
			*FString::Printf(TEXT("%s unlocks %s"), Homes[Home], SolidStronghold::SizeName(HomeSize[Home])),
			SolidEventTestPrivate::CountSizeUnlocks(*Event, HomeSize[Home]),
			1);
		TestTrue(
			*FString::Printf(TEXT("%s journal names %s"), Homes[Home], SolidStronghold::SizeName(HomeSize[Home])),
			SolidJournal::Describe(*Event).Contains(
				FString::Printf(TEXT("%s size unlocked"), SolidStronghold::SizeName(HomeSize[Home]))));
	}

	const SolidEvents::FEvent* Dragongate = SolidEventTestPrivate::FindEvent(TEXT("Enter Dragongate"));
	TestNotNull(TEXT("Enter Dragongate is listed"), Dragongate);
	if (Dragongate)
	{
		TestEqual(
			TEXT("Enter Dragongate still lists Meagre"),
			SolidEventTestPrivate::CountSizeUnlocks(*Dragongate, ESolidStrongholdSize::Meagre),
			1);
		TestEqual(
			TEXT("Enter Dragongate still lists Humble"),
			SolidEventTestPrivate::CountSizeUnlocks(*Dragongate, ESolidStrongholdSize::Humble),
			1);
		TestEqual(
			TEXT("Enter Dragongate still lists Average"),
			SolidEventTestPrivate::CountSizeUnlocks(*Dragongate, ESolidStrongholdSize::Average),
			1);
	}

	USolidGameState& Closed = *NewObject<USolidGameState>();
	USolidCaptain& ClosedCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult Monster = SolidEvents::NotifyMessageClosed(Closed, ClosedCaptain, SolidMessages::IdKilledByMonsters);
	TestTrue(TEXT("Killed by monsters fires"), Monster.bFired);
	TestTrue(TEXT("the close unlocks Meagre"), SolidEvents::IsStrongholdSizeUnlocked(Closed, ESolidStrongholdSize::Meagre));
	TestFalse(TEXT("the close leaves Humble locked"), SolidEvents::IsStrongholdSizeUnlocked(Closed, ESolidStrongholdSize::Humble));
	TestFalse(TEXT("the close leaves Average locked"), SolidEvents::IsStrongholdSizeUnlocked(Closed, ESolidStrongholdSize::Average));

	USolidGameState& Relion = *NewObject<USolidGameState>();
	USolidCaptain& RelionCaptain = *NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(Relion, RelionCaptain, SolidTowns::Iglin);
	const SolidEvents::FResult EnteredRelion = SolidEvents::NotifyOccupiedTown(Relion, RelionCaptain, SolidTowns::Relion);
	TestTrue(TEXT("Enter Relion fires"), EnteredRelion.bFired);
	TestTrue(TEXT("Enter Relion opens strongholds"), Relion.bStrongholdsEnabled);
	TestFalse(TEXT("Enter Relion does not enter"), EnteredRelion.bEnterStronghold);
	TestTrue(TEXT("Enter Relion unlocks Meagre with Humble"), SolidEvents::IsStrongholdSizeUnlocked(Relion, ESolidStrongholdSize::Meagre));
	TestTrue(TEXT("Enter Relion unlocks Humble"), SolidEvents::IsStrongholdSizeUnlocked(Relion, ESolidStrongholdSize::Humble));
	TestFalse(TEXT("Enter Relion leaves Average locked"), SolidEvents::IsStrongholdSizeUnlocked(Relion, ESolidStrongholdSize::Average));

	USolidGameState& Kanfold = *NewObject<USolidGameState>();
	USolidCaptain& KanfoldCaptain = *NewObject<USolidCaptain>();
	const SolidEvents::FResult EnteredKanfold = SolidEvents::NotifyOccupiedTown(Kanfold, KanfoldCaptain, SolidTowns::Kanfold);
	TestTrue(TEXT("Enter Kanfold fires"), EnteredKanfold.bFired);
	TestTrue(TEXT("Enter Kanfold opens strongholds"), Kanfold.bStrongholdsEnabled);
	TestFalse(TEXT("Enter Kanfold does not enter"), EnteredKanfold.bEnterStronghold);
	TestTrue(TEXT("Enter Kanfold unlocks Meagre with Average"), SolidEvents::IsStrongholdSizeUnlocked(Kanfold, ESolidStrongholdSize::Meagre));
	TestTrue(TEXT("Enter Kanfold unlocks Humble with Average"), SolidEvents::IsStrongholdSizeUnlocked(Kanfold, ESolidStrongholdSize::Humble));
	TestTrue(TEXT("Enter Kanfold unlocks Average"), SolidEvents::IsStrongholdSizeUnlocked(Kanfold, ESolidStrongholdSize::Average));
	TestFalse(
		TEXT("Enter Kanfold leaves Impressive locked"),
		SolidEvents::IsStrongholdSizeUnlocked(Kanfold, ESolidStrongholdSize::Impressive));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
