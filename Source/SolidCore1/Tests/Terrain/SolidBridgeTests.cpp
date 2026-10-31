#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBridge.h"
#include "SolidRiver.h"
#include "SolidRoad.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D BridgeTestMin(-51200.f, -51200.f);
	const FVector2D BridgeTestMax(51200.f, 51200.f);

	struct FBridgeTestMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = USolidWorldChart::GrassMarker;
	};

	FString BridgeTestGrid(const TArray<FBridgeTestMark>& Marks, const FString& Roads)
	{
		FString Text;
		Text.Reserve((USolidWorldChart::MapWidth + 1) * USolidWorldChart::MapHeight + Roads.Len());
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldChart::GrassMarker;
				for (const FBridgeTestMark& Mark : Marks)
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
		Text += Roads;
		return Text;
	}

	FString BridgeTestNamed(
		const int32 FromCol,
		const int32 FromRow,
		const TCHAR* FromTown,
		const int32 ToCol,
		const int32 ToRow,
		const TCHAR* ToTown,
		const TCHAR* Type)
	{
		return FString::Printf(
			TEXT("! %d,%d = Town %s\n! %d,%d = Town %s\nRoad: %s -> %s, %s\n"),
			FromRow + 1, FromCol + 1, FromTown,
			ToRow + 1, ToCol + 1, ToTown,
			FromTown, ToTown, Type);
	}

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeStraightTest,
	"SolidCore1.Bridge.Straight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeStraightTest::RunTest(const FString& Parameters)
{
	TArray<FBridgeTestMark> Marks;
	Marks.Add({ 4, 16, USolidWorldChart::LocationMarker });
	Marks.Add({ 12, 16, USolidWorldChart::RiverMarker });
	Marks.Add({ 20, 16, USolidWorldChart::LocationMarker });

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("map"), Map);
	if (!TestTrue(TEXT("map loads"), Map->LoadFromString(BridgeTestGrid(Marks, BridgeTestNamed(4, 16, TEXT("Iglin"), 20, 16, TEXT("Relion"), TEXT("track"))))))
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
	// The points on the 4 m rim are river. A sample stays river until the next point.
	TestTrue(TEXT("center covers the river and rests on both banks"),
		FMath::IsNearlyEqual(
			BridgePlanLength(Bridge.Deck),
			SolidRiver::WidthCm + 2.f * SolidBridge::BankRestCm + SolidRoad::TerrainQuadCm,
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
	USolidWorldChart* Dry = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("dry map"), Dry);
	if (TestTrue(TEXT("dry map loads"), Dry->LoadFromString(BridgeTestGrid({
		{ 4, 16, USolidWorldChart::LocationMarker },
		{ 20, 16, USolidWorldChart::LocationMarker },
	}, BridgeTestNamed(4, 16, TEXT("Iglin"), 20, 16, TEXT("Relion"), TEXT("track"))))))
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
	TArray<FBridgeTestMark> Marks;
	Marks.Add({ 4, 16, USolidWorldChart::LocationMarker });
	Marks.Add({ 12, 16, USolidWorldChart::RiverMarker });
	Marks.Add({ 20, 16, USolidWorldChart::LocationMarker });

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("map"), Map);
	if (!TestTrue(TEXT("map loads"), Map->LoadFromString(BridgeTestGrid(Marks, BridgeTestNamed(4, 16, TEXT("Iglin"), 20, 16, TEXT("Relion"), TEXT("track"))))))
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
	FSolidBridgeSpanTest,
	"SolidCore1.Bridge.Span",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeSpanTest::RunTest(const FString& Parameters)
{
	const auto Height = [](float, float) { return 100.f; };

	TArray<FBridgeTestMark> ShortMarks;
	ShortMarks.Add({ 2, 20, USolidWorldChart::LocationMarker });
	ShortMarks.Add({ 10, 20, USolidWorldChart::RiverMarker });
	ShortMarks.Add({ 18, 20, USolidWorldChart::LocationMarker });
	USolidWorldChart* ShortMap = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("short map"), ShortMap);
	if (!TestTrue(TEXT("short map loads"), ShortMap->LoadFromString(BridgeTestGrid(ShortMarks, BridgeTestNamed(2, 20, TEXT("Iglin"), 18, 20, TEXT("Relion"), TEXT("trail"))))))
	{
		return false;
	}
	TArray<SolidBridge::FBridge> ShortBridges;
	if (!TestEqual(TEXT("short crossing"), SolidBridge::Collect(*ShortMap, BridgeTestMin, BridgeTestMax, Height, ShortBridges), 1)
		|| ShortBridges.Num() != 1)
	{
		return false;
	}

	TArray<FBridgeTestMark> LongMarks;
	LongMarks.Add({ 2, 20, USolidWorldChart::LocationMarker });
	LongMarks.Add({ 18, 20, USolidWorldChart::LocationMarker });
	constexpr int32 RiverCells = 5;
	for (int32 Col = 8; Col < 8 + RiverCells; ++Col)
	{
		LongMarks.Add({ Col, 20, USolidWorldChart::RiverMarker });
	}
	USolidWorldChart* LongMap = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("long map"), LongMap);
	if (!TestTrue(TEXT("long map loads"), LongMap->LoadFromString(BridgeTestGrid(LongMarks, BridgeTestNamed(2, 20, TEXT("Iglin"), 18, 20, TEXT("Relion"), TEXT("trail"))))))
	{
		return false;
	}
	TArray<SolidBridge::FBridge> LongBridges;
	TestEqual(TEXT("a trail does not follow the river"),
		SolidBridge::Collect(*LongMap, BridgeTestMin, BridgeTestMax, Height, LongBridges), 0);

	TestTrue(TEXT("a trail center is 1 m"), FMath::IsNearlyEqual(ShortBridges[0].WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Trail)));

	TArray<FBridgeTestMark> Blocked;
	Blocked.Add({ 4, 20, USolidWorldChart::LocationMarker });
	Blocked.Add({ 20, 20, USolidWorldChart::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		Blocked.Add({ 12, Row, USolidWorldChart::RiverMarker });
	}
	USolidWorldChart* Wide = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("wide map"), Wide);
	if (!TestTrue(TEXT("wide map loads"), Wide->LoadFromString(BridgeTestGrid(Blocked, BridgeTestNamed(4, 20, TEXT("Iglin"), 20, 20, TEXT("Relion"), TEXT("road"))))))
	{
		return false;
	}
	TArray<SolidBridge::FBridge> WideBridges;
	if (!TestEqual(TEXT("a road that must cross gets one bridge"),
		SolidBridge::Collect(*Wide, BridgeTestMin, BridgeTestMax, Height, WideBridges), 1)
		|| WideBridges.Num() != 1)
	{
		return false;
	}
	TestTrue(TEXT("a road center is 3 m"), FMath::IsNearlyEqual(WideBridges[0].WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Road)));
	TestTrue(TEXT("a wider road widens the center"), WideBridges[0].WidthCm > ShortBridges[0].WidthCm + 100.f);

	TArray<FBridgeTestMark> Gap;
	Gap.Add({ 4, 10, USolidWorldChart::LocationMarker });
	Gap.Add({ 22, 10, USolidWorldChart::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		if (Row != 22)
		{
			Gap.Add({ 12, Row, USolidWorldChart::RiverMarker });
		}
	}
	USolidWorldChart* Around = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("around map"), Around);
	if (TestTrue(TEXT("around map loads"), Around->LoadFromString(BridgeTestGrid(Gap, BridgeTestNamed(4, 10, TEXT("Iglin"), 22, 10, TEXT("Relion"), TEXT("road"))))))
	{
		TArray<SolidBridge::FBridge> AroundBridges;
		TestEqual(TEXT("a road crosses the river and takes one bridge"),
			SolidBridge::Collect(*Around, BridgeTestMin, BridgeTestMax, Height, AroundBridges), 1);
	}
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

	auto LoadCrossing = [this, Height](const TCHAR* FromKind, const TCHAR* FromName, const TCHAR* ToKind, const TCHAR* ToName, const TCHAR* Type, TArray<SolidBridge::FBridge>& Out) -> bool
	{
		TArray<FBridgeTestMark> Marks;
		Marks.Add({ 4, 16, USolidWorldChart::LocationMarker });
		Marks.Add({ 12, 16, USolidWorldChart::RiverMarker });
		Marks.Add({ 20, 16, USolidWorldChart::LocationMarker });
		const FString Lines = FString::Printf(
			TEXT("! 17,5 = %s (%s)\n! 17,21 = %s (%s)\nRoad: %s -> %s, %s\n"),
			FromKind, FromName, ToKind, ToName, FromName, ToName, Type);
		USolidWorldChart* Map = NewObject<USolidWorldChart>();
		TestNotNull(TEXT("map"), Map);
		if (!TestTrue(TEXT("map loads"), Map && Map->LoadFromString(BridgeTestGrid(Marks, Lines))))
		{
			return false;
		}
		return TestEqual(TEXT("one crossing"), SolidBridge::Collect(*Map, BridgeTestMin, BridgeTestMax, Height, Out), 1)
			&& Out.Num() == 1;
	};

	TArray<SolidBridge::FBridge> Iglin;
	if (LoadCrossing(TEXT("Town"), TEXT("Iglin"), TEXT("Town"), TEXT("Relion"), TEXT("track"), Iglin))
	{
		TestTrue(TEXT("Iglin to Relion is broken"), Iglin[0].bBroken);
		TestTrue(TEXT("Iglin to Relion is the Iglin bridge"), Iglin[0].bIglinRelion);
		TestTrue(TEXT("the missing span is still measured"), Iglin[0].Deck.Num() >= 2);
	}

	TArray<SolidBridge::FBridge> Reverse;
	if (LoadCrossing(TEXT("Town"), TEXT("Relion"), TEXT("Town"), TEXT("Iglin"), TEXT("track"), Reverse))
	{
		TestTrue(TEXT("Relion to Iglin is broken"), Reverse[0].bBroken);
		TestTrue(TEXT("Relion to Iglin is the Iglin bridge"), Reverse[0].bIglinRelion);
	}

	TArray<SolidBridge::FBridge> Other;
	if (LoadCrossing(TEXT("Town"), TEXT("Kanfold"), TEXT("Town"), TEXT("Visolar"), TEXT("track"), Other))
	{
		TestFalse(TEXT("another route keeps its span"), Other[0].bBroken);
		TestFalse(TEXT("another route is not the Iglin bridge"), Other[0].bIglinRelion);
	}

	TArray<SolidBridge::FBridge> Cave;
	if (LoadCrossing(TEXT("Stronghold"), TEXT("MeagreDungeon"), TEXT("Town"), TEXT("Iglin"), TEXT("trail"), Cave))
	{
		TestFalse(TEXT("the cave road is not the Iglin bridge"), Cave[0].bBroken);
		TestFalse(TEXT("the cave road is not marked as the Iglin bridge"), Cave[0].bIglinRelion);
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

#endif
