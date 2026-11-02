#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBridge.h"
#include "SolidBridges.h"
#include "SolidRiver.h"
#include "SolidRoad.h"
#include "SolidTerrainMap.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FIntPoint BridgeCellPoint(const FIntPoint Cell)
	{
		int32 X = 0;
		int32 Y = 0;
		USolidWorldChart::PointUnderCell(Cell.X, Cell.Y, X, Y);
		return FIntPoint(X, Y);
	}

	const FVector2D BridgeTestMin(-51200.f, -51200.f);
	const FVector2D BridgeTestMax(51200.f, 51200.f);

	float BridgePlanLength(const TArray<FVector>& Line)
	{
		float Length = 0.f;
		for (int32 Index = 1; Index < Line.Num(); ++Index)
		{
			Length += FVector2D::Distance(
				FVector2D(Line[Index - 1].X, Line[Index - 1].Y),
				FVector2D(Line[Index].X, Line[Index].Y));
		}
		return Length;
	}

	bool BridgeFlat(const TArray<FVector>& Line)
	{
		if (Line.Num() == 0)
		{
			return false;
		}
		for (const FVector& Point : Line)
		{
			if (!FMath::IsNearlyEqual(Point.Z, Line[0].Z, 0.1f))
			{
				return false;
			}
		}
		return true;
	}

	bool BridgeOnOneAxis(const SolidBridge::FBridge& Bridge)
	{
		TArray<FVector> Points;
		Points.Append(Bridge.Deck);
		Points.Append(Bridge.RampFrom);
		Points.Append(Bridge.RampTo);
		if (Points.Num() < 2)
		{
			return false;
		}
		float MinX = Points[0].X;
		float MaxX = MinX;
		float MinY = Points[0].Y;
		float MaxY = MinY;
		for (const FVector& Point : Points)
		{
			MinX = FMath::Min(MinX, Point.X);
			MaxX = FMath::Max(MaxX, Point.X);
			MinY = FMath::Min(MinY, Point.Y);
			MaxY = FMath::Max(MaxY, Point.Y);
		}
		const bool bEastWest = (MaxX - MinX) >= (MaxY - MinY);
		const float Lock = bEastWest ? Points[0].Y : Points[0].X;
		const float Span = bEastWest ? (MaxX - MinX) : (MaxY - MinY);
		if (Span <= 1.f)
		{
			return false;
		}
		for (const FVector& Point : Points)
		{
			const float Value = bEastWest ? Point.Y : Point.X;
			if (!FMath::IsNearlyEqual(Value, Lock, 1.f))
			{
				return false;
			}
		}
		return true;
	}

	bool BridgeBuried(const TArray<FVector>& Ramp, const TFunction<float(float, float)>& HeightAt)
	{
		if (Ramp.Num() < 2)
		{
			return false;
		}
		const FVector& End = Ramp.Last();
		const float Ground = HeightAt(End.X, End.Y);
		const float Slack = SolidBridge::Grade * SolidBridge::SampleStepCm + 1.f;
		return End.Z < Ground - SolidBridge::Grade * SolidBridge::BurialCm + Slack
			&& End.Z < Ramp[0].Z;
	}

	// The load needs a full point grid. The river is one point column, and the towns are marks.
	bool BridgeLoadTrack(
		FAutomationTestBase& Test,
		USolidWorldChart*& OutMap,
		const bool bRiver,
		const FIntPoint Iglin = FIntPoint(4, 16),
		const FIntPoint Relion = FIntPoint(20, 16))
	{
		USolidWorldChart* Chart = NewObject<USolidWorldChart>();
		if (!Test.TestNotNull(TEXT("chart"), Chart))
		{
			return false;
		}
		Chart->Init(ESolidBiome::Grassland);
		Chart->Points.Init(ESolidBiome::Grassland, USolidWorldChart::PointAxis * USolidWorldChart::PointAxis);

		FString Error;
		if (!Test.TestTrue(TEXT("Iglin places"), Chart->PlaceTownAt(BridgeCellPoint(Iglin), TEXT("Iglin"), Error))
			|| !Test.TestTrue(TEXT("Relion places"), Chart->PlaceTownAt(BridgeCellPoint(Relion), TEXT("Relion"), Error))
			|| !Test.TestTrue(TEXT("Iglin Bridge places"), Chart->PlaceBridgeAt(BridgeCellPoint(FIntPoint(11, 16)), TEXT("Iglin Bridge"), Error)))
		{
			return false;
		}

		if (bRiver)
		{
			int32 RiverTerrainX = 0;
			int32 RiverTerrainY = 0;
			USolidWorldChart::PointUnderCell(12, 16, RiverTerrainX, RiverTerrainY);
			const FIntPoint RiverTerrainPoint(RiverTerrainX, RiverTerrainY);
			if (!Test.TestTrue(TEXT("the river sits on a point"), RiverTerrainPoint.X >= 0))
			{
				return false;
			}
			const int32 Last = USolidWorldChart::PointAxis - 1;
			if (!Test.TestTrue(TEXT("the river paints"),
				Chart->SetPointRectangle(FIntPoint(RiverTerrainPoint.X, 0), FIntPoint(RiverTerrainPoint.X, Last), ESolidBiome::River)))
			{
				return false;
			}
		}

		USolidWorldChart* Loaded = NewObject<USolidWorldChart>();
		if (!Test.TestNotNull(TEXT("loaded chart"), Loaded))
		{
			return false;
		}
		if (!Test.TestTrue(TEXT("the chart loads"), Loaded->LoadFromChart(Chart)))
		{
			return false;
		}
		OutMap = Loaded;
		return Test.TestEqual(TEXT("both halves of the Iglin track are routed"), Loaded->GetRoutedRoads().Num(), 2);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeStraightTest,
	"SolidCore1.Bridge.Straight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeStraightTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = nullptr;
	if (!BridgeLoadTrack(*this, Map, true))
	{
		return false;
	}

	const auto Height = [](float, float) { return 100.f; };
	TArray<SolidBridge::FBridge> Bridges;
	TestEqual(TEXT("one bridge"), SolidBridge::Collect(*Map, BridgeTestMin, BridgeTestMax, Height, Bridges), 1);
	if (!TestTrue(TEXT("a bridge"), Bridges.Num() == 1))
	{
		return false;
	}

	const SolidBridge::FBridge& Bridge = Bridges[0];
	TestTrue(TEXT("center is as wide as the track"), FMath::IsNearlyEqual(Bridge.WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Track)));
	TestTrue(TEXT("center is flat"), BridgeFlat(Bridge.Deck));
	// One point column is the channel. A sample stays river until the next point.
	TestTrue(TEXT("center covers the river and rests on both banks"),
		FMath::IsNearlyEqual(
			BridgePlanLength(Bridge.Deck),
			SolidRoad::TerrainQuadCm + 2.f * SolidBridge::BankRestCm,
			2.f));
	TestTrue(TEXT("three pieces"), Bridge.Deck.Num() >= 2 && Bridge.RampFrom.Num() >= 2 && Bridge.RampTo.Num() >= 2);
	TestTrue(TEXT("the near ramp leaves the center"), Bridge.RampFrom[0].Equals(Bridge.Deck[0], 1.f));
	TestTrue(TEXT("the far ramp leaves the center"), Bridge.RampTo[0].Equals(Bridge.Deck.Last(), 1.f));
	TestTrue(TEXT("the near ramp runs into the ground"), BridgeBuried(Bridge.RampFrom, Height));
	TestTrue(TEXT("the far ramp runs into the ground"), BridgeBuried(Bridge.RampTo, Height));

	const float Lift = FMath::Max(SolidBridge::RiseCm, SolidBridge::MinClearanceCm + SolidBridge::ThicknessCm);
	TestTrue(TEXT("the center stands above the ground"),
		FMath::IsNearlyEqual(Bridge.Deck[0].Z, 100.f + Lift, 0.1f));
	TestTrue(TEXT("ramps on level banks are the same length"),
		FMath::Abs(BridgePlanLength(Bridge.RampFrom) - BridgePlanLength(Bridge.RampTo)) < SolidBridge::SampleStepCm * 2.f);

	TArray<SolidBridge::FBridge> None;
	USolidWorldChart* Dry = nullptr;
	if (BridgeLoadTrack(*this, Dry, false))
	{
		TestEqual(TEXT("no river means no bridge"), SolidBridge::Collect(*Dry, BridgeTestMin, BridgeTestMax, Height, None), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeBanksTest,
	"SolidCore1.Bridge.Banks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeBanksTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = nullptr;
	if (!BridgeLoadTrack(*this, Map, true))
	{
		return false;
	}

	FVector2D River = FVector2D::ZeroVector;
	TestTrue(TEXT("river cell"), Map->TerrainPointToWorld(BridgeCellPoint(FIntPoint(12, 16)), BridgeTestMin, BridgeTestMax, River));
	const auto Height = [River](float X, float)
	{
		return X > River.X ? 400.f : 100.f;
	};

	TArray<SolidBridge::FBridge> Bridges;
	if (!TestEqual(TEXT("one bridge"), SolidBridge::Collect(*Map, BridgeTestMin, BridgeTestMax, Height, Bridges), 1)
		|| Bridges.Num() != 1)
	{
		return false;
	}

	const SolidBridge::FBridge& Bridge = Bridges[0];
	TestTrue(TEXT("center clears the higher bank"), Bridge.Deck[0].Z >= 400.f + SolidBridge::RiseCm - 0.1f);
	TestTrue(TEXT("center stays flat over uneven banks"), BridgeFlat(Bridge.Deck));
	TestTrue(TEXT("the high bank gets the shorter ramp"),
		BridgePlanLength(Bridge.RampFrom) * 2.f < BridgePlanLength(Bridge.RampTo));
	TestTrue(TEXT("high ramp still enters the ground"), BridgeBuried(Bridge.RampFrom, Height));
	TestTrue(TEXT("low ramp reaches the lower bank"), BridgeBuried(Bridge.RampTo, Height));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeAlignedTest,
	"SolidCore1.Bridge.Aligned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeAlignedTest::RunTest(const FString& Parameters)
{
	const FVector2D Paint = SolidRoad::StripPaintOffset();
	TestTrue(TEXT("paint offset is half a quad southwest"),
		FMath::IsNearlyEqual(Paint.X, -0.5f * SolidRoad::TerrainQuadCm)
		&& FMath::IsNearlyEqual(Paint.Y, -0.5f * SolidRoad::TerrainQuadCm));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeBrokenTest,
	"SolidCore1.Bridge.Broken",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeBrokenTest::RunTest(const FString& Parameters)
{
	const auto Height = [](float, float) { return 100.f; };

	auto OneBridge = [this, Height](const USolidWorldChart& Map, TArray<SolidBridge::FBridge>& Out) -> bool
	{
		return TestEqual(TEXT("one crossing"), SolidBridge::Collect(Map, BridgeTestMin, BridgeTestMax, Height, Out), 1)
			&& Out.Num() == 1;
	};

	USolidWorldChart* WestIglin = nullptr;
	TArray<SolidBridge::FBridge> Iglin;
	if (BridgeLoadTrack(*this, WestIglin, true) && WestIglin && OneBridge(*WestIglin, Iglin))
	{
		TestFalse(TEXT("the west half stays intact"), Iglin[0].bBroken);
		TestFalse(TEXT("the west half leaves the Iglin repair flag clear"), Iglin[0].bIglinRelion);
		TestTrue(TEXT("the span is still measured"), Iglin[0].Deck.Num() >= 2);
	}

	USolidWorldChart* EastIglin = nullptr;
	TArray<SolidBridge::FBridge> Swapped;
	if (BridgeLoadTrack(*this, EastIglin, true, FIntPoint(20, 16), FIntPoint(4, 16))
		&& EastIglin && OneBridge(*EastIglin, Swapped))
	{
		TestFalse(TEXT("the east half stays intact"), Swapped[0].bBroken);
		TestFalse(TEXT("the east half leaves the Iglin repair flag clear"), Swapped[0].bIglinRelion);
	}

	ASolidBridge* Actor = NewObject<ASolidBridge>();
	TestNotNull(TEXT("bridge"), Actor);
	if (!Actor)
	{
		return false;
	}

	SolidBridge::FBridge Spec;
	Spec.bBroken = true;
	Spec.bIglinRelion = true;
	Spec.WidthCm = SolidRoad::WidthCm(SolidRoad::EType::Track);
	Spec.Deck = { FVector(0.f, 0.f, 200.f), FVector(600.f, 0.f, 200.f) };
	Spec.RampFrom = { FVector(0.f, 0.f, 200.f), FVector(-400.f, 0.f, 100.f), FVector(-800.f, 0.f, 0.f) };
	Spec.RampTo = { FVector(600.f, 0.f, 200.f), FVector(1000.f, 0.f, 100.f), FVector(1400.f, 0.f, 0.f) };
	Actor->SetBridge(Spec);

	Actor->SetBroken(false);
	TestFalse(TEXT("repair restores the span"), Actor->GetBridge().bBroken);
	TestTrue(TEXT("repair still knows the Iglin bridge"), Actor->GetBridge().bIglinRelion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeAxisTest,
	"SolidCore1.Bridge.Axis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeAxisTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Loaded = nullptr;
	if (!BridgeLoadTrack(*this, Loaded, true) || !Loaded)
	{
		return false;
	}

	const auto Height = [](float, float) { return 100.f; };
	TArray<SolidBridge::FBridge> Bridges;
	TestTrue(TEXT("a road that still crosses the river has a bridge"),
		SolidBridge::Collect(*Loaded, BridgeTestMin, BridgeTestMax, Height, Bridges) > 0);
	for (const SolidBridge::FBridge& Bridge : Bridges)
	{
		TestTrue(TEXT("a routed bridge is straight on one axis"), BridgeOnOneAxis(Bridge));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeDefinitionsTest,
	"SolidCore1.Bridge.Definitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeDefinitionsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("three bridges"), SolidBridge::DefinitionCount, 3);
	if (SolidBridge::DefinitionCount < 3)
	{
		return false;
	}

	const SolidBridge::FBridgeMarker& Iglin = SolidBridge::Bridges[0];
	TestEqual(TEXT("Iglin Bridge"), FString(Iglin.Name), FString(TEXT("Iglin Bridge")));
	TestEqual(TEXT("Iglin is 18 m"), Iglin.LengthCm, 1800.f);
	TestEqual(
		TEXT("Iglin is east-west"),
		static_cast<uint8>(Iglin.Orientation),
		static_cast<uint8>(SolidBridge::EOrientation::EastWest));
	TestEqual(
		TEXT("Iglin is broken"),
		static_cast<uint8>(Iglin.State),
		static_cast<uint8>(SolidBridge::EInitialState::Broken));

	const SolidBridge::FBridgeMarker& Jethan = SolidBridge::Bridges[1];
	TestEqual(TEXT("Jethan Bridge"), FString(Jethan.Name), FString(TEXT("Jethan Bridge")));
	TestEqual(TEXT("Jethan is 18 m"), Jethan.LengthCm, 1800.f);
	TestEqual(
		TEXT("Jethan is north-south"),
		static_cast<uint8>(Jethan.Orientation),
		static_cast<uint8>(SolidBridge::EOrientation::NorthSouth));
	TestEqual(
		TEXT("Jethan is intact"),
		static_cast<uint8>(Jethan.State),
		static_cast<uint8>(SolidBridge::EInitialState::Intact));

	const SolidBridge::FBridgeMarker& Rathen = SolidBridge::Bridges[2];
	TestEqual(TEXT("Rathen Bridge"), FString(Rathen.Name), FString(TEXT("Rathen Bridge")));
	TestEqual(TEXT("Rathen is 18 m"), Rathen.LengthCm, 1800.f);
	TestEqual(
		TEXT("Rathen is north-south"),
		static_cast<uint8>(Rathen.Orientation),
		static_cast<uint8>(SolidBridge::EOrientation::NorthSouth));
	TestEqual(
		TEXT("Rathen is intact"),
		static_cast<uint8>(Rathen.State),
		static_cast<uint8>(SolidBridge::EInitialState::Intact));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeStandsTest,
	"SolidCore1.Bridge.Stands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeStandsTest::RunTest(const FString& Parameters)
{
	const FVector2D Centre(1000.f, 2000.f);
	const auto Height = [](float, float) { return 100.f; };
	const float DeckLen = SolidRiver::WidthCm + 2.f * SolidBridge::BankRestCm;
	const float Width = SolidRoad::WidthCm(SolidRoad::EType::Track);

	SolidBridge::FBridge Iglin;
	TestTrue(TEXT("Iglin stands"), SolidBridge::Stand(SolidBridge::Bridges[0], Centre, Width, Height, Iglin));
	TestTrue(TEXT("Iglin starts broken"), Iglin.bBroken);
	TestTrue(TEXT("Iglin can be repaired"), Iglin.bIglinRelion);
	TestTrue(TEXT("Iglin is as wide as the track"), FMath::IsNearlyEqual(Iglin.WidthCm, Width));
	TestTrue(TEXT("Iglin center is flat"), BridgeFlat(Iglin.Deck));
	TestTrue(TEXT("Iglin center is the channel plus both rests"),
		FMath::IsNearlyEqual(BridgePlanLength(Iglin.Deck), DeckLen, 1.f));
	TestTrue(TEXT("Iglin is 18 m, ramps included"),
		FMath::IsNearlyEqual(
			BridgePlanLength(Iglin.RampFrom) + BridgePlanLength(Iglin.Deck) + BridgePlanLength(Iglin.RampTo),
			1800.f,
			1.f));
	TestTrue(TEXT("Iglin runs east-west"), BridgeOnOneAxis(Iglin));
	const FVector IglinMid = (Iglin.Deck[0] + Iglin.Deck.Last()) * 0.5f;
	TestTrue(TEXT("Iglin is centred on the marker"),
		FVector2D(IglinMid.X, IglinMid.Y).Equals(Centre, 1.f));
	TestTrue(TEXT("Iglin's span stays on one east-west line"),
		FMath::IsNearlyEqual(Iglin.Deck[0].Y, Iglin.Deck.Last().Y, 1.f)
		&& FMath::Abs(Iglin.Deck[0].X - Iglin.Deck.Last().X) > 100.f);
	TestTrue(TEXT("the near ramp runs into the ground"), BridgeBuried(Iglin.RampFrom, Height));
	TestTrue(TEXT("the far ramp runs into the ground"), BridgeBuried(Iglin.RampTo, Height));

	const auto Bank = [Centre](float X, float)
	{
		return X > Centre.X ? 400.f : 100.f;
	};
	SolidBridge::FBridge HighBank;
	TestTrue(TEXT("Iglin stands on a high bank"), SolidBridge::Stand(SolidBridge::Bridges[0], Centre, Width, Bank, HighBank));
	TestTrue(TEXT("the center clears the higher bank"), HighBank.Deck[0].Z >= 400.f + SolidBridge::RiseCm - 0.1f);
	TestTrue(TEXT("the high bank gets the shorter ramp"),
		BridgePlanLength(HighBank.RampTo) * 2.f < BridgePlanLength(HighBank.RampFrom));
	TestTrue(TEXT("the high ramp enters the ground"), BridgeBuried(HighBank.RampTo, Bank));
	TestTrue(TEXT("the low ramp enters the ground"), BridgeBuried(HighBank.RampFrom, Bank));

	SolidBridge::FBridge Jethan;
	TestTrue(TEXT("Jethan stands"), SolidBridge::Stand(SolidBridge::Bridges[1], Centre, Width, Height, Jethan));
	TestFalse(TEXT("Jethan starts intact"), Jethan.bBroken);
	TestFalse(TEXT("Jethan is not the Iglin repair"), Jethan.bIglinRelion);
	TestTrue(TEXT("Jethan runs north-south"),
		FMath::IsNearlyEqual(Jethan.Deck[0].X, Jethan.Deck.Last().X, 1.f)
		&& FMath::Abs(Jethan.Deck[0].Y - Jethan.Deck.Last().Y) > 100.f);

	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("chart"), Chart);
	Chart->Init(ESolidBiome::Grassland);
	TArray<SolidBridge::FBridge> None;
	const USolidTerrainMap* Defaults = GetDefault<USolidTerrainMap>();
	const FVector2D WorldMin = Defaults->GetWorldMinXY();
	const FVector2D WorldMax = Defaults->GetWorldMaxXY();
	TestEqual(TEXT("a chart with no bridge mark stands nothing"),
		SolidBridge::FromMarkers(*Chart, Height, None, WorldMin, WorldMax), 0);

	FString Error;
	const FIntPoint BridgePoint = BridgeCellPoint(FIntPoint(11, 16));
	TestTrue(TEXT("Iglin Bridge places"), Chart->PlaceBridgeAt(BridgePoint, TEXT("Iglin Bridge"), Error));

	int32 RiverTerrainX = 0;
	int32 RiverTerrainY = 0;
	USolidWorldChart::PointUnderCell(12, 16, RiverTerrainX, RiverTerrainY);
	const FIntPoint RiverTerrainPoint(RiverTerrainX, RiverTerrainY);
	const int32 LastPoint = USolidWorldChart::PointAxis - 1;
	if (TestTrue(TEXT("the river sits on a point"), RiverTerrainPoint.X >= 0))
	{
		TestTrue(TEXT("the river paints"),
			Chart->SetPointRectangle(FIntPoint(RiverTerrainPoint.X, 0), FIntPoint(RiverTerrainPoint.X, LastPoint), ESolidBiome::River));
	}
	int32 MarkIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Chart->Marks.Num(); ++Index)
	{
		if (Chart->Marks[Index].TerrainPoint == BridgePoint)
		{
			MarkIndex = Index;
			break;
		}
	}
	if (TestTrue(TEXT("the mark is stored"), MarkIndex != INDEX_NONE))
	{
		SolidBridge::FBridge OverWater;
		const FIntPoint MarkPoint = Chart->Marks[MarkIndex].TerrainPoint;
		const float Spacing = (WorldMax.X - WorldMin.X) / static_cast<float>(USolidWorldChart::PointAxis - 1);
		const FVector2D MarkCentre(
			WorldMin.X + static_cast<float>(MarkPoint.X) * Spacing,
			WorldMin.Y + static_cast<float>(MarkPoint.Y) * Spacing);
		if (TestTrue(TEXT("Iglin stands over the river"),
			SolidBridge::Stand(
				SolidBridge::Bridges[0],
				MarkCentre,
				Width,
				Height,
				OverWater,
				Chart,
				WorldMin,
				WorldMax)))
		{
			const FVector DeckMid = (OverWater.Deck[0] + OverWater.Deck.Last()) * 0.5f;
			TestTrue(TEXT("the centre span is the marker"),
				FVector2D(DeckMid.X, DeckMid.Y).Equals(MarkCentre, 1.f));
			TestTrue(TEXT("the span stays east-west beside the river"),
				FMath::IsNearlyEqual(OverWater.Deck[0].Y, MarkCentre.Y, 1.f)
				&& FMath::Abs(OverWater.Deck[0].X - OverWater.Deck.Last().X) > 100.f);
			TestTrue(TEXT("the ramps leave that span"),
				OverWater.RampFrom[0].Equals(OverWater.Deck[0], 1.f)
				&& OverWater.RampTo[0].Equals(OverWater.Deck.Last(), 1.f));
			TestTrue(TEXT("Iglin starts with the centre removed"), OverWater.bBroken);
		}
	}

	TArray<SolidBridge::FBridge> Marked;
	TestEqual(TEXT("the placed mark stands"),
		SolidBridge::FromMarkers(*Chart, Height, Marked, WorldMin, WorldMax), 1);
	if (Marked.Num() == 1)
	{
		TestTrue(TEXT("the placed Iglin bridge starts broken"), Marked[0].bBroken);
		TestTrue(TEXT("the placed span is as wide as the Iglin track"),
			FMath::IsNearlyEqual(Marked[0].WidthCm, Width));
	}

	const FVector2D ShiftedMin = WorldMin + FVector2D(-51200.f, -51200.f);
	const FVector2D ShiftedMax = WorldMax + FVector2D(-51200.f, -51200.f);
	TArray<SolidBridge::FBridge> Shifted;
	TestEqual(TEXT("the shifted terrain still stands the mark"),
		SolidBridge::FromMarkers(*Chart, Height, Shifted, ShiftedMin, ShiftedMax), 1);
	if (Shifted.Num() == 1 && MarkIndex != INDEX_NONE)
	{
		FVector2D Ground = FVector2D::ZeroVector;
		TestTrue(TEXT("the mark projects to the shifted terrain"),
			Chart->TerrainPointToWorld(Chart->Marks[MarkIndex].TerrainPoint, ShiftedMin, ShiftedMax, Ground));
		const FVector ShiftedMid = (Shifted[0].Deck[0] + Shifted[0].Deck.Last()) * 0.5f;
		TestTrue(TEXT("the centre span is the marker on the terrain"),
			FVector2D(ShiftedMid.X, ShiftedMid.Y).Equals(Ground, 1.f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeBuriedRampTest,
	"SolidCore1.Bridge.BuriedRamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeBuriedRampTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("chart"), Chart);
	Chart->Init(ESolidBiome::Grassland);

	FString Error;
	const FIntPoint Cell(11, 16);
	if (!TestTrue(TEXT("Iglin Bridge places"), Chart->PlaceBridgeAt(BridgeCellPoint(Cell), TEXT("Iglin Bridge"), Error)))
	{
		return false;
	}

	int32 CentreTerrainX = 0;
	int32 CentreTerrainY = 0;
	USolidWorldChart::PointUnderCell(Cell.X, Cell.Y, CentreTerrainX, CentreTerrainY);
	const USolidTerrainMap* Defaults = GetDefault<USolidTerrainMap>();
	const FVector2D WorldMin = Defaults->GetWorldMinXY();
	const FVector2D WorldMax = Defaults->GetWorldMaxXY();
	const FVector2D Centre(
		WorldMin.X + static_cast<float>(CentreTerrainX) * Defaults->PointSpacing,
		WorldMin.Y + static_cast<float>(CentreTerrainY) * Defaults->PointSpacing);

	const auto Height = [](float, float) { return 100.f; };
	const float Width = SolidRoad::WidthCm(SolidRoad::EType::Track);
	SolidBridge::FBridge Bridge;
	if (!TestTrue(TEXT("Iglin stands"), SolidBridge::Stand(SolidBridge::Bridges[0], Centre, Width, Height, Bridge)))
	{
		return false;
	}

	SolidBridge::PaintBuriedRamp(*Chart, Bridge, Height, WorldMin, WorldMax);

	const float HalfQuad = SolidRoad::TerrainQuadCm * 0.5f;
	bool bSawBuried = false;
	bool bSawOpen = false;
	const TArray<FVector>* Ramps[] = { &Bridge.RampFrom, &Bridge.RampTo };
	for (const TArray<FVector>* Ramp : Ramps)
	{
		for (const FVector& Sample : *Ramp)
		{
			FIntPoint SurfaceTerrainPoint = FIntPoint::ZeroValue;
			Chart->TerrainPointAt(Sample.X, Sample.Y + HalfQuad, WorldMin, WorldMax, SurfaceTerrainPoint);
			const ESolidBiome Painted = Chart->GetPointBiome(SurfaceTerrainPoint);
			if (Sample.Z < 99.f)
			{
				bSawBuried = true;
				TestEqual(
					TEXT("the ground over a buried ramp is road"),
					static_cast<uint8>(Painted),
					static_cast<uint8>(ESolidBiome::Road));
			}
			else if (Sample.Equals((*Ramp)[0], 1.f))
			{
				bSawOpen = true;
				TestEqual(
					TEXT("the ground over the open ramp stays grassland"),
					static_cast<uint8>(Painted),
					static_cast<uint8>(ESolidBiome::Grassland));
			}
		}
	}
	TestTrue(TEXT("a ramp does go under the ground"), bSawBuried);
	TestTrue(TEXT("a ramp also stands above the ground"), bSawOpen);

	TestEqual(
		TEXT("the ramp's own point stays grassland"),
		static_cast<uint8>(Chart->GetPointBiome(FIntPoint(CentreTerrainX, CentreTerrainY))),
		static_cast<uint8>(ESolidBiome::Grassland));
	return true;
}

#endif
