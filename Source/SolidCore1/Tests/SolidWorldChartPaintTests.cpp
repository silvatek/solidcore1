#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidLair.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"
#include "SolidWorldMap.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString PaintGrid()
	{
		FString Text;
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			Text += FString::ChrN(USolidWorldMap::MapWidth, USolidWorldMap::GrassMarker);
			Text += TEXT("\n");
		}
		return Text;
	}

	USolidWorldChart* GrassChart()
	{
		USolidWorldChart* Chart = NewObject<USolidWorldChart>();
		Chart->ImportFromString(PaintGrid());
		return Chart;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartPaintTest,
	"SolidCore1.WorldChart.Paint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSolidWorldChartPaintTest::RunTest(const FString& Parameters)
{
	const FString Grid = PaintGrid();
	USolidWorldChart* Chart = GrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart) || !TestTrue(TEXT("the chart covers the map"), Chart->Cells.Num() == USolidWorldMap::MapWidth * USolidWorldMap::MapHeight))
	{
		return false;
	}
	TestTrue(TEXT("a fresh import matches its text"), Chart->Disagreement(Grid).IsEmpty());

	FString Error;
	const FIntPoint Cell(4, 6);
	TestFalse(TEXT("swamp is not a paint"), Chart->PaintCell(Cell, ESolidBiome::Swamp, Error));
	TestFalse(TEXT("the swamp error names the palette"), Error.IsEmpty());
	TestFalse(TEXT("road is not a paint"), Chart->PaintCell(Cell, ESolidBiome::Road, Error));
	TestEqual(
		TEXT("a rejected paint leaves grassland"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldMap::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestTrue(TEXT("one cell paints"), Chart->PaintCell(Cell, ESolidBiome::Forest, Error));
	TestEqual(
		TEXT("the cell is forest"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldMap::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Forest));
	TestFalse(TEXT("a paint disagrees with the text"), Chart->Disagreement(Grid).IsEmpty());
	TestTrue(TEXT("undo restores the cell"), Chart->Undo());
	TestEqual(
		TEXT("the cell is grassland again"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldMap::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestTrue(TEXT("undo restored the text"), Chart->Disagreement(Grid).IsEmpty());
	TestFalse(TEXT("a second undo has nothing to put back"), Chart->Undo());

	const TArray<FIntPoint> Run = { FIntPoint(1, 2), FIntPoint(3, 2), FIntPoint(8, 2) };
	TestTrue(TEXT("a drag is one paint"), Chart->PaintRun(Run, ESolidBiome::Desert, Error));
	TestEqual(TEXT("the run's first cell"), static_cast<uint8>(Chart->Cells[2 * USolidWorldMap::MapWidth + 1]), static_cast<uint8>(ESolidBiome::Desert));
	TestEqual(TEXT("a gap in the run stays grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldMap::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("the run's last cell"), static_cast<uint8>(Chart->Cells[2 * USolidWorldMap::MapWidth + 8]), static_cast<uint8>(ESolidBiome::Desert));
	TestTrue(TEXT("one undo clears the whole run"), Chart->Undo());
	TestEqual(TEXT("the run is grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldMap::MapWidth + 1]), static_cast<uint8>(ESolidBiome::Grassland));

	TestTrue(TEXT("a rectangle paints"), Chart->PaintRectangle(FIntPoint(0, 0), FIntPoint(2, 1), ESolidBiome::Sea, Error));
	TestEqual(TEXT("the rectangle's far corner"), static_cast<uint8>(Chart->Cells[1 * USolidWorldMap::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Sea));
	TestEqual(TEXT("outside the rectangle stays grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldMap::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Grassland));
	int32 SeaCells = 0;
	for (int32 Row = 0; Row <= 1; ++Row)
	{
		for (int32 Col = 0; Col <= 2; ++Col)
		{
			if (Chart->Cells[Row * USolidWorldMap::MapWidth + Col] == ESolidBiome::Sea)
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

	TestTrue(TEXT("forest paint leaves the town"), Chart->PaintCell(TownCell, ESolidBiome::Forest, Error));
	TestTrue(TEXT("Iglin is still on the cell"), Chart->MarkAtCell(TownCell) != INDEX_NONE);

	FSolidWorldChartRead TownRead;
	TestTrue(TEXT("the town cell reads"), Chart->ReadCell(TownCell, TownRead));
	TestEqual(TEXT("the town was painted forest"), static_cast<uint8>(TownRead.Painted), static_cast<uint8>(ESolidBiome::Forest));
	TestEqual(TEXT("a named town resolves as town"), static_cast<uint8>(TownRead.Resolved), static_cast<uint8>(ESolidBiome::Town));
	TestTrue(TEXT("the town mark overrides the paint"), TownRead.bMarkOverrides);
	TestTrue(TEXT("the read names the mark"), TownRead.bHasMark);

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
	FSolidWorldChartRead MouthRead;
	TestTrue(TEXT("the mouth reads"), Chart->ReadCell(MouthCell, MouthRead));
	TestEqual(TEXT("the mouth was painted grassland"), static_cast<uint8>(MouthRead.Painted), static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("a lair mouth resolves as mountain"), static_cast<uint8>(MouthRead.Resolved), static_cast<uint8>(ESolidBiome::Mountain));
	TestTrue(TEXT("the mouth overrides the paint"), MouthRead.bMarkOverrides);
	TestEqual(
		TEXT("the mouth is an entrance"),
		static_cast<uint8>(MouthRead.Mark.Kind),
		static_cast<uint8>(ESolidWorldChartMarkKind::LairEntrance));
	TestEqual(TEXT("the mouth keeps the lair id"), MouthRead.Mark.LairId, FString(SolidLair::IglinTunnels.Id));

	TestTrue(TEXT("the mouth paint changes"), Chart->PaintCell(MouthCell, ESolidBiome::Sea, Error));
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

	FSolidWorldChartRead RoadRead;
	TestTrue(TEXT("a route cell reads"), Chart->ReadCell(TownCell, RoadRead));
	TestTrue(TEXT("the road is named on the town cell"), RoadRead.RoadIndexes.Num() > 0);

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
	const int32 NoteIndexOnMap = NoteCell.Y * USolidWorldMap::MapWidth + NoteCell.X;
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

	USolidWorldMap* Noted = NewObject<USolidWorldMap>();
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

#endif
