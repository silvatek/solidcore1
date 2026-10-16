#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidGroundMove.h"
#include "SolidBridge.h"
#include "SolidWorldMap.h"

#include <initializer_list>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D GroundMoveMin(-51200.f, -51200.f);
	const FVector2D GroundMoveMax(51200.f, 51200.f);

	struct FGroundMoveMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = USolidWorldMap::GrassMarker;
	};

	FString GroundMoveGrid(std::initializer_list<FGroundMoveMark> Marks)
	{
		FString Text;
		Text.Reserve((USolidWorldMap::MapWidth + 1) * USolidWorldMap::MapHeight);
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldMap::GrassMarker;
				for (const FGroundMoveMark& Mark : Marks)
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

	FVector2D GroundMoveCenter(const USolidWorldMap& Map, const int32 Col, const int32 Row)
	{
		FVector2D Center = FVector2D::ZeroVector;
		Map.CellToWorldXY(FIntPoint(Col, Row), GroundMoveMin, GroundMoveMax, Center);
		return Center;
	}

	bool GroundMoveAllows(
		const USolidWorldMap& Map,
		const FVector2D& Point,
		const TArray<SolidBridge::FBridge>& Bridges)
	{
		return SolidGroundMove::AllowsFooting(
			Map, Point.X, Point.Y, GroundMoveMin, GroundMoveMax, Bridges);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGroundMoveFootingTest,
	"SolidCore1.GroundMove.Footing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGroundMoveFootingTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	if (!TestNotNull(TEXT("map"), Map))
	{
		return false;
	}
	const FString Grid = GroundMoveGrid({
		{ 10, 9, USolidWorldMap::RiverMarker },
		{ 10, 10, USolidWorldMap::RiverMarker },
		{ 10, 11, USolidWorldMap::RiverMarker },
		{ 12, 10, USolidWorldMap::RiverMarker },
		{ 20, 10, USolidWorldMap::SeaMarker },
		{ 22, 10, TEXT('M') },
		{ 24, 10, USolidWorldMap::ForestMarker },
		{ 26, 10, TEXT('C') },
	});
	if (!TestTrue(TEXT("grid loads"), Map->LoadFromString(Grid)))
	{
		return false;
	}

	const TArray<SolidBridge::FBridge> NoBridge;
	const FVector2D Channel = GroundMoveCenter(*Map, 10, 10);
	const FVector2D Pond = GroundMoveCenter(*Map, 12, 10);
	const FVector2D Sea = GroundMoveCenter(*Map, 20, 10);
	const FVector2D Mountain = GroundMoveCenter(*Map, 22, 10);
	const FVector2D Forest = GroundMoveCenter(*Map, 24, 10);
	const FVector2D Cave = GroundMoveCenter(*Map, 26, 10);
	const FVector2D Grass = GroundMoveCenter(*Map, 30, 10);

	TestFalse(TEXT("grass is open"), SolidGroundMove::IsImpassable(ESolidBiome::Grassland));
	TestFalse(TEXT("forest is open"), SolidGroundMove::IsImpassable(ESolidBiome::Forest));
	TestTrue(TEXT("sea is closed"), SolidGroundMove::IsImpassable(ESolidBiome::Sea));
	TestTrue(TEXT("mountain is closed"), SolidGroundMove::IsImpassable(ESolidBiome::Mountain));
	TestTrue(TEXT("river is closed"), SolidGroundMove::IsImpassable(ESolidBiome::River));

	TestTrue(TEXT("grass cell"), GroundMoveAllows(*Map, Grass, NoBridge));
	TestTrue(TEXT("forest cell"), GroundMoveAllows(*Map, Forest, NoBridge));
	TestFalse(TEXT("sea cell"), GroundMoveAllows(*Map, Sea, NoBridge));
	TestFalse(TEXT("mountain cell"), GroundMoveAllows(*Map, Mountain, NoBridge));
	TestFalse(TEXT("cave cell uses the mountain"), GroundMoveAllows(*Map, Cave, NoBridge));

	TestFalse(TEXT("channel centre"), GroundMoveAllows(*Map, Channel, NoBridge));
	TestFalse(TEXT("inside the 4 m channel"), GroundMoveAllows(*Map, Channel + FVector2D(100.f, 0.f), NoBridge));
	TestTrue(
		TEXT("same river cell outside the channel"),
		GroundMoveAllows(*Map, Channel + FVector2D(250.f, 0.f), NoBridge));
	TestTrue(
		TEXT("river cell corner is bank"),
		GroundMoveAllows(*Map, Channel + FVector2D(350.f, 700.f), NoBridge));

	TestFalse(TEXT("pond centre"), GroundMoveAllows(*Map, Pond, NoBridge));
	TestTrue(TEXT("pond cell outside the pond"), GroundMoveAllows(*Map, Pond + FVector2D(250.f, 0.f), NoBridge));

	SolidBridge::FBridge Span;
	Span.WidthCm = 200.f;
	Span.Deck = {
		FVector(Channel.X - 300.f, Channel.Y, 0.f),
		FVector(Channel.X + 300.f, Channel.Y, 0.f),
	};
	Span.RampFrom = {
		Span.Deck[0],
		FVector(Channel.X - 800.f, Channel.Y, 0.f),
	};
	Span.RampTo = {
		Span.Deck.Last(),
		FVector(Channel.X + 800.f, Channel.Y, 0.f),
	};
	const TArray<SolidBridge::FBridge> Bridges = { Span };

	TestTrue(TEXT("bridge centre crosses the channel"), GroundMoveAllows(*Map, Channel, Bridges));
	TestTrue(
		TEXT("on the bridge deck"),
		GroundMoveAllows(*Map, Channel + FVector2D(0.f, 50.f), Bridges));
	TestFalse(
		TEXT("channel beside the bridge"),
		GroundMoveAllows(*Map, Channel + FVector2D(0.f, 150.f), Bridges));
	TestFalse(TEXT("a bridge does not open the sea"), GroundMoveAllows(*Map, Sea, Bridges));
	TestFalse(TEXT("a bridge does not open a mountain"), GroundMoveAllows(*Map, Mountain, Bridges));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGroundMoveClampTest,
	"SolidCore1.GroundMove.Clamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGroundMoveClampTest::RunTest(const FString& Parameters)
{
	auto Shore = [](const float X, const float Y)
	{
		return X <= 0.f;
	};

	const FVector2D Into = SolidGroundMove::ClampPlanar(FVector2D(-30.f, 0.f), FVector2D(100.f, 0.f), 40.f, Shore);
	TestTrue(TEXT("a step into closed ground stops"), Into.IsNearlyZero());

	const FVector2D Along = SolidGroundMove::ClampPlanar(FVector2D(-30.f, 0.f), FVector2D(0.f, 80.f), 40.f, Shore);
	TestTrue(TEXT("a step along the edge continues"), Along.Equals(FVector2D(0.f, 80.f), 0.1f));

	const FVector2D Slide = SolidGroundMove::ClampPlanar(FVector2D(-30.f, 10.f), FVector2D(100.f, 50.f), 40.f, Shore);
	TestTrue(TEXT("a diagonal keeps the free axis"), Slide.Equals(FVector2D(0.f, 50.f), 0.1f));

	const FVector2D Open = SolidGroundMove::ClampPlanar(FVector2D(-200.f, 0.f), FVector2D(20.f, 0.f), 40.f, Shore);
	TestTrue(TEXT("an open step is unchanged"), Open.Equals(FVector2D(20.f, 0.f), 0.1f));

	const FVector2D Leave = SolidGroundMove::ClampPlanar(FVector2D(40.f, 0.f), FVector2D(-10.f, 5.f), 40.f, Shore);
	TestTrue(TEXT("closed ground keeps only the step back out"), Leave.Equals(FVector2D(-10.f, 0.f), 0.1f));

	const FVector2D Deeper = SolidGroundMove::ClampPlanar(FVector2D(40.f, 0.f), FVector2D(20.f, 0.f), 40.f, Shore);
	TestTrue(TEXT("closed ground refuses a deeper step"), Deeper.IsNearlyZero());

	auto Corner = [](const float X, const float Y)
	{
		return !(X > 0.f && Y > 0.f);
	};
	const FVector2D PastCorner = SolidGroundMove::ClampPlanar(
		FVector2D(-20.f, -20.f), FVector2D(100.f, 80.f), 40.f, Corner);
	TestTrue(TEXT("a corner keeps one axis"), PastCorner.Equals(FVector2D(100.f, 0.f), 0.1f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGroundMoveBrokenTest,
	"SolidCore1.GroundMove.Broken",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGroundMoveBrokenTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	if (!TestNotNull(TEXT("map"), Map))
	{
		return false;
	}
	const FString Grid = GroundMoveGrid({
		{ 10, 9, USolidWorldMap::RiverMarker },
		{ 10, 10, USolidWorldMap::RiverMarker },
		{ 10, 11, USolidWorldMap::RiverMarker },
	});
	if (!TestTrue(TEXT("grid loads"), Map->LoadFromString(Grid)))
	{
		return false;
	}

	const FVector2D Channel = GroundMoveCenter(*Map, 10, 10);
	SolidBridge::FBridge Span;
	Span.WidthCm = 200.f;
	Span.Deck = {
		FVector(Channel.X - 300.f, Channel.Y, 0.f),
		FVector(Channel.X + 300.f, Channel.Y, 0.f),
	};
	// The near ramp reaches into the channel. The far ramp stays on the bank.
	Span.RampFrom = {
		FVector(Channel.X - 800.f, Channel.Y, 0.f),
		FVector(Channel.X - 150.f, Channel.Y, 0.f),
	};
	Span.RampTo = {
		Span.Deck.Last(),
		FVector(Channel.X + 800.f, Channel.Y, 0.f),
	};

	const TArray<SolidBridge::FBridge> Intact = { Span };
	TestTrue(TEXT("an intact center crosses the channel"), GroundMoveAllows(*Map, Channel, Intact));

	Span.bBroken = true;
	const TArray<SolidBridge::FBridge> Broken = { Span };
	const TArray<SolidBridge::FBridge> None;
	const FVector2D Lip(Channel.X - 150.f, Channel.Y);
	TestFalse(TEXT("a broken center leaves the channel closed"), GroundMoveAllows(*Map, Channel, Broken));
	TestFalse(TEXT("the ramp lip is in the channel"), GroundMoveAllows(*Map, Lip, None));
	TestTrue(TEXT("a ramp still reaches into the channel"), GroundMoveAllows(*Map, Lip, Broken));
	TestFalse(
		TEXT("the channel beside a broken bridge stays closed"),
		GroundMoveAllows(*Map, Channel + FVector2D(0.f, 150.f), Broken));

	// RampTo[0] is the lip, on the bank. The next point walks away from the gap.
	const FVector2D BankPastLip(Channel.X + 250.f, Channel.Y);
	TestTrue(TEXT("the bank in front of an intact ramp is open"), GroundMoveAllows(*Map, BankPastLip, Intact));
	TestFalse(TEXT("the bank in front of a broken ramp is closed"), GroundMoveAllows(*Map, BankPastLip, Broken));
	TestTrue(TEXT("the broken ramp lip can still be stood on"), GroundMoveAllows(*Map, FVector2D(Span.RampTo[0].X, Span.RampTo[0].Y), Broken));
	TestTrue(
		TEXT("the broken ramp can be walked back down"),
		GroundMoveAllows(*Map, FVector2D(Channel.X + 500.f, Channel.Y), Broken));

	auto Footing = [&Map, &Broken](const float X, const float Y)
	{
		return GroundMoveAllows(*Map, FVector2D(X, Y), Broken);
	};
	const FVector2D TowardGap = SolidGroundMove::ClampPlanar(
		FVector2D(Channel.X + 320.f, Channel.Y),
		FVector2D(-100.f, 0.f),
		42.f,
		Footing);
	TestTrue(TEXT("a step off the broken ramp stops"), TowardGap.IsNearlyZero());
	const FVector2D BackDown = SolidGroundMove::ClampPlanar(
		FVector2D(Channel.X + 320.f, Channel.Y),
		FVector2D(100.f, 0.f),
		42.f,
		Footing);
	TestTrue(TEXT("a step back down the broken ramp continues"), BackDown.Equals(FVector2D(100.f, 0.f), 0.1f));
	return true;
}

#endif
