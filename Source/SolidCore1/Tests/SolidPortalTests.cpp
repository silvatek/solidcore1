#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
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
	TestEqual(TEXT("seven towns"), SolidPortal::TownCount(), 7);

	const int32 ExpectedIds[] = { 0, 1, 2, 3, 4, 5, 6 };
	const TCHAR* Expected[] = {
		TEXT("Iglin"), TEXT("Relion"), TEXT("Kanfold"), TEXT("Visolar"), TEXT("Jethan"),
		TEXT("Dragongate"), TEXT("Rathen") };
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		TestEqual(TEXT("town id"), Choices[Index].TownId, ExpectedIds[Index]);
		TestEqual(TEXT("town name"), Choices[Index].Name, FString(Expected[Index]));
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
	TestFalse(TEXT("no eighth row"), SolidPortal::FindChoice(7, Found));

	TestEqual(TEXT("wrap down"), SolidPortal::WrapIndex(0, 1), 1);
	TestEqual(TEXT("wrap past end"), SolidPortal::WrapIndex(6, 1), 0);
	TestEqual(TEXT("wrap up"), SolidPortal::WrapIndex(0, -1), 6);
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
	const FVector Delta = SolidPortal::PartyDelta(Captain, Landing);
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
	TestTrue(TEXT("Kanfold"), Map->FindLocation(2, Kanfold));
	TestEqual(TEXT("Kanfold is one pair"), Kanfold.Cells.Num(), 2);
	if (Kanfold.Cells.Num() == 2)
	{
		const int32 Row = Kanfold.Cells[0].Y;
		TestEqual(TEXT("Kanfold cells share a row"), Kanfold.Cells[1].Y, Row);
		const int32 EastCol = FMath::Min(Kanfold.Cells[0].X, Kanfold.Cells[1].X);
		const int32 WestCol = FMath::Max(Kanfold.Cells[0].X, Kanfold.Cells[1].X);
		TestEqual(TEXT("Kanfold cells are adjacent"), WestCol - EastCol, 1);
		FIntPoint LandingCell = FIntPoint::ZeroValue;
		TestTrue(TEXT("Kanfold landing cell"), SolidPortal::ChooseLandingCell(Kanfold.Cells, LandingCell));
		TestEqual(TEXT("portal stands on the eastern Kanfold cell"), LandingCell.X, EastCol);
		TestEqual(TEXT("portal stands on the Kanfold row"), LandingCell.Y, Row);
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

#endif // WITH_DEV_AUTOMATION_TESTS
