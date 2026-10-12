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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidEventsStartAndTownsTest,
	"SolidCore1.Events.EnterTown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidEventsStartAndTownsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("four enter-town events"), SolidEvents::DefinitionCount, 4);
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
	TestEqual(TEXT("re-entering Relion does not shrink the party"), State.MaxPartySize, 1);

	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(State, 2);
	TestTrue(TEXT("entering town 2 fires"), Kanfold.bFired);
	TestFalse(TEXT("town 2 does not change sight"), Kanfold.bSetSight);
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
	TestFalse(TEXT("skipped Relion does not enable raven sight"), Skipped.bRavenSightEnabled);
	TestEqual(TEXT("town 2 still fields Sam and Alex"), SolidEvents::PartyCount(Skipped), 2);

	const SolidEvents::FResult Dragongate = SolidEvents::NotifyOccupiedTown(State, 5);
	TestTrue(TEXT("entering Dragongate fires"), Dragongate.bFired);
	TestFalse(TEXT("Dragongate does not change sight"), Dragongate.bSetSight);
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
	TestFalse(TEXT("Dragongate leaves raven sight locked"), GateFirst.bRavenSightEnabled);
	TestEqual(TEXT("Dragongate fields the company"), SolidEvents::PartyCount(GateFirst), 5);
	SolidEvents::NotifyOccupiedTown(GateFirst, INDEX_NONE);
	const SolidEvents::FResult GateReturn = SolidEvents::NotifyOccupiedTown(GateFirst, 5);
	TestFalse(TEXT("Dragongate event already fired"), GateReturn.bFired);
	const SolidEvents::FResult KanfoldLater = SolidEvents::NotifyOccupiedTown(GateFirst, 2);
	TestTrue(TEXT("Kanfold still fires after Dragongate"), KanfoldLater.bFired);
	TestEqual(TEXT("Kanfold sets the party to Sam and Alex"), GateFirst.MaxPartySize, 2);
	TestFalse(TEXT("Kanfold still does not unlock raven sight"), GateFirst.bRavenSightEnabled);

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
	TestEqual(TEXT("skipped path is Ranger"), FString(Skipped.CaptainTitle), FString(SolidEvents::TitleRanger));
	const SolidEvents::FResult RelionLater = SolidEvents::NotifyOccupiedTown(Skipped, 1);
	TestTrue(TEXT("Relion still fires later"), RelionLater.bSetTitle);
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

	const FString Detail = SolidJournal::Describe(SolidEvents::Definitions[3]);
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
				Line.AppendChar(TEXT('G'));
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

#endif // WITH_DEV_AUTOMATION_TESTS
