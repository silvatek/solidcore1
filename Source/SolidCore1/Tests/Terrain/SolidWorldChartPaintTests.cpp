#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidLair.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString PaintGrid()
	{
		FString Text;
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			Text += FString::ChrN(USolidWorldChart::MapWidth, USolidWorldChart::GrassMarker);
			Text += TEXT("\n");
		}
		return Text;
	}

	USolidWorldChart* PaintGrassChart()
	{
		USolidWorldChart* Chart = NewObject<USolidWorldChart>();
		if (Chart)
		{
			Chart->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
		}
		return Chart;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartPaintTest,
	"SolidCore1.WorldChart.Paint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSolidWorldChartPaintTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = PaintGrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart)
		|| !TestTrue(TEXT("the chart covers the map"), Chart->Cells.Num() == USolidWorldChart::MapWidth * USolidWorldChart::MapHeight))
	{
		return false;
	}

	FString Error;
	const FIntPoint Cell(4, 6);
	TestFalse(TEXT("swamp is not a paint"), Chart->PaintRun({ Cell }, ESolidBiome::Swamp, Error));
	TestFalse(TEXT("the swamp error names the palette"), Error.IsEmpty());
	TestFalse(TEXT("road is not a paint"), Chart->PaintRun({ Cell }, ESolidBiome::Road, Error));
	TestEqual(
		TEXT("a rejected paint leaves grassland"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldChart::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestTrue(TEXT("one cell paints"), Chart->PaintRun({ Cell }, ESolidBiome::Forest, Error));
	TestEqual(
		TEXT("the cell is forest"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldChart::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Forest));
	TestTrue(TEXT("undo restores the cell"), Chart->Undo());
	TestEqual(
		TEXT("the cell is grassland again"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldChart::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestFalse(TEXT("a second undo has nothing to put back"), Chart->Undo());

	const TArray<FIntPoint> Run = { FIntPoint(1, 2), FIntPoint(3, 2), FIntPoint(8, 2) };
	TestTrue(TEXT("a drag is one paint"), Chart->PaintRun(Run, ESolidBiome::Desert, Error));
	TestEqual(TEXT("the run's first cell"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 1]), static_cast<uint8>(ESolidBiome::Desert));
	TestEqual(TEXT("a gap in the run stays grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("the run's last cell"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 8]), static_cast<uint8>(ESolidBiome::Desert));
	TestTrue(TEXT("one undo clears the whole run"), Chart->Undo());
	TestEqual(TEXT("the run is grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 1]), static_cast<uint8>(ESolidBiome::Grassland));

	TestTrue(TEXT("a rectangle paints"), Chart->PaintRectangle(FIntPoint(0, 0), FIntPoint(2, 1), ESolidBiome::Sea, Error));
	TestEqual(TEXT("the rectangle's far corner"), static_cast<uint8>(Chart->Cells[1 * USolidWorldChart::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Sea));
	TestEqual(TEXT("outside the rectangle stays grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Grassland));
	int32 SeaCells = 0;
	for (int32 Row = 0; Row <= 1; ++Row)
	{
		for (int32 Col = 0; Col <= 2; ++Col)
		{
			if (Chart->Cells[Row * USolidWorldChart::MapWidth + Col] == ESolidBiome::Sea)
			{
				++SeaCells;
			}
		}
	}
	TestEqual(TEXT("the rectangle is six cells"), SeaCells, 6);
	TestTrue(TEXT("one undo clears the rectangle"), Chart->Undo());

	TestFalse(TEXT("an unknown town is rejected"), Chart->PlaceTown(FIntPoint(5, 5), TEXT("Nowhere"), Error));
	TestTrue(TEXT("the error names Nowhere"), Error.Contains(TEXT("Nowhere")));
	TestEqual(TEXT("the unknown town left no mark"), Chart->MarkAtCell(FIntPoint(5, 5)), INDEX_NONE);

	const FIntPoint TownCell(10, 4);
	const FIntPoint HoldCell(14, 4);
	TestTrue(TEXT("Iglin places"), Chart->PlaceTown(TownCell, TEXT("Iglin"), Error));
	TestTrue(TEXT("the cave places"), Chart->PlaceStronghold(HoldCell, ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon, Error));
	const int32 HoldIndex = Chart->MarkAtCell(HoldCell);
	if (TestTrue(TEXT("the cave is on its cell"), HoldIndex != INDEX_NONE))
	{
		TestEqual(TEXT("the cave's name"), Chart->Marks[HoldIndex].Name, FString(TEXT("Meagre Dungeon")));
		FVector2D Centre = FVector2D::ZeroVector;
		TestTrue(TEXT("the cave cell has a centre"), USolidWorldChart::CellCentre(HoldCell, Centre));
		TestTrue(TEXT("the cave sits on the cell centre"), FVector2D::Distance(Chart->Marks[HoldIndex].PositionCm, Centre) <= 1.f);
	}

	TestTrue(TEXT("forest paint leaves the town"), Chart->PaintRun({ TownCell }, ESolidBiome::Forest, Error));
	const int32 TownMark = Chart->MarkAtCell(TownCell);
	TestTrue(TEXT("Iglin is still on the cell"), TownMark != INDEX_NONE);
	TestTrue(TEXT("the read names the mark"), TownMark != INDEX_NONE);
	const int32 TownCellIndex = TownCell.Y * USolidWorldChart::MapWidth + TownCell.X;
	TestEqual(
		TEXT("the town was painted forest"),
		static_cast<uint8>(Chart->Cells[TownCellIndex]),
		static_cast<uint8>(ESolidBiome::Forest));
	USolidWorldChart* TownSolved = NewObject<USolidWorldChart>();
	TestTrue(TEXT("the town cell loads"), TownSolved && TownSolved->LoadFromChart(Chart));
	if (TownSolved)
	{
		const ESolidBiome TownResolved = TownSolved->GetBiomeAtCell(TownCell.X, TownCell.Y);
		TestEqual(
			TEXT("a named town resolves as town"),
			static_cast<uint8>(TownResolved),
			static_cast<uint8>(ESolidBiome::Town));
		TestTrue(TEXT("the town mark overrides the paint"), TownResolved != Chart->Cells[TownCellIndex]);
	}

	const FIntPoint SiteCell(2, 8);
	TestTrue(TEXT("a site places"), Chart->PlaceSite(SiteCell, ESolidStrongholdSize::Humble, Error));
	const int32 SiteIndex = Chart->MarkAtCell(SiteCell);
	if (TestTrue(TEXT("the site is on its cell"), SiteIndex != INDEX_NONE))
	{
		TestEqual(TEXT("a site stores the size name"), Chart->Marks[SiteIndex].Name, FString(TEXT("Humble")));
		TestEqual(
			TEXT("a site has no type of its own"),
			static_cast<uint8>(Chart->Marks[SiteIndex].Kind),
			static_cast<uint8>(ESolidWorldChartMarkKind::StrongholdSite));
	}

	const FIntPoint MouthCell(3, 3);
	TestTrue(TEXT("the lair mouth places"), Chart->PlaceLairMouth(MouthCell, SolidLair::IglinTunnels.Name, true, Error));
	const int32 MouthMark = Chart->MarkAtCell(MouthCell);
	TestTrue(TEXT("the mouth reads"), MouthMark != INDEX_NONE);
	const int32 MouthCellIndex = MouthCell.Y * USolidWorldChart::MapWidth + MouthCell.X;
	TestEqual(
		TEXT("the mouth was painted grassland"),
		static_cast<uint8>(Chart->Cells[MouthCellIndex]),
		static_cast<uint8>(ESolidBiome::Grassland));
	USolidWorldChart* MouthSolved = NewObject<USolidWorldChart>();
	TestTrue(TEXT("the mouth loads"), MouthSolved && MouthSolved->LoadFromChart(Chart));
	if (MouthSolved)
	{
		const ESolidBiome MouthResolved = MouthSolved->GetBiomeAtCell(MouthCell.X, MouthCell.Y);
		TestEqual(
			TEXT("a lair mouth resolves as mountain"),
			static_cast<uint8>(MouthResolved),
			static_cast<uint8>(ESolidBiome::Mountain));
		TestTrue(TEXT("the mouth overrides the paint"), MouthResolved != Chart->Cells[MouthCellIndex]);
	}
	if (MouthMark != INDEX_NONE)
	{
		TestEqual(
			TEXT("the mouth is an entrance"),
			static_cast<uint8>(Chart->Marks[MouthMark].Kind),
			static_cast<uint8>(ESolidWorldChartMarkKind::LairEntrance));
		TestEqual(TEXT("the mouth keeps the lair id"), Chart->Marks[MouthMark].LairId, FString(SolidLair::IglinTunnels.Id));
	}

	TestTrue(TEXT("the mouth paint changes"), Chart->PaintRun({ MouthCell }, ESolidBiome::Sea, Error));
	TestTrue(TEXT("the mouth mark stays"), Chart->MarkAtCell(MouthCell) != INDEX_NONE);
	TestTrue(TEXT("delete clears the mouth"), Chart->DeleteMark(MouthCell, Error));
	TestEqual(TEXT("the mouth cell has no mark"), Chart->MarkAtCell(MouthCell), INDEX_NONE);
	TestFalse(TEXT("delete again fails"), Chart->DeleteMark(MouthCell, Error));

	TestTrue(TEXT("the road is stored"), Chart->AddRoad(TEXT("Iglin"), TEXT("Meagre Dungeon"), ESolidWorldChartRoadType::Track, Error));
	TestTrue(TEXT("both ends are on the chart"), Error.IsEmpty());
	TArray<FIntPoint> Route;
	TestTrue(TEXT("the road solves"), Chart->RouteForRoad(Chart->Roads.Num() - 1, Route));
	TestTrue(TEXT("the strip has cells"), Route.Num() > 1);
	TestTrue(TEXT("the strip starts at a town or the cave"), Route.Contains(TownCell) || Route.Contains(HoldCell));
	TestTrue(TEXT("the strip reaches the other end"), Route.Contains(TownCell) && Route.Contains(HoldCell));

	USolidWorldChart* RoadSolved = NewObject<USolidWorldChart>();
	TestTrue(TEXT("a route cell reads"), RoadSolved && RoadSolved->LoadFromChart(Chart));
	bool bTownOnARoad = false;
	if (RoadSolved)
	{
		for (const SolidRoad::FRoute& Solved : RoadSolved->GetRoads())
		{
			if (Solved.Cells.Contains(TownCell))
			{
				bTownOnARoad = true;
				break;
			}
		}
	}
	TestTrue(TEXT("the road is named on the town cell"), bTownOnARoad);

	const int32 RoadsBefore = Chart->Roads.Num();
	TestTrue(TEXT("a missing end is still stored"), Chart->AddRoad(TEXT("Iglin"), TEXT("Nowhere"), ESolidWorldChartRoadType::Trail, Error));
	TestTrue(TEXT("the tool names Nowhere"), Error.Contains(TEXT("Nowhere")));
	TestEqual(TEXT("the missing road was stored"), Chart->Roads.Num(), RoadsBefore + 1);
	TArray<FIntPoint> Missing;
	TestTrue(TEXT("the missing road solves to nothing"), Chart->RouteForRoad(Chart->Roads.Num() - 1, Missing));
	TestEqual(TEXT("a missing end has an empty strip"), Missing.Num(), 0);

	TestTrue(TEXT("a site road is stored"), Chart->AddRoad(TEXT("Humble"), TEXT("Iglin"), ESolidWorldChartRoadType::Road, Error));
	TestTrue(TEXT("a site is not a road end"), Error.Contains(TEXT("stronghold site")));
	TArray<FIntPoint> SiteRoute;
	Chart->RouteForRoad(Chart->Roads.Num() - 1, SiteRoute);
	TestEqual(TEXT("a site road has an empty strip"), SiteRoute.Num(), 0);

	TestTrue(TEXT("undo removes the site road"), Chart->Undo());

	const FIntPoint NoteCell(90, 50);
	const int32 NoteIndexOnMap = NoteCell.Y * USolidWorldChart::MapWidth + NoteCell.X;
	const uint8 PaintBefore = static_cast<uint8>(Chart->Cells[NoteIndexOnMap]);
	TestFalse(TEXT("an empty note is rejected"), Chart->PlaceCustom(NoteCell, TEXT("   "), Error));
	TestTrue(TEXT("the empty note says it needs text"), Error.Contains(TEXT("note")));
	TestFalse(TEXT("a long note is rejected"), Chart->PlaceCustom(NoteCell, FString::ChrN(49, TEXT('a')), Error));
	TestTrue(TEXT("a note places"), Chart->PlaceCustom(NoteCell, TEXT("  Bridge  here  "), Error));
	const int32 NoteMark = Chart->MarkAtCell(NoteCell);
	TestTrue(TEXT("the note is on the cell"), NoteMark != INDEX_NONE);
	if (NoteMark != INDEX_NONE)
	{
		TestEqual(
			TEXT("the note kind"),
			static_cast<uint8>(Chart->Marks[NoteMark].Kind),
			static_cast<uint8>(ESolidWorldChartMarkKind::Custom));
		TestEqual(TEXT("the note is one trimmed line"), Chart->Marks[NoteMark].Name, FString(TEXT("Bridge here")));
	}
	TestEqual(TEXT("a note leaves the paint"), static_cast<uint8>(Chart->Cells[NoteIndexOnMap]), PaintBefore);

	USolidWorldChart* Noted = NewObject<USolidWorldChart>();
	TestTrue(TEXT("a chart with a note still loads"), Noted->LoadFromChart(Chart));
	TestEqual(
		TEXT("a note does not change the biome"),
		static_cast<uint8>(Noted->GetBiomeAtCell(NoteCell.X, NoteCell.Y)),
		PaintBefore);
	FSolidWorldBang NotedBang;
	TestFalse(TEXT("a note is not a world place"), Noted->FindBang(NoteCell.X, NoteCell.Y, NotedBang));

	TestTrue(TEXT("a note is not a road end"), Chart->AddRoad(TEXT("Bridge here"), TEXT("Iglin"), ESolidWorldChartRoadType::Trail, Error));
	TestTrue(TEXT("the tool does not treat the note as a town"), Error.Contains(TEXT("not a town or stronghold")));
	TestTrue(TEXT("undo removes the note road"), Chart->Undo());
	TestTrue(TEXT("undo removes the note"), Chart->Undo());
	TestEqual(TEXT("the cell has no mark"), Chart->MarkAtCell(NoteCell), static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartEncounterTest,
	"SolidCore1.WorldChart.Encounter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSolidWorldChartEncounterTest::RunTest(const FString& Parameters)
{
	const FString Grid = PaintGrid();
	USolidWorldChart* Chart = PaintGrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart))
	{
		return false;
	}

	USolidWorldChart* FromText = NewObject<USolidWorldChart>();
	TestTrue(TEXT("the text has no encounters"), FromText && FromText->LoadFromString(Grid));
	if (FromText)
	{
		TestEqual(TEXT("LoadFromString keeps no encounter"), FromText->GetEncounterMarks().Num(), 0);
	}

	FString Error;
	const FIntPoint Cell(12, 6);
	TestFalse(TEXT("an unknown encounter is rejected"), Chart->PlaceEncounter(Cell, TEXT("Nope"), Error));
	TestTrue(TEXT("the error names Nope"), Error.Contains(TEXT("Nope")));
	TestEqual(TEXT("the unknown name left no mark"), Chart->MarkAtCell(Cell), static_cast<int32>(INDEX_NONE));

	const int32 CellIndex = Cell.Y * USolidWorldChart::MapWidth + Cell.X;
	const uint8 PaintBefore = static_cast<uint8>(Chart->Cells[CellIndex]);
	TestTrue(TEXT("Iglin places first"), Chart->PlaceTown(Cell, TEXT("Iglin"), Error));
	TestTrue(TEXT("the encounter replaces the town"), Chart->PlaceEncounter(Cell, TEXT("Iglin Bridge"), Error));
	const int32 MarkIndex = Chart->MarkAtCell(Cell);
	if (!TestTrue(TEXT("the encounter is on the cell"), MarkIndex != INDEX_NONE))
	{
		return false;
	}
	TestEqual(
		TEXT("the mark is an encounter"),
		static_cast<uint8>(Chart->Marks[MarkIndex].Kind),
		static_cast<uint8>(ESolidWorldChartMarkKind::Encounter));
	TestEqual(TEXT("the catalog name is stored"), Chart->Marks[MarkIndex].Name, FString(TEXT("Iglin Bridge")));
	FVector2D Centre = FVector2D::ZeroVector;
	TestTrue(TEXT("the cell has a centre"), USolidWorldChart::CellCentre(Cell, Centre));
	TestTrue(
		TEXT("the encounter sits on the cell centre"),
		FVector2D::Distance(Chart->Marks[MarkIndex].PositionCm, Centre) <= 1.f);
	TestEqual(TEXT("the encounter leaves the paint"), static_cast<uint8>(Chart->Cells[CellIndex]), PaintBefore);

	TestTrue(TEXT("paint leaves the mark"), Chart->PaintRun({ Cell }, ESolidBiome::Forest, Error));
	const int32 EncounterMark = Chart->MarkAtCell(Cell);
	TestTrue(TEXT("the encounter is still on the cell"), EncounterMark != INDEX_NONE);
	TestTrue(TEXT("the read has the mark"), EncounterMark != INDEX_NONE);
	TestEqual(
		TEXT("the paint is forest"),
		static_cast<uint8>(Chart->Cells[CellIndex]),
		static_cast<uint8>(ESolidBiome::Forest));
	if (EncounterMark != INDEX_NONE)
	{
		TestEqual(TEXT("the read names the encounter"), Chart->Marks[EncounterMark].Name, FString(TEXT("Iglin Bridge")));
	}

	USolidWorldChart* Loaded = NewObject<USolidWorldChart>();
	TestTrue(TEXT("the chart loads"), Loaded && Loaded->LoadFromChart(Chart));
	if (Loaded)
	{
		const ESolidBiome Resolved = Loaded->GetBiomeAtCell(Cell.X, Cell.Y);
		TestEqual(
			TEXT("the loaded cell stays forest"),
			static_cast<uint8>(Resolved),
			static_cast<uint8>(ESolidBiome::Forest));
		TestEqual(
			TEXT("the world map leaves the cell as painted"),
			static_cast<uint8>(Resolved),
			static_cast<uint8>(ESolidBiome::Forest));
		TestFalse(TEXT("an encounter does not override the paint"), Resolved != Chart->Cells[CellIndex]);
		FSolidWorldBang Bang;
		TestFalse(TEXT("an encounter is not a world place"), Loaded->FindBang(Cell.X, Cell.Y, Bang));
		TestEqual(TEXT("the load keeps the encounter"), Loaded->GetEncounterMarks().Num(), 1);
		if (Loaded->GetEncounterMarks().Num() == 1)
		{
			TestEqual(TEXT("the kept name"), Loaded->GetEncounterMarks()[0].Name, FString(TEXT("Iglin Bridge")));
			TestTrue(
				TEXT("the kept position is the cell centre"),
				FVector2D::Distance(Loaded->GetEncounterMarks()[0].PositionCm, Centre) <= 1.f);
		}
	}

	TestTrue(TEXT("delete clears the encounter"), Chart->DeleteMark(Cell, Error));
	TestEqual(TEXT("the cell has no mark"), Chart->MarkAtCell(Cell), static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("delete again fails"), Chart->DeleteMark(Cell, Error));
	TestTrue(TEXT("undo restores the encounter"), Chart->Undo());
	TestTrue(TEXT("the encounter is back"), Chart->MarkAtCell(Cell) != INDEX_NONE);

	const FIntPoint DracorexCell(20, 8);
	TestFalse(TEXT("an unknown encounter is still rejected"), Chart->PlaceEncounter(DracorexCell, TEXT("Nope"), Error));
	TestTrue(TEXT("Dracorex places"), Chart->PlaceEncounter(DracorexCell, TEXT("dracorex"), Error));
	const int32 DracorexIndex = Chart->MarkAtCell(DracorexCell);
	TestTrue(TEXT("Dracorex is on the cell"), DracorexIndex != INDEX_NONE);
	if (DracorexIndex != INDEX_NONE)
	{
		TestEqual(TEXT("the stored name is Dracorex"), Chart->Marks[DracorexIndex].Name, FString(TEXT("Dracorex")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartPlaceMovesTest,
	"SolidCore1.WorldChart.PlaceMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSolidWorldChartPlaceMovesTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = PaintGrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart))
	{
		return false;
	}

	auto CountKind = [](const USolidWorldChart& World, const ESolidWorldChartMarkKind Kind)
	{
		int32 Count = 0;
		for (const FSolidWorldChartMark& Mark : World.Marks)
		{
			if (Mark.Kind == Kind)
			{
				++Count;
			}
		}
		return Count;
	};

	FString Error;
	const FIntPoint Mouth(3, 3);
	const FIntPoint MouthMoved(20, 20);
	TestTrue(TEXT("the entrance places"), Chart->PlaceLairMouth(Mouth, SolidLair::IglinTunnels.Name, true, Error));
	FVector2D BoundsMin = FVector2D::ZeroVector;
	FVector2D BoundsMax = FVector2D::ZeroVector;
	USolidWorldChart::DefaultWorldBounds(BoundsMin, BoundsMax);
	FVector2D MouthCentre = FVector2D::ZeroVector;
	TestTrue(TEXT("the entrance cell has a centre"), USolidWorldChart::CellCentre(Mouth, MouthCentre));
	FIntPoint MouthPoint = FIntPoint::ZeroValue;
	TestTrue(TEXT("the entrance point"), Chart->TerrainPointAt(MouthCentre.X, MouthCentre.Y, BoundsMin, BoundsMax, MouthPoint));
	TestTrue(TEXT("forest covers the entrance"), Chart->PaintBrush({ MouthPoint }, 1, ESolidBiome::Forest, Error));
	TestEqual(
		TEXT("the entrance point stays grassland"),
		static_cast<uint8>(Chart->GetPointBiome(MouthPoint)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("the point beside the entrance is forest"),
		static_cast<uint8>(Chart->GetPointBiome(MouthPoint + FIntPoint(1, 0))),
		static_cast<uint8>(ESolidBiome::Forest));

	TestTrue(TEXT("the entrance moves"), Chart->PlaceLairMouth(MouthMoved, SolidLair::IglinTunnels.Name, true, Error));
	TestEqual(TEXT("one entrance"), CountKind(*Chart, ESolidWorldChartMarkKind::LairEntrance), 1);
	TestEqual(TEXT("the old entrance cell is empty"), Chart->MarkAtCell(Mouth), static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("the entrance is on the new cell"), Chart->MarkAtCell(MouthMoved) != INDEX_NONE);
	TestEqual(
		TEXT("the old entrance point takes the forest beside it"),
		static_cast<uint8>(Chart->GetPointBiome(MouthPoint)),
		static_cast<uint8>(ESolidBiome::Forest));
	FVector2D MovedCentre = FVector2D::ZeroVector;
	TestTrue(TEXT("the new cell has a centre"), USolidWorldChart::CellCentre(MouthMoved, MovedCentre));
	FIntPoint MovedPoint = FIntPoint::ZeroValue;
	TestTrue(TEXT("the new entrance point"), Chart->TerrainPointAt(MovedCentre.X, MovedCentre.Y, BoundsMin, BoundsMax, MovedPoint));
	TestEqual(
		TEXT("the new entrance point is grassland"),
		static_cast<uint8>(Chart->GetPointBiome(MovedPoint)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestTrue(TEXT("undo puts the entrance back"), Chart->Undo());
	TestTrue(TEXT("the entrance is on the old cell"), Chart->MarkAtCell(Mouth) != INDEX_NONE);
	TestEqual(TEXT("the new cell is empty again"), Chart->MarkAtCell(MouthMoved), static_cast<int32>(INDEX_NONE));

	const FIntPoint Exit(4, 4);
	const FIntPoint ExitMoved(9, 9);
	TestTrue(TEXT("the exit places"), Chart->PlaceLairMouth(Exit, SolidLair::IglinTunnels.Name, false, Error));
	TestTrue(TEXT("the exit moves"), Chart->PlaceLairMouth(ExitMoved, SolidLair::IglinTunnels.Name, false, Error));
	TestEqual(TEXT("one exit"), CountKind(*Chart, ESolidWorldChartMarkKind::LairExit), 1);
	TestEqual(TEXT("the old exit cell is empty"), Chart->MarkAtCell(Exit), static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("the entrance stayed"), Chart->MarkAtCell(Mouth) != INDEX_NONE);
	TestTrue(TEXT("the exit is on the new cell"), Chart->MarkAtCell(ExitMoved) != INDEX_NONE);

	const FIntPoint Town(10, 4);
	const FIntPoint TownMoved(12, 6);
	TestTrue(TEXT("Iglin places"), Chart->PlaceTown(Town, TEXT("Iglin"), Error));
	TestTrue(TEXT("Iglin moves"), Chart->PlaceTown(TownMoved, TEXT("iglin"), Error));
	TestEqual(TEXT("one Iglin"), CountKind(*Chart, ESolidWorldChartMarkKind::Town), 1);
	TestEqual(TEXT("the old Iglin cell is empty"), Chart->MarkAtCell(Town), static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("Iglin is on the new cell"), Chart->MarkAtCell(TownMoved) != INDEX_NONE);

	const FIntPoint Cave(14, 4);
	const FIntPoint CaveMoved(16, 4);
	TestTrue(
		TEXT("the cave places"),
		Chart->PlaceStronghold(Cave, ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon, Error));
	TestTrue(
		TEXT("a Meagre fortress moves the cave"),
		Chart->PlaceStronghold(CaveMoved, ESolidStrongholdSize::Meagre, ESolidStrongholdType::Fortress, Error));
	TestEqual(TEXT("one Meagre stronghold"), CountKind(*Chart, ESolidWorldChartMarkKind::Stronghold), 1);
	TestEqual(TEXT("the cave cell is empty"), Chart->MarkAtCell(Cave), static_cast<int32>(INDEX_NONE));
	const int32 Fortress = Chart->MarkAtCell(CaveMoved);
	if (TestTrue(TEXT("the fortress is on the new cell"), Fortress != INDEX_NONE))
	{
		TestEqual(
			TEXT("the moved stronghold is a fortress"),
			static_cast<uint8>(Chart->Marks[Fortress].StrongholdType),
			static_cast<uint8>(ESolidStrongholdType::Fortress));
	}
	const FIntPoint HumbleHold(18, 4);
	TestTrue(
		TEXT("a Humble stronghold stays"),
		Chart->PlaceStronghold(HumbleHold, ESolidStrongholdSize::Humble, ESolidStrongholdType::Dungeon, Error));
	TestEqual(TEXT("two stronghold sizes"), CountKind(*Chart, ESolidWorldChartMarkKind::Stronghold), 2);

	const FIntPoint Site(2, 8);
	const FIntPoint SiteMoved(5, 8);
	TestTrue(TEXT("a Humble site places"), Chart->PlaceSite(Site, ESolidStrongholdSize::Humble, Error));
	TestTrue(TEXT("the Humble site moves"), Chart->PlaceSite(SiteMoved, ESolidStrongholdSize::Humble, Error));
	TestEqual(TEXT("one Humble site"), CountKind(*Chart, ESolidWorldChartMarkKind::StrongholdSite), 1);
	TestEqual(TEXT("the old site cell is empty"), Chart->MarkAtCell(Site), static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("the site is on the new cell"), Chart->MarkAtCell(SiteMoved) != INDEX_NONE);
	TestEqual(TEXT("both strongholds stayed"), CountKind(*Chart, ESolidWorldChartMarkKind::Stronghold), 2);
	const FIntPoint ImperiousSite(6, 8);
	TestTrue(TEXT("an Imperious site stays"), Chart->PlaceSite(ImperiousSite, ESolidStrongholdSize::Imperious, Error));
	TestEqual(TEXT("two site sizes"), CountKind(*Chart, ESolidWorldChartMarkKind::StrongholdSite), 2);

	const FIntPoint Bridge(10, 10);
	const FIntPoint BridgeMoved(11, 11);
	TestTrue(TEXT("Iglin Bridge places"), Chart->PlaceEncounter(Bridge, TEXT("Iglin Bridge"), Error));
	TestTrue(TEXT("Iglin Bridge moves"), Chart->PlaceEncounter(BridgeMoved, TEXT("iglin bridge"), Error));
	TestEqual(TEXT("one Iglin Bridge"), CountKind(*Chart, ESolidWorldChartMarkKind::Encounter), 1);
	TestEqual(TEXT("the old bridge cell is empty"), Chart->MarkAtCell(Bridge), static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("Iglin Bridge is on the new cell"), Chart->MarkAtCell(BridgeMoved) != INDEX_NONE);

	const FIntPoint Note(6, 6);
	const FIntPoint NoteMoved(7, 7);
	TestTrue(TEXT("the note places"), Chart->PlaceCustom(Note, TEXT("Lookout"), Error));
	TestTrue(TEXT("the same note moves"), Chart->PlaceCustom(NoteMoved, TEXT("Lookout"), Error));
	TestEqual(TEXT("one Lookout"), CountKind(*Chart, ESolidWorldChartMarkKind::Custom), 1);
	TestEqual(TEXT("the old note cell is empty"), Chart->MarkAtCell(Note), static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("a different note stays"), Chart->PlaceCustom(FIntPoint(8, 8), TEXT("Ridge"), Error));
	TestEqual(TEXT("two notes"), CountKind(*Chart, ESolidWorldChartMarkKind::Custom), 2);

	const FIntPoint Occupied(22, 12);
	TestTrue(TEXT("a town occupies a cell"), Chart->PlaceTown(Occupied, TEXT("Relion"), Error));
	TestTrue(TEXT("Iglin replaces Relion and leaves its old cell"), Chart->PlaceTown(Occupied, TEXT("Iglin"), Error));
	TestEqual(TEXT("Iglin is still one town"), CountKind(*Chart, ESolidWorldChartMarkKind::Town), 1);
	TestEqual(TEXT("the earlier Iglin cell is empty"), Chart->MarkAtCell(TownMoved), static_cast<int32>(INDEX_NONE));
	TestTrue(TEXT("Iglin is on Relion's cell"), Chart->MarkAtCell(Occupied) != INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartBridgeTest,
	"SolidCore1.WorldChart.Bridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSolidWorldChartBridgeTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = PaintGrassChart();
	UTEST_NOT_NULL(TEXT("grass chart"), Chart);

	FString Error;
	const FIntPoint Cell(14, 8);
	TestFalse(TEXT("an unknown bridge is rejected"), Chart->PlaceBridge(Cell, TEXT("Nope"), Error));
	TestTrue(TEXT("the error names Nope"), Error.Contains(TEXT("Nope")));
	TestEqual(TEXT("the unknown name left no mark"), Chart->MarkAtCell(Cell), static_cast<int32>(INDEX_NONE));

	const int32 CellIndex = Cell.Y * USolidWorldChart::MapWidth + Cell.X;
	const uint8 PaintBefore = static_cast<uint8>(Chart->Cells[CellIndex]);
	TestTrue(TEXT("Iglin Bridge places"), Chart->PlaceBridge(Cell, TEXT("Iglin Bridge"), Error));
	const int32 MarkIndex = Chart->MarkAtCell(Cell);
	if (!TestTrue(TEXT("the bridge is on the cell"), MarkIndex != INDEX_NONE))
	{
		return false;
	}
	TestEqual(
		TEXT("the mark is a bridge"),
		static_cast<uint8>(Chart->Marks[MarkIndex].Kind),
		static_cast<uint8>(ESolidWorldChartMarkKind::Bridge));
	TestEqual(TEXT("the list name is stored"), Chart->Marks[MarkIndex].Name, FString(TEXT("Iglin Bridge")));
	FVector2D Centre = FVector2D::ZeroVector;
	TestTrue(TEXT("the cell has a centre"), USolidWorldChart::CellCentre(Cell, Centre));
	TestTrue(
		TEXT("the bridge sits on the cell centre"),
		FVector2D::Distance(Chart->Marks[MarkIndex].PositionCm, Centre) <= 1.f);
	TestEqual(TEXT("the bridge leaves the paint"), static_cast<uint8>(Chart->Cells[CellIndex]), PaintBefore);

	const FIntPoint Moved(15, 9);
	TestTrue(TEXT("the same bridge moves"), Chart->PlaceBridge(Moved, TEXT("iglin bridge"), Error));
	TestEqual(TEXT("the old cell is empty"), Chart->MarkAtCell(Cell), static_cast<int32>(INDEX_NONE));
	const int32 MovedIndex = Chart->MarkAtCell(Moved);
	TestTrue(TEXT("Iglin Bridge is on the new cell"), MovedIndex != INDEX_NONE);
	if (MovedIndex != INDEX_NONE)
	{
		FVector2D MovedCentre = FVector2D::ZeroVector;
		TestTrue(TEXT("the new cell has a centre"), USolidWorldChart::CellCentre(Moved, MovedCentre));
		TestTrue(
			TEXT("the moved bridge sits on that centre"),
			FVector2D::Distance(Chart->Marks[MovedIndex].PositionCm, MovedCentre) <= 1.f);
	}
	return true;
}

#endif
