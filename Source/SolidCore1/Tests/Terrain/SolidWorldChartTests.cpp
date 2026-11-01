#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString ChartTestGrid(const TArray<TPair<FIntPoint, TCHAR>>& Painted, const FString& Lines)
	{
		FString Text;
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			FString Line;
			Line.Reserve(USolidWorldChart::MapWidth);
			for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldChart::GrassMarker;
				for (const TPair<FIntPoint, TCHAR>& Cell : Painted)
				{
					if (Cell.Key.X == Col && Cell.Key.Y == Row)
					{
						Marker = Cell.Value;
						break;
					}
				}
				Line.AppendChar(Marker);
			}
			Text += Line;
			Text += TEXT("\n");
		}
		return Text + Lines;
	}

	bool SameBang(const FSolidWorldBang& A, const FSolidWorldBang& B)
	{
		return A.Kind == B.Kind
			&& A.LocationId == B.LocationId
			&& A.Name == B.Name
			&& A.LairId == B.LairId
			&& A.StrongholdType == B.StrongholdType
			&& A.StrongholdSize == B.StrongholdSize
			&& A.Cell == B.Cell;
	}

	bool MapsMatch(FAutomationTestBase& Test, const USolidWorldChart& TextMap, const USolidWorldChart& ChartMap)
	{
		int32 BiomeMiss = 0;
		FIntPoint FirstBiome = FIntPoint::ZeroValue;
		int32 MarkMiss = 0;
		FIntPoint FirstMark = FIntPoint::ZeroValue;
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
			{
				if (TextMap.GetBiomeAtCell(Col, Row) != ChartMap.GetBiomeAtCell(Col, Row))
				{
					if (BiomeMiss == 0)
					{
						FirstBiome = FIntPoint(Col, Row);
					}
					++BiomeMiss;
				}

				FSolidWorldBang TextBang;
				FSolidWorldBang ChartBang;
				const bool bText = TextMap.FindBang(Col, Row, TextBang);
				const bool bChart = ChartMap.FindBang(Col, Row, ChartBang);
				if (bText != bChart || (bText && !SameBang(TextBang, ChartBang)))
				{
					if (MarkMiss == 0)
					{
						FirstMark = FIntPoint(Col, Row);
					}
					++MarkMiss;
				}
			}
		}

		bool bOk = Test.TestEqual(
			*FString::Printf(TEXT("biomes match, first cell %d,%d"), FirstBiome.X, FirstBiome.Y),
			BiomeMiss,
			0);
		bOk &= Test.TestEqual(
			*FString::Printf(TEXT("marks match, first cell %d,%d"), FirstMark.X, FirstMark.Y),
			MarkMiss,
			0);
		return bOk;
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartRulesTest,
	"SolidCore1.WorldMap.ChartRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldChartRulesTest::RunTest(const FString& Parameters)
{
	const FIntPoint IglinCell(4, 4);
	const FIntPoint CaveCell(8, 4);
	const FIntPoint GateCell(12, 8);
	const FIntPoint UnknownCell(2, 10);
	const FIntPoint BareCell(6, 10);
	const FIntPoint DigitCell(3, 12);
	TArray<TPair<FIntPoint, TCHAR>> Painted;
	Painted.Add(TPair<FIntPoint, TCHAR>(IglinCell, USolidWorldChart::LocationMarker));
	Painted.Add(TPair<FIntPoint, TCHAR>(CaveCell, USolidWorldChart::LocationMarker));
	Painted.Add(TPair<FIntPoint, TCHAR>(GateCell, USolidWorldChart::LocationMarker));
	Painted.Add(TPair<FIntPoint, TCHAR>(FIntPoint(GateCell.X + 1, GateCell.Y), USolidWorldChart::LocationMarker));
	Painted.Add(TPair<FIntPoint, TCHAR>(UnknownCell, USolidWorldChart::LocationMarker));
	Painted.Add(TPair<FIntPoint, TCHAR>(BareCell, USolidWorldChart::LocationMarker));
	Painted.Add(TPair<FIntPoint, TCHAR>(DigitCell, TEXT('7')));
	const FString Text = ChartTestGrid(
		Painted,
		FString::Printf(
			TEXT("! %d,%d = Town Iglin\n")
			TEXT("! %d,%d = Stronghold MeagreDungeon\n")
			TEXT("! %d,%d = Town Dragongate\n")
			TEXT("! %d,%d = Town (Dragongate)\n")
			TEXT("! %d,%d = Town Nowhere\n"),
			IglinCell.Y + 1, IglinCell.X + 1,
			CaveCell.Y + 1, CaveCell.X + 1,
			GateCell.Y + 1, GateCell.X + 1,
			GateCell.Y + 1, GateCell.X + 2,
			UnknownCell.Y + 1, UnknownCell.X + 1));

	USolidWorldChart* TextMap = NewObject<USolidWorldChart>();
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	USolidWorldChart* ChartMap = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("rules text loads"), TextMap->LoadFromString(Text))
		|| !TestTrue(TEXT("rules chart imports"), Chart->ImportFromString(Text))
		|| !TestTrue(TEXT("rules chart loads"), ChartMap->LoadFromChart(Chart)))
	{
		return false;
	}

	TextMap->Marks = Chart->Marks;
	TestTrue(TEXT("the small chart matches its text"), MapsMatch(*this, *TextMap, *ChartMap));

	TestEqual(TEXT("an unknown town is not a location"), ChartMap->GetLocationIdAtCell(UnknownCell.X, UnknownCell.Y), INDEX_NONE);
	FSolidWorldBang UnknownBang;
	TestFalse(TEXT("Nowhere is not a mark"), ChartMap->FindBang(UnknownCell.X, UnknownCell.Y, UnknownBang));
	const int32 UnknownIndex = UnknownCell.Y * USolidWorldChart::MapWidth + UnknownCell.X;
	TestEqual(
		TEXT("an unknown town imports as grassland"),
		static_cast<uint8>(Chart->Cells[UnknownIndex]),
		static_cast<uint8>(ESolidBiome::Grassland));

	FSolidWorldBang BareBang;
	TestFalse(TEXT("a bare ! is not a mark"), ChartMap->FindBang(BareCell.X, BareCell.Y, BareBang));
	const int32 BareIndex = BareCell.Y * USolidWorldChart::MapWidth + BareCell.X;
	TestEqual(
		TEXT("a bare ! imports as grassland"),
		static_cast<uint8>(Chart->Cells[BareIndex]),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("a digit imports as grassland"),
		static_cast<uint8>(Chart->Cells[DigitCell.Y * USolidWorldChart::MapWidth + DigitCell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));

	FIntPoint Gate;
	TestTrue(TEXT("Dragongate has a cell"), ChartMap->TownCell(SolidTowns::Dragongate, Gate));
	TestEqual(TEXT("the town cell is its marker"), Gate, GateCell);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidShippedWorldChartTest,
	"SolidCore1.WorldMap.ShippedChartMatchesText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidShippedWorldChartTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = LoadObject<USolidWorldChart>(nullptr, USolidWorldChart::ShippedPath());
	if (!TestNotNull(TEXT("/Game/World/WorldChart"), Chart))
	{
		return false;
	}

	USolidWorldChart* DefaultMap = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("LoadDefault loads the chart"), DefaultMap->LoadDefault()))
	{
		return false;
	}

	const FIntPoint Cave = SolidStronghold::StartingCell();
	FSolidWorldBang CaveBang;
	TestTrue(TEXT("the cave is the Meagre Dungeon mark"), DefaultMap->FindBang(Cave.X, Cave.Y, CaveBang));
	TestEqual(
		TEXT("the cave stays Meagre"),
		static_cast<uint8>(CaveBang.StrongholdSize),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(
		TEXT("the cave stays a Dungeon"),
		static_cast<uint8>(CaveBang.StrongholdType),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the cave row stays 9"), CaveBang.Cell.Y + 1, SolidStronghold::StartingRow);
	TestEqual(TEXT("the cave column stays 17"), CaveBang.Cell.X + 1, SolidStronghold::StartingColumn);

	FIntPoint Iglin;
	TestTrue(TEXT("Iglin stays location 0"), DefaultMap->TownCell(0, Iglin));
	FIntPoint Gate;
	TestTrue(TEXT("Dragongate stays one town"), DefaultMap->TownCell(SolidTowns::Dragongate, Gate));
	return true;
}

#endif
