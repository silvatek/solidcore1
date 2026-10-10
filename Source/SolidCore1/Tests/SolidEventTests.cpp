#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidEvents.h"
#include "SolidGameMode.h"
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
	TestEqual(TEXT("two starter events"), SolidEvents::DefinitionCount, 2);
	TestEqual(TEXT("roster is Sam then Alex"), SolidEvents::RosterCount, 2);
	TestEqual(TEXT("slot 0 is Sam"), FString(SolidEvents::RosterName(0)), FString(TEXT("Sam")));
	TestEqual(TEXT("slot 1 is Alex"), FString(SolidEvents::RosterName(1)), FString(TEXT("Alex")));

	SolidEvents::FState State;
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
	TestTrue(TEXT("town 0 switches sight"), Iglin.bSetSight);
	TestEqual(
		TEXT("town 0 enables raven sight"),
		static_cast<uint8>(Iglin.Sight),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("raven sight unlocked"), SolidEvents::IsSightEnabled(State, ESolidSight::Raven));
	TestTrue(TEXT("true sight stays available"), SolidEvents::IsSightEnabled(State, ESolidSight::True));
	TestEqual(TEXT("town 0 party size"), State.MaxPartySize, 1);
	TestEqual(TEXT("town 0 companion count"), SolidEvents::PartyCount(State), 1);
	TestEqual(TEXT("Sam joins at Iglin"), FString(SolidEvents::RosterName(0)), FString(TEXT("Sam")));

	const SolidEvents::FResult StillIglin = SolidEvents::NotifyOccupiedTown(State, 0);
	TestFalse(TEXT("staying in town 0 does not fire again"), StillIglin.bFired);

	const SolidEvents::FResult Leave = SolidEvents::NotifyOccupiedTown(State, INDEX_NONE);
	TestFalse(TEXT("leaving town fires nothing"), Leave.bFired);
	TestEqual(TEXT("party size sticks after leaving"), State.MaxPartySize, 1);

	const SolidEvents::FResult IglinAgain = SolidEvents::NotifyOccupiedTown(State, 0);
	TestFalse(TEXT("town 0 event already fired"), IglinAgain.bFired);
	TestEqual(TEXT("re-entering Iglin does not shrink the party"), State.MaxPartySize, 1);

	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(State, 1);
	TestTrue(TEXT("entering town 1 fires"), Relion.bFired);
	TestFalse(TEXT("town 1 does not change sight"), Relion.bSetSight);
	TestTrue(TEXT("raven sight still unlocked"), SolidEvents::IsSightEnabled(State, ESolidSight::Raven));
	TestEqual(TEXT("town 1 party size"), State.MaxPartySize, 2);
	TestEqual(TEXT("Sam and Alex"), SolidEvents::PartyCount(State), 2);
	TestEqual(TEXT("Alex is the second companion"), FString(SolidEvents::RosterName(1)), FString(TEXT("Alex")));

	SolidEvents::FState Skipped;
	const SolidEvents::FResult OtherTown = SolidEvents::NotifyOccupiedTown(Skipped, 4);
	TestFalse(TEXT("Jethan has no event"), OtherTown.bFired);
	TestEqual(TEXT("Jethan leaves the party empty"), Skipped.MaxPartySize, 0);
	TestFalse(TEXT("Jethan does not unlock raven sight"), Skipped.bRavenSightEnabled);

	const SolidEvents::FResult RelionFirst = SolidEvents::NotifyOccupiedTown(Skipped, 1);
	TestTrue(TEXT("town 1 still fires if Iglin was skipped"), RelionFirst.bFired);
	TestFalse(TEXT("skipped Iglin does not enable raven sight"), Skipped.bRavenSightEnabled);
	TestEqual(TEXT("town 1 still fields Sam and Alex"), SolidEvents::PartyCount(Skipped), 2);

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
