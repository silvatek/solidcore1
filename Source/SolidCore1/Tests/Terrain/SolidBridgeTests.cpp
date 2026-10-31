#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBridge.h"
#include "SolidBridges.h"
#include "SolidRoad.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
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

	// LoadFromChart needs a full cell grid. The river is one point column, and the towns are marks.
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
		Chart->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
		Chart->Points.Init(ESolidBiome::Grassland, USolidWorldChart::PointAxis * USolidWorldChart::PointAxis);

		FString Error;
		if (!Test.TestTrue(TEXT("Iglin places"), Chart->PlaceTown(Iglin, TEXT("Iglin"), Error))
			|| !Test.TestTrue(TEXT("Relion places"), Chart->PlaceTown(Relion, TEXT("Relion"), Error)))
		{
			return false;
		}

		if (bRiver)
		{
			FVector2D Min = FVector2D::ZeroVector;
			FVector2D Max = FVector2D::ZeroVector;
			USolidWorldChart::DefaultWorldBounds(Min, Max);
			FVector2D Centre = FVector2D::ZeroVector;
			FIntPoint River = FIntPoint::ZeroValue;
			if (!Test.TestTrue(TEXT("the river cell has a centre"), Chart->CellToWorldXY(FIntPoint(12, 16), Min, Max, Centre))
				|| !Test.TestTrue(TEXT("the river sits on a point"), Chart->TerrainPointAt(Centre.X, Centre.Y, Min, Max, River)))
			{
				return false;
			}
			const int32 Last = USolidWorldChart::PointAxis - 1;
			if (!Test.TestTrue(TEXT("the river paints"),
				Chart->SetPointRectangle(FIntPoint(River.X, 0), FIntPoint(River.X, Last), ESolidBiome::River)))
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
		return Test.TestEqual(TEXT("the Iglin track is routed"), Loaded->GetRoutedRoads().Num(), 1);
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
	TestTrue(TEXT("river cell"), Map->CellToWorldXY(FIntPoint(12, 16), BridgeTestMin, BridgeTestMax, River));
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
		TestTrue(TEXT("Iglin west of the river is broken"), Iglin[0].bBroken);
		TestTrue(TEXT("Iglin west of the river is the Iglin bridge"), Iglin[0].bIglinRelion);
		TestTrue(TEXT("the missing span is still measured"), Iglin[0].Deck.Num() >= 2);
	}

	USolidWorldChart* EastIglin = nullptr;
	TArray<SolidBridge::FBridge> Swapped;
	if (BridgeLoadTrack(*this, EastIglin, true, FIntPoint(20, 16), FIntPoint(4, 16))
		&& EastIglin && OneBridge(*EastIglin, Swapped))
	{
		TestTrue(TEXT("Iglin east of the river is broken"), Swapped[0].bBroken);
		TestTrue(TEXT("Iglin east of the river is the Iglin bridge"), Swapped[0].bIglinRelion);
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
	const USolidWorldChart* Shipped = LoadObject<USolidWorldChart>(nullptr, USolidWorldChart::ShippedPath());
	TestNotNull(TEXT("shipped chart"), Shipped);
	if (!Shipped)
	{
		return false;
	}

	USolidWorldChart* Loaded = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("loaded chart"), Loaded);
	if (!Loaded || !TestTrue(TEXT("the chart loads"), Loaded->LoadFromChart(Shipped)))
	{
		return false;
	}
	TestTrue(TEXT("the chart has a routed road"), Loaded->GetRoutedRoads().Num() > 0);

	FVector2D WorldMin = FVector2D::ZeroVector;
	FVector2D WorldMax = FVector2D::ZeroVector;
	USolidWorldChart::DefaultWorldBounds(WorldMin, WorldMax);
	const auto Height = [](float, float) { return 100.f; };
	TArray<SolidBridge::FBridge> Bridges;
	TestTrue(TEXT("a routed chart has a bridge"),
		SolidBridge::Collect(*Loaded, WorldMin, WorldMax, Height, Bridges) > 0);
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

#endif
