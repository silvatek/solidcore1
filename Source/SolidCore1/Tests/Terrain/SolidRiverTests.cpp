#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidRiver.h"
#include "SolidWorldChart.h"

#include <initializer_list>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D RiverTestMin(-51200.f, -51200.f);
	const FVector2D RiverTestMax(51200.f, 51200.f);

	struct FRiverTestMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = USolidWorldChart::GrassMarker;
	};

	FString RiverTestGrid(std::initializer_list<FRiverTestMark> Marks)
	{
		FString Text;
		Text.Reserve((USolidWorldChart::MapWidth + 1) * USolidWorldChart::MapHeight);
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldChart::GrassMarker;
				for (const FRiverTestMark& Mark : Marks)
				{
					if (Mark.Col == Col && Mark.Row == Row)
					{
						Marker = Mark.Marker;
						break;
					}
				}
				Text.AppendChar(Marker);
			}
			Text.AppendChar(TEXT('\n'));
		}
		return Text;
	}

	bool RiverTestBiome(
		FAutomationTestBase& Test,
		const TCHAR* What,
		const ESolidBiome Actual,
		const ESolidBiome Expected)
	{
		return Test.TestEqual(What, static_cast<uint8>(Actual), static_cast<uint8>(Expected));
	}

	ESolidBiome RiverTestSample(const USolidWorldChart& Map, const FVector2D& World)
	{
		return Map.SampleBiome(World.X, World.Y, RiverTestMin, RiverTestMax);
	}

	FVector2D RiverTestCenter(const USolidWorldChart& Map, const int32 Col, const int32 Row)
	{
		FVector2D Center = FVector2D::ZeroVector;
		Map.CellToWorldXY(FIntPoint(Col, Row), RiverTestMin, RiverTestMax, Center);
		return Center;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRiverChannelTest,
	"SolidCore1.River.Channel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRiverChannelTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("river joins"), SolidRiver::Joins(USolidWorldChart::RiverMarker));
	TestTrue(TEXT("sea joins"), SolidRiver::Joins(USolidWorldChart::SeaMarker));
	TestFalse(TEXT("grass does not join"), SolidRiver::Joins(USolidWorldChart::GrassMarker));
	TestTrue(TEXT("width is 4 m"), FMath::IsNearlyEqual(SolidRiver::WidthCm, 400.f));

	const float Cell = 1600.f;
	const SolidRiver::FLinks Pond;
	TestTrue(TEXT("pond centre"), SolidRiver::IsWater(FVector2D::ZeroVector, Cell, Cell, Pond));
	TestTrue(TEXT("pond at 190 cm"), SolidRiver::IsWater(FVector2D(190.f, 0.f), Cell, Cell, Pond));
	TestTrue(TEXT("pond on the rim"), SolidRiver::IsWater(FVector2D(200.f, 0.f), Cell, Cell, Pond));
	TestFalse(TEXT("pond past 210 cm"), SolidRiver::IsWater(FVector2D(210.f, 0.f), Cell, Cell, Pond));
	TestTrue(TEXT("pond inside the diagonal"), SolidRiver::IsWater(FVector2D(140.f, 140.f), Cell, Cell, Pond));
	TestFalse(TEXT("pond outside the diagonal"), SolidRiver::IsWater(FVector2D(150.f, 150.f), Cell, Cell, Pond));

	SolidRiver::FLinks EastWest;
	EastWest.bEast = true;
	EastWest.bWest = true;
	TestTrue(TEXT("channel at 190 cm"), SolidRiver::IsWater(FVector2D(0.f, 190.f), Cell, Cell, EastWest));
	TestFalse(TEXT("bank at 250 cm"), SolidRiver::IsWater(FVector2D(0.f, 250.f), Cell, Cell, EastWest));
	TestTrue(TEXT("channel along the square"), SolidRiver::IsWater(FVector2D(700.f, 0.f), Cell, Cell, EastWest));
	TestTrue(TEXT("channel keeps its width"), SolidRiver::IsWater(FVector2D(700.f, 150.f), Cell, Cell, EastWest));
	TestFalse(TEXT("bank stays narrow"), SolidRiver::IsWater(FVector2D(700.f, 250.f), Cell, Cell, EastWest));
	TestEqual(
		TEXT("north bank faces north"),
		static_cast<uint8>(SolidRiver::Side(FVector2D(0.f, 300.f), Cell, Cell, EastWest)),
		static_cast<uint8>(SolidRiver::ESide::North));
	TestEqual(
		TEXT("south bank faces south"),
		static_cast<uint8>(SolidRiver::Side(FVector2D(0.f, -300.f), Cell, Cell, EastWest)),
		static_cast<uint8>(SolidRiver::ESide::South));

	SolidRiver::FLinks NorthSouth;
	NorthSouth.bNorth = true;
	NorthSouth.bSouth = true;
	TestEqual(
		TEXT("west bank faces west"),
		static_cast<uint8>(SolidRiver::Side(FVector2D(300.f, 0.f), Cell, Cell, NorthSouth)),
		static_cast<uint8>(SolidRiver::ESide::West));
	TestEqual(
		TEXT("east bank faces east"),
		static_cast<uint8>(SolidRiver::Side(FVector2D(-300.f, 0.f), Cell, Cell, NorthSouth)),
		static_cast<uint8>(SolidRiver::ESide::East));
	TestTrue(TEXT("north join reaches the edge"), SolidRiver::IsWater(FVector2D(0.f, 700.f), Cell, Cell, NorthSouth));

	SolidRiver::FLinks EastOnly;
	EastOnly.bEast = true;
	TestTrue(TEXT("sea mouth is water"), SolidRiver::IsWater(FVector2D(-790.f, 0.f), Cell, Cell, EastOnly));
	TestFalse(TEXT("mouth does not widen"), SolidRiver::IsWater(FVector2D(-790.f, 300.f), Cell, Cell, EastOnly));
	TestEqual(
		TEXT("land beside a mouth faces the bank"),
		static_cast<uint8>(SolidRiver::Side(FVector2D(0.f, 300.f), Cell, Cell, EastOnly)),
		static_cast<uint8>(SolidRiver::ESide::North));
	TestTrue(TEXT("shared edge is water"), SolidRiver::IsWater(FVector2D(-800.f, 0.f), Cell, Cell, EastOnly));
	TestFalse(TEXT("shared edge stays 4 m"), SolidRiver::IsWater(FVector2D(-800.f, 250.f), Cell, Cell, EastOnly));
	TestFalse(TEXT("a corner off the channel is dry"), SolidRiver::IsWater(FVector2D(-700.f, 700.f), Cell, Cell, EastOnly));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRiverBanksTest,
	"SolidCore1.River.Banks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRiverBanksTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("map"), Map);
	const bool bLoaded = Map->LoadFromString(RiverTestGrid({
		// Isolated pond. West is mountain, east grass, north forest, south desert.
		{ 10, 10, USolidWorldChart::RiverMarker },
		{ 9, 10, TEXT('M') },
		{ 11, 10, USolidWorldChart::GrassMarker },
		{ 10, 9, USolidWorldChart::ForestMarker },
		{ 10, 11, USolidWorldChart::DesertMarker },
		// North-south channel. Forest to the west, desert to the east.
		{ 20, 18, USolidWorldChart::RiverMarker },
		{ 20, 19, USolidWorldChart::RiverMarker },
		{ 20, 20, USolidWorldChart::RiverMarker },
		{ 19, 18, USolidWorldChart::ForestMarker },
		{ 19, 19, USolidWorldChart::ForestMarker },
		{ 19, 20, USolidWorldChart::ForestMarker },
		{ 21, 18, USolidWorldChart::DesertMarker },
		{ 21, 19, USolidWorldChart::DesertMarker },
		{ 21, 20, USolidWorldChart::DesertMarker },
		// West into the sea. Grass north, desert south, grass east.
		{ 5, 30, USolidWorldChart::RiverMarker },
		{ 4, 30, USolidWorldChart::SeaMarker },
		{ 5, 31, USolidWorldChart::DesertMarker },
		// Sea along the north bank of an east-west channel. Desert to the south.
		{ 9, 40, USolidWorldChart::RiverMarker },
		{ 10, 40, USolidWorldChart::RiverMarker },
		{ 11, 40, USolidWorldChart::RiverMarker },
		{ 10, 39, USolidWorldChart::SeaMarker },
		{ 10, 41, USolidWorldChart::DesertMarker },
		// Bend. The dry corner must not become more river.
		{ 20, 50, USolidWorldChart::RiverMarker },
		{ 19, 50, USolidWorldChart::RiverMarker },
		{ 20, 51, USolidWorldChart::RiverMarker },
		{ 20, 49, USolidWorldChart::ForestMarker },
		{ 21, 50, USolidWorldChart::DesertMarker },
		// Diagonal neighbor does not join.
		{ 50, 50, USolidWorldChart::RiverMarker },
		{ 49, 49, USolidWorldChart::RiverMarker },
	}));
	TestTrue(TEXT("synthetic map loads"), bLoaded);
	if (!bLoaded)
	{
		return false;
	}

	RiverTestBiome(*this, TEXT("grass stays grass"), RiverTestSample(*Map, RiverTestCenter(*Map, 0, 0)), ESolidBiome::Grassland);

	// East-west cells are 8 m. A step copied from the old 16 m square walks into the next cell.
	const float CellW = (RiverTestMax.X - RiverTestMin.X) / static_cast<float>(USolidWorldChart::MapWidth);
	const float CellH = (RiverTestMax.Y - RiverTestMin.Y) / static_cast<float>(USolidWorldChart::MapHeight);
	const float HalfW = CellW * 0.5f;
	const float HalfH = CellH * 0.5f;
	const float Bank = FMath::Min(SolidRiver::RadiusCm() + 100.f, HalfW - 50.f);
	const float Along = HalfW - 100.f;
	const float Edge = HalfH - 10.f;
	const float InChannel = SolidRiver::RadiusCm() * 0.75f;

	const FVector2D Pond = RiverTestCenter(*Map, 10, 10);
	TestEqual(
		TEXT("marker is still river"),
		static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
		static_cast<uint8>(ESolidBiome::River));
	RiverTestBiome(*this, TEXT("pond centre"), RiverTestSample(*Map, Pond), ESolidBiome::River);
	RiverTestBiome(*this, TEXT("west of the pond is mountain"), RiverTestSample(*Map, Pond + FVector2D(Bank, 0.f)), ESolidBiome::Mountain);
	// 400 cm is two terrain steps. The sample east and south of the pond is that point.
	RiverTestBiome(*this, TEXT("east of the pond is grass"), RiverTestSample(*Map, Pond + FVector2D(-400.f, 0.f)), ESolidBiome::Grassland);
	RiverTestBiome(*this, TEXT("north of the pond is forest"), RiverTestSample(*Map, Pond + FVector2D(0.f, Bank)), ESolidBiome::Forest);
	RiverTestBiome(*this, TEXT("south of the pond is desert"), RiverTestSample(*Map, Pond + FVector2D(0.f, -400.f)), ESolidBiome::Desert);

	const FVector2D Channel = RiverTestCenter(*Map, 20, 19);
	RiverTestBiome(*this, TEXT("channel centre"), RiverTestSample(*Map, Channel), ESolidBiome::River);
	RiverTestBiome(*this, TEXT("west bank is forest"), RiverTestSample(*Map, Channel + FVector2D(Bank, 0.f)), ESolidBiome::Forest);
	RiverTestBiome(*this, TEXT("east bank is desert"), RiverTestSample(*Map, Channel + FVector2D(-400.f, 0.f)), ESolidBiome::Desert);
	RiverTestBiome(*this, TEXT("along the channel stays river"), RiverTestSample(*Map, Channel + FVector2D(0.f, HalfH * 0.25f)), ESolidBiome::River);

	const FVector2D NorthCell = RiverTestCenter(*Map, 20, 18);
	RiverTestBiome(*this, TEXT("join between squares"), RiverTestSample(*Map, NorthCell + FVector2D(InChannel, -Edge)), ESolidBiome::River);
	RiverTestBiome(*this, TEXT("bank beside the join"), RiverTestSample(*Map, NorthCell + FVector2D(Bank, -Edge)), ESolidBiome::Forest);

	const FVector2D Mouth = RiverTestCenter(*Map, 5, 30);
	RiverTestBiome(*this, TEXT("mouth channel"), RiverTestSample(*Map, Mouth + FVector2D(Along, 0.f)), ESolidBiome::River);
	RiverTestBiome(*this, TEXT("north of the mouth is grass"), RiverTestSample(*Map, Mouth + FVector2D(0.f, Bank)), ESolidBiome::Grassland);
	RiverTestBiome(*this, TEXT("south of the mouth is desert"), RiverTestSample(*Map, Mouth + FVector2D(0.f, -400.f)), ESolidBiome::Desert);
	RiverTestBiome(*this, TEXT("east of the mouth is grass"), RiverTestSample(*Map, Mouth + FVector2D(-400.f, 0.f)), ESolidBiome::Grassland);

	const FVector2D SeaBank = RiverTestCenter(*Map, 10, 40);
	RiverTestBiome(*this, TEXT("sea bank"), RiverTestSample(*Map, SeaBank + FVector2D(0.f, CellH)), ESolidBiome::Sea);
	RiverTestBiome(*this, TEXT("opposite bank stays desert"), RiverTestSample(*Map, SeaBank + FVector2D(0.f, -400.f)), ESolidBiome::Desert);
	RiverTestBiome(*this, TEXT("channel in front of the sea"), RiverTestSample(*Map, SeaBank + FVector2D(Along, 0.f)), ESolidBiome::River);

	const FVector2D Bend = RiverTestCenter(*Map, 20, 50);
	RiverTestBiome(*this, TEXT("bend centre"), RiverTestSample(*Map, Bend), ESolidBiome::River);
	RiverTestBiome(*this, TEXT("bend arm"), RiverTestSample(*Map, Bend + FVector2D(Along, 0.f)), ESolidBiome::River);
	RiverTestBiome(*this, TEXT("bend corner stays a bank"), RiverTestSample(*Map, Bend + FVector2D(400.f, -400.f)), ESolidBiome::Desert);

	const FVector2D Diagonal = RiverTestCenter(*Map, 50, 50);
	RiverTestBiome(*this, TEXT("diagonal stays a pond"), RiverTestSample(*Map, Diagonal), ESolidBiome::River);
	RiverTestBiome(*this, TEXT("diagonal does not join"), RiverTestSample(*Map, Diagonal + FVector2D(Bank, 0.f)), ESolidBiome::Grassland);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRiverShippedMapTest,
	"SolidCore1.River.ShippedMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRiverShippedMapTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("map"), Map);
	TestTrue(TEXT("default map loads"), Map->LoadDefault());

	int32 RiverCells = 0;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (Map->GetMarkerAtCell(Col, Row) != USolidWorldChart::RiverMarker)
			{
				continue;
			}
			++RiverCells;
			RiverTestBiome(
				*this,
				*FString::Printf(TEXT("river cell (%d,%d) marker"), Col, Row),
				Map->GetBiomeAtCell(Col, Row),
				ESolidBiome::River);
		}
	}

	TestTrue(TEXT("shipped map has a river"), RiverCells > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
