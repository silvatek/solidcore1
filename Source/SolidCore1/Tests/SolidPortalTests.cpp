#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidMessages.h"
#include "Journal/SolidJournal.h"
#include "Lairs/SolidLair.h"
#include "SolidGameState.h"
#include "Company/SolidCaptain.h"
#include "SolidHUD.h"
#include "SolidPortal.h"
#include "Terrain/SolidWorldMap.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPortalChoicesTest,
	"SolidCore1.Portal.Choices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPortalChoicesTest::RunTest(const FString& Parameters)
{
	TArray<SolidPortal::FChoice> Choices;
	SolidPortal::CollectChoices(Choices);
	TestEqual(TEXT("one row per town"), Choices.Num(), SolidPortal::TownCount());
	TestTrue(TEXT("lists every town"), SolidPortal::TownCount() >= SolidTowns::DefinitionCount);
	if (Choices.Num() < SolidTowns::DefinitionCount)
	{
		return false;
	}

	for (int32 Index = 0; Index < SolidTowns::DefinitionCount; ++Index)
	{
		TestEqual(TEXT("town id"), Choices[Index].TownId, SolidTowns::Definitions[Index].Id);
		TestEqual(TEXT("town name"), Choices[Index].Name, FString(SolidTowns::Definitions[Index].Name));
	}

	SolidPortal::FChoice Found;
	TestTrue(TEXT("find Relion"), SolidPortal::FindChoice(1, Found));
	TestEqual(TEXT("found name"), Found.Name, FString(TEXT("Relion")));
	TestTrue(TEXT("find Dragongate"), SolidPortal::FindChoice(5, Found));
	TestEqual(TEXT("Dragongate is town 5"), Found.TownId, 5);
	const SolidTowns::FTownInfo* GateTown = SolidTowns::Find(Found.TownId);
	TestNotNull(TEXT("Dragongate info"), GateTown);
	if (GateTown)
	{
		TestEqual(
			TEXT("Dragongate is a wall gate"),
			static_cast<uint8>(GateTown->Type),
			static_cast<uint8>(SolidTowns::ETownType::WallGate));
	}
	TestTrue(TEXT("find Rathen"), SolidPortal::FindChoice(6, Found));
	TestEqual(TEXT("Rathen is town 6"), Found.TownId, 6);
	TestFalse(TEXT("past the list"), SolidPortal::FindChoice(SolidPortal::TownCount(), Found));

	TestEqual(TEXT("wrap down"), SolidPortal::WrapIndex(0, 1), 1);
	TestEqual(TEXT("wrap past end"), SolidPortal::WrapIndex(SolidPortal::TownCount() - 1, 1), 0);
	TestEqual(TEXT("wrap up"), SolidPortal::WrapIndex(0, -1), SolidPortal::TownCount() - 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPortalLandingTest,
	"SolidCore1.Portal.Landing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPortalLandingTest::RunTest(const FString& Parameters)
{
	TArray<FIntPoint> Cells;
	Cells.Add(FIntPoint(10, 5));
	Cells.Add(FIntPoint(3, 8));
	Cells.Add(FIntPoint(1, 5));
	FIntPoint Chosen = FIntPoint::ZeroValue;
	TestTrue(TEXT("picks a cell"), SolidPortal::ChooseLandingCell(Cells, Chosen));
	TestEqual(TEXT("northernmost row"), Chosen.Y, 5);
	TestEqual(TEXT("easternmost on that row"), Chosen.X, 1);

	TArray<FIntPoint> Empty;
	TestFalse(TEXT("no cells"), SolidPortal::ChooseLandingCell(Empty, Chosen));

	const FVector2D Plaza(0.f, -450.f);
	const FVector2D Center(1000.f, 2000.f);
	const FVector2D Landing = SolidPortal::LandingXY(Center, Plaza);
	TestTrue(TEXT("lands south of the cell center"), Landing.Equals(FVector2D(1000.f, 1550.f), 0.1f));

	const FVector Captain(10.f, 20.f, 300.f);
	const FVector Delta = SolidPortal::SquadDelta(Captain, Landing);
	TestTrue(TEXT("delta is planar"), FMath::IsNearlyZero(Delta.Z));
	TestTrue(TEXT("delta reaches the landing"), FVector2D(Captain.X + Delta.X, Captain.Y + Delta.Y).Equals(Landing, 0.1f));

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Map->LoadDefault());
	const float Half = 0.5f * 512.f * 200.f;
	const FVector2D GameMin(-Half, -Half);
	const FVector2D GameMax(Half, Half);
	const float CellEastCm = (GameMax.X - GameMin.X) / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellNorthCm = (GameMax.Y - GameMin.Y) / static_cast<float>(USolidWorldMap::MapHeight);
	TestTrue(TEXT("plaza offset stays inside the cell"),
		FMath::Abs(Plaza.X) < CellEastCm * 0.5f && FMath::Abs(Plaza.Y) < CellNorthCm * 0.5f);

	for (const SolidTowns::FTownInfo& Town : SolidTowns::Definitions)
	{
		const int32 TownId = Town.Id;
		FSolidWorldLocation Location;
		if (!Map->FindLocation(TownId, Location))
		{
			TestTrue(TEXT("only a wall gate may be missing from the map"),
				Town.Type == SolidTowns::ETownType::WallGate);
			continue;
		}
		FIntPoint Cell;
		TestTrue(TEXT("landing cell"), SolidPortal::ChooseLandingCell(Location.Cells, Cell));
		TestTrue(TEXT("cell belongs to the town"), Location.Cells.Contains(Cell));

		FVector2D CellXY = FVector2D::ZeroVector;
		TestTrue(TEXT("cell projects"), Map->CellToWorldXY(Cell, GameMin, GameMax, CellXY));
		const FVector2D TownLanding = SolidPortal::LandingXY(CellXY, Plaza);
		TestEqual(
			*FString::Printf(TEXT("town %d landing is that town"), TownId),
			Map->SampleLocationId(TownLanding.X, TownLanding.Y, GameMin, GameMax),
			TownId);
	}

	FSolidWorldLocation Kanfold;
	TestTrue(TEXT("Kanfold"), Map->FindLocation(SolidTowns::Kanfold, Kanfold));
	TestEqual(TEXT("Kanfold is one cell"), Kanfold.Cells.Num(), 1);
	if (Kanfold.Cells.Num() == 1)
	{
		FIntPoint LandingCell = FIntPoint::ZeroValue;
		TestTrue(TEXT("Kanfold landing cell"), SolidPortal::ChooseLandingCell(Kanfold.Cells, LandingCell));
		TestEqual(TEXT("portal stands on the Kanfold column"), LandingCell.X, Kanfold.Cells[0].X);
		TestEqual(TEXT("portal stands on the Kanfold row"), LandingCell.Y, Kanfold.Cells[0].Y);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPortalHudTest,
	"SolidCore1.Portal.Hud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPortalHudTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	TestFalse(TEXT("portal closed"), HUD->IsPortalOpen());

	HUD->HandleMenuKey();
	TestTrue(TEXT("menu open"), HUD->IsMainMenuOpen());
	HUD->OpenPortal();
	TestTrue(TEXT("portal opens"), HUD->IsPortalOpen());
	TestFalse(TEXT("menu closes"), HUD->IsMainMenuOpen());
	TestEqual(TEXT("first town"), HUD->GetPortalIndex(), 0);

	HUD->MovePortalSelection(1);
	TestEqual(TEXT("down selects the next town"), HUD->GetPortalIndex(), 1);
	HUD->MovePortalSelection(-1);
	HUD->MovePortalSelection(-1);
	TestEqual(TEXT("up wraps to the last town"), HUD->GetPortalIndex(), SolidPortal::TownCount() - 1);

	HUD->SetPortalIndex(99);
	TestEqual(TEXT("index clamps"), HUD->GetPortalIndex(), SolidPortal::TownCount() - 1);

	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes the portal"), HUD->IsPortalOpen());
	TestFalse(TEXT("closing the portal does not open the menu"), HUD->IsMainMenuOpen());

	HUD->OpenPortal();
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes the portal"), HUD->IsPortalOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPortalTunnelMouthTest,
	"SolidCore1.Portal.TunnelMouth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPortalTunnelMouthTest::RunTest(const FString& Parameters)
{
	TArray<SolidPortal::FChoice> Choices;
	SolidPortal::CollectChoices(Choices);
	TestEqual(
		TEXT("tunnels follow the towns"),
		Choices.Num(),
		SolidTowns::DefinitionCount + SolidPortal::ExtraPlaceCount);
	const int32 TunnelIndex = SolidTowns::DefinitionCount;
	if (!TestTrue(TEXT("the tunnels row is on the list"), Choices.IsValidIndex(TunnelIndex)))
	{
		return false;
	}

	const SolidPortal::FChoice& Tunnels = Choices[TunnelIndex];
	TestEqual(TEXT("tunnels name"), Tunnels.Name, FString(SolidLair::IglinTunnels.Name));
	TestEqual(
		TEXT("tunnels place"),
		static_cast<uint8>(Tunnels.Place),
		static_cast<uint8>(SolidPortal::EPlace::TunnelMouth));
	TestEqual(TEXT("tunnels end is X"), static_cast<int32>(Tunnels.TunnelEnd), static_cast<int32>(USolidWorldMap::XMarker));
	TestEqual(TEXT("tunnels are not a town"), Tunnels.TownId, INDEX_NONE);

	SolidPortal::FChoice Found;
	TestTrue(TEXT("find the tunnels"), SolidPortal::FindChoice(TunnelIndex, Found));
	TestEqual(TEXT("found tunnels"), Found.Name, FString(SolidLair::IglinTunnels.Name));

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Map->LoadDefault());
	const float Half = 0.5f * 512.f * 200.f;
	const FVector2D GameMin(-Half, -Half);
	const FVector2D GameMax(Half, Half);
	TArray<SolidLair::FEntrance> Mouths;
	TestTrue(TEXT("X mouth is on the map"), SolidLair::CollectEntrances(Map, GameMin, GameMax, Mouths) >= 1);
	const SolidLair::FEntrance* Mouth = nullptr;
	for (const SolidLair::FEntrance& Entrance : Mouths)
	{
		if (Entrance.EndMarker == USolidWorldMap::XMarker)
		{
			Mouth = &Entrance;
			break;
		}
	}
	if (!TestNotNull(TEXT("X entrance"), Mouth))
	{
		return false;
	}

	const FVector2D Landing = SolidPortal::NorthOfEntrance(Mouth->WorldXY);
	TestTrue(TEXT("landing keeps the mouth's easting"), FMath::IsNearlyEqual(Landing.X, Mouth->WorldXY.X, 0.1f));
	TestTrue(
		TEXT("landing is north of the mouth"),
		FMath::IsNearlyEqual(Landing.Y - Mouth->WorldXY.Y, SolidPortal::NorthOfEntranceCm, 0.1f));
	TestTrue(TEXT("the mouth faces north"), FMath::IsNearlyEqual(Mouth->FacingYawDeg, 90.f, 0.1f));
	TestTrue(
		TEXT("the captain faces the mouth"),
		FMath::IsNearlyEqual(SolidPortal::YawTowardMouth(Mouth->FacingYawDeg), -90.f, 0.1f));

	FVector2D GrassXY = FVector2D::ZeroVector;
	FVector2D CliffXY = FVector2D::ZeroVector;
	TestTrue(TEXT("grass cell projects"), Map->CellToWorldXY(Mouth->GrassCell, GameMin, GameMax, GrassXY));
	TestTrue(TEXT("cliff cell projects"), Map->CellToWorldXY(Mouth->Cell, GameMin, GameMax, CliffXY));
	const float CellEastCm = (GameMax.X - GameMin.X) / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellNorthCm = (GameMax.Y - GameMin.Y) / static_cast<float>(USolidWorldMap::MapHeight);
	TestTrue(
		TEXT("landing stays on the grass cell"),
		FMath::Abs(Landing.X - GrassXY.X) < CellEastCm * 0.5f
			&& FMath::Abs(Landing.Y - GrassXY.Y) < CellNorthCm * 0.5f);
	TestTrue(
		TEXT("landing stays off the cliff"),
		FMath::Abs(Landing.X - CliffXY.X) >= CellEastCm * 0.5f
			|| FMath::Abs(Landing.Y - CliffXY.Y) >= CellNorthCm * 0.5f);
	TestEqual(
		TEXT("landing is grass"),
		static_cast<uint8>(Map->SampleBiome(Landing.X, Landing.Y, GameMin, GameMax)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("landing is not a town"),
		Map->SampleLocationId(Landing.X, Landing.Y, GameMin, GameMax),
		INDEX_NONE);

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	const int32 KilledIndex = SolidEvents::FindIndexById(SolidEvents::IdKilledByMonsters);
	const SolidEvents::FResult Recorded = SolidEvents::MarkComplete(State, StateCaptain, SolidEvents::IdDrivenFromVillage);
	TestTrue(TEXT("the portal records Driven from the village"), Recorded.bFired);
	TestTrue(TEXT("the tunnels open"), SolidLair::TunnelsOpen(State));
	TestFalse(TEXT("the cave stays shut"), State.bStrongholdsEnabled);
	TestTrue(TEXT("killed by monsters is still unfired"), KilledIndex != INDEX_NONE && !State.bFired[KilledIndex]);
	TestTrue(TEXT("the title is set"), Recorded.bSetTitle);
	TestEqual(TEXT("title is Outcast"), FString(CaptainTitleName(StateCaptain.Title)), FString(CaptainTitleName(ESolidCaptainTitle::Outcast)));
	TestTrue(TEXT("the village message is shown"), Recorded.bShowMessage);
	TestEqual(
		TEXT("message id"),
		FString(Recorded.MessageId ? Recorded.MessageId : TEXT("")),
		FString(SolidMessages::IdDrivenFromVillage));

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	bool bNamed = false;
	bool bDetail = false;
	const TCHAR* MessageText = SolidMessages::TextFor(SolidMessages::IdDrivenFromVillage);
	for (const SolidJournal::FRow& JournalRow : Rows)
	{
		if (JournalRow.Kind == SolidJournal::ERowKind::Entry
			&& JournalRow.Text == FString(TEXT("Driven from the village")))
		{
			bNamed = true;
		}
		if (JournalRow.Kind == SolidJournal::ERowKind::Detail
			&& JournalRow.Text.Contains(TEXT("Title Outcast"))
			&& MessageText
			&& JournalRow.Text.Contains(MessageText))
		{
			bDetail = true;
		}
	}
	TestTrue(TEXT("the journal names the event"), bNamed);
	TestTrue(TEXT("the journal records the message and title"), bDetail);

	const int32 FiredCount = State.FiredOrder.Num();
	const int32 JournalCount = State.Journal.Num();
	const SolidEvents::FResult Again = SolidEvents::MarkComplete(State, StateCaptain, SolidEvents::IdDrivenFromVillage);
	TestFalse(TEXT("a second portal does not record it again"), Again.bFired);
	TestEqual(TEXT("fired once"), State.FiredOrder.Num(), FiredCount);
	TestEqual(TEXT("one journal line"), State.Journal.Num(), JournalCount);

	USolidGameState& Empty = *NewObject<USolidGameState>();
	USolidCaptain& EmptyCaptain = *NewObject<USolidCaptain>();
	TestFalse(TEXT("an unknown id does nothing"), SolidEvents::MarkComplete(Empty, EmptyCaptain, TEXT("missing")).bFired);
	TestEqual(TEXT("unknown id leaves the journal empty"), Empty.FiredOrder.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
