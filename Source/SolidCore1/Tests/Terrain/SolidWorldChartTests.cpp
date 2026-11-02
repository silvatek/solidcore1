#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString ChartTestGrid(const TArray<TPair<FIntPoint, TCHAR>>& Painted)
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
		return Text;
	}

	bool BiomesMatch(FAutomationTestBase& Test, const USolidWorldChart& TextMap, const USolidWorldChart& ChartMap)
	{
		int32 BiomeMiss = 0;
		FIntPoint FirstBiome = FIntPoint::ZeroValue;
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
			}
		}

		return Test.TestEqual(
			*FString::Printf(TEXT("biomes match, first cell %d,%d"), FirstBiome.X, FirstBiome.Y),
			BiomeMiss,
			0);
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartRulesTest,
	"SolidCore1.WorldMap.ChartRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldChartRulesTest::RunTest(const FString& Parameters)
{
	const FIntPoint BangCell(6, 10);
	const FIntPoint DigitCell(3, 12);
	TArray<TPair<FIntPoint, TCHAR>> Painted;
	Painted.Add(TPair<FIntPoint, TCHAR>(BangCell, USolidWorldChart::LocationMarker));
	Painted.Add(TPair<FIntPoint, TCHAR>(DigitCell, TEXT('7')));
	const FString Text = ChartTestGrid(Painted);

	USolidWorldChart* TextMap = NewObject<USolidWorldChart>();
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	USolidWorldChart* ChartMap = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("rules text loads"), TextMap->LoadFromString(Text))
		|| !TestTrue(TEXT("rules chart imports"), Chart->ImportFromString(Text))
		|| !TestTrue(TEXT("rules chart loads"), ChartMap->LoadFromChart(Chart)))
	{
		return false;
	}

	TestTrue(TEXT("the small chart matches its text"), BiomesMatch(*this, *TextMap, *ChartMap));

	TestEqual(TEXT("a bare ! is not a location"), ChartMap->GetLocationIdAtCell(BangCell.X, BangCell.Y), INDEX_NONE);
	const int32 BangIndex = BangCell.Y * USolidWorldChart::MapWidth + BangCell.X;
	TestEqual(
		TEXT("a bare ! imports as grassland"),
		static_cast<uint8>(Chart->Cells[BangIndex]),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("a digit imports as grassland"),
		static_cast<uint8>(Chart->Cells[DigitCell.Y * USolidWorldChart::MapWidth + DigitCell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));
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
