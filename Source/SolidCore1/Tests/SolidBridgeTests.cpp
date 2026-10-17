#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/BodySetup.h"
#include "SolidBridge.h"
#include "SolidRiver.h"
#include "SolidRoad.h"
#include "SolidWorldMap.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D BridgeTestMin(-51200.f, -51200.f);
	const FVector2D BridgeTestMax(51200.f, 51200.f);

	struct FBridgeTestMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = USolidWorldMap::GrassMarker;
	};

	FString BridgeTestGrid(const TArray<FBridgeTestMark>& Marks, const FString& Roads)
	{
		FString Text;
		Text.Reserve((USolidWorldMap::MapWidth + 1) * USolidWorldMap::MapHeight + Roads.Len());
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldMap::GrassMarker;
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
	Marks.Add({ 4, 16, USolidWorldMap::LocationMarker });
	Marks.Add({ 12, 16, USolidWorldMap::RiverMarker });
	Marks.Add({ 20, 16, USolidWorldMap::LocationMarker });

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
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
	TestTrue(TEXT("center covers the river and rests on both banks"),
		FMath::IsNearlyEqual(
			BridgePlanLength(Bridge.Deck),
			SolidRiver::WidthCm + 2.f * SolidBridge::BankRestCm,
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
	USolidWorldMap* Dry = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("dry map"), Dry);
	if (TestTrue(TEXT("dry map loads"), Dry->LoadFromString(BridgeTestGrid({
		{ 4, 16, USolidWorldMap::LocationMarker },
		{ 20, 16, USolidWorldMap::LocationMarker },
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
	Marks.Add({ 4, 16, USolidWorldMap::LocationMarker });
	Marks.Add({ 12, 16, USolidWorldMap::RiverMarker });
	Marks.Add({ 20, 16, USolidWorldMap::LocationMarker });

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
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
	const float CellW = (BridgeTestMax.X - BridgeTestMin.X) / static_cast<float>(USolidWorldMap::MapWidth);

	TArray<FBridgeTestMark> ShortMarks;
	ShortMarks.Add({ 2, 20, USolidWorldMap::LocationMarker });
	ShortMarks.Add({ 10, 20, USolidWorldMap::RiverMarker });
	ShortMarks.Add({ 18, 20, USolidWorldMap::LocationMarker });
	USolidWorldMap* ShortMap = NewObject<USolidWorldMap>();
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
	LongMarks.Add({ 2, 20, USolidWorldMap::LocationMarker });
	LongMarks.Add({ 18, 20, USolidWorldMap::LocationMarker });
	constexpr int32 RiverCells = 5;
	for (int32 Col = 8; Col < 8 + RiverCells; ++Col)
	{
		LongMarks.Add({ Col, 20, USolidWorldMap::RiverMarker });
	}
	USolidWorldMap* LongMap = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("long map"), LongMap);
	if (!TestTrue(TEXT("long map loads"), LongMap->LoadFromString(BridgeTestGrid(LongMarks, BridgeTestNamed(2, 20, TEXT("Iglin"), 18, 20, TEXT("Relion"), TEXT("trail"))))))
	{
		return false;
	}
	TArray<SolidBridge::FBridge> LongBridges;
	if (!TestEqual(TEXT("long crossing"), SolidBridge::Collect(*LongMap, BridgeTestMin, BridgeTestMax, Height, LongBridges), 1)
		|| LongBridges.Num() != 1)
	{
		return false;
	}

	TestTrue(TEXT("a trail center is 1 m"), FMath::IsNearlyEqual(ShortBridges[0].WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Trail)));
	TestTrue(TEXT("a longer river lengthens the center"),
		FMath::IsNearlyEqual(
			BridgePlanLength(LongBridges[0].Deck),
			SolidRiver::WidthCm + static_cast<float>(RiverCells - 1) * CellW + 2.f * SolidBridge::BankRestCm,
			5.f));
	TestTrue(TEXT("the long center is longer than one channel"),
		BridgePlanLength(LongBridges[0].Deck) > BridgePlanLength(ShortBridges[0].Deck) + CellW);

	TArray<FBridgeTestMark> Blocked;
	Blocked.Add({ 4, 20, USolidWorldMap::LocationMarker });
	Blocked.Add({ 20, 20, USolidWorldMap::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		Blocked.Add({ 12, Row, USolidWorldMap::RiverMarker });
	}
	USolidWorldMap* Wide = NewObject<USolidWorldMap>();
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
	Gap.Add({ 4, 10, USolidWorldMap::LocationMarker });
	Gap.Add({ 22, 10, USolidWorldMap::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		if (Row != 22)
		{
			Gap.Add({ 12, Row, USolidWorldMap::RiverMarker });
		}
	}
	USolidWorldMap* Around = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("around map"), Around);
	if (TestTrue(TEXT("around map loads"), Around->LoadFromString(BridgeTestGrid(Gap, BridgeTestNamed(4, 10, TEXT("Iglin"), 22, 10, TEXT("Relion"), TEXT("road"))))))
	{
		TArray<SolidBridge::FBridge> AroundBridges;
		TestEqual(TEXT("a road that stays off the water has no bridge"),
			SolidBridge::Collect(*Around, BridgeTestMin, BridgeTestMax, Height, AroundBridges), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeShippedTest,
	"SolidCore1.Bridge.Shipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeShippedTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("shipped map"), Map);
	if (!TestTrue(TEXT("shipped map loads"), Map && Map->LoadDefault()))
	{
		return false;
	}

	const auto Height = [](float, float) { return 0.f; };
	TArray<SolidBridge::FBridge> Bridges;
	TestTrue(TEXT("the shipped roads cross water"), SolidBridge::Collect(*Map, BridgeTestMin, BridgeTestMax, Height, Bridges) >= 1);
	for (const SolidBridge::FBridge& Bridge : Bridges)
	{
		const bool bKnownWidth = FMath::IsNearlyEqual(Bridge.WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Trail))
			|| FMath::IsNearlyEqual(Bridge.WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Track))
			|| FMath::IsNearlyEqual(Bridge.WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Road))
			|| FMath::IsNearlyEqual(Bridge.WidthCm, SolidRoad::WidthCm(SolidRoad::EType::Highway));
		TestTrue(TEXT("width is a route width"), bKnownWidth);
		TestTrue(TEXT("shipped center is flat"), BridgeFlat(Bridge.Deck));
		TestTrue(TEXT("shipped center covers more than the channel"),
			BridgePlanLength(Bridge.Deck) > SolidRiver::WidthCm);
		TestTrue(TEXT("shipped ramps leave the center"),
			Bridge.RampFrom.Num() >= 2 && Bridge.RampTo.Num() >= 2
			&& Bridge.RampFrom[0].Equals(Bridge.Deck[0], 1.f)
			&& Bridge.RampTo[0].Equals(Bridge.Deck.Last(), 1.f));
		TestTrue(TEXT("shipped ramps run into the ground"), BridgeBuried(Bridge.RampFrom, Height) && BridgeBuried(Bridge.RampTo, Height));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeAlignedTest,
	"SolidCore1.Bridge.Aligned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeAlignedTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("map"), Map);
	if (!TestTrue(TEXT("default map loads"), Map && Map->LoadDefault()))
	{
		return false;
	}

	const auto Height = [](float, float) { return 0.f; };
	TArray<SolidBridge::FBridge> Bridges;
	TestTrue(TEXT("the shipped roads cross water"),
		SolidBridge::Collect(*Map, BridgeTestMin, BridgeTestMax, Height, Bridges) >= 1);

	const SolidBridge::FBridge* Iglin = nullptr;
	int32 IglinCount = 0;
	for (const SolidBridge::FBridge& Bridge : Bridges)
	{
		if (Bridge.bIglinRelion)
		{
			++IglinCount;
			Iglin = &Bridge;
		}
	}
	if (!TestEqual(TEXT("one Iglin bridge"), IglinCount, 1) || !Iglin)
	{
		return false;
	}

	const SolidRoad::FRoute* Track = nullptr;
	for (const SolidRoad::FRoute& Route : Map->GetRoads())
	{
		const int32 From = Route.Definition.From.LocationId;
		const int32 To = Route.Definition.To.LocationId;
		if ((From == 0 && To == 1) || (From == 1 && To == 0))
		{
			Track = &Route;
			break;
		}
	}
	if (!TestTrue(TEXT("the Iglin track is listed"), Track != nullptr) || !Track)
	{
		return false;
	}
	FVector2D River = FVector2D::ZeroVector;
	bool bFound = false;
	for (const FIntPoint& Cell : Track->Cells)
	{
		if (Map->GetMarkerAtCell(Cell.X, Cell.Y) == USolidWorldMap::RiverMarker)
		{
			bFound = Map->CellToWorldXY(Cell, BridgeTestMin, BridgeTestMax, River);
			break;
		}
	}
	TestTrue(TEXT("river cell"), bFound);

	const SolidBridge::FBridge& Bridge = *Iglin;
	// Bridge.Aligned used to expect the cell centre. The painted strip is half a
	// quad southwest of that centre, and the bridge follows the strip.
	const FVector2D Paint = SolidRoad::StripPaintOffset();
	TestTrue(TEXT("paint offset is half a quad southwest"),
		FMath::IsNearlyEqual(Paint.X, -0.5f * SolidRoad::TerrainQuadCm)
		&& FMath::IsNearlyEqual(Paint.Y, -0.5f * SolidRoad::TerrainQuadCm));
	auto OnRoad = [&River, Paint](const FVector& Point)
	{
		return FMath::Abs(Point.Y - (River.Y + Paint.Y)) < 1.f;
	};
	for (const FVector& Point : Bridge.Deck)
	{
		TestTrue(TEXT("the center lies on the road"), OnRoad(Point));
	}
	for (const FVector& Point : Bridge.RampFrom)
	{
		TestTrue(TEXT("the near ramp lies on the road"), OnRoad(Point));
	}
	for (const FVector& Point : Bridge.RampTo)
	{
		TestTrue(TEXT("the far ramp lies on the road"), OnRoad(Point));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBridgeWalkableTest,
	"SolidCore1.Bridge.Walkable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBridgeWalkableTest::RunTest(const FString& Parameters)
{
	ASolidBridge* BridgeActor = NewObject<ASolidBridge>();
	TestNotNull(TEXT("bridge"), BridgeActor);
	if (!BridgeActor)
	{
		return false;
	}

	SolidBridge::FBridge Spec;
	Spec.WidthCm = SolidRoad::WidthCm(SolidRoad::EType::Track);
	Spec.Deck = { FVector(0.f, 0.f, 200.f), FVector(600.f, 0.f, 200.f) };
	Spec.RampFrom = { FVector(0.f, 0.f, 200.f), FVector(-400.f, 0.f, 100.f), FVector(-800.f, 0.f, 0.f) };
	Spec.RampTo = { FVector(600.f, 0.f, 200.f), FVector(1000.f, 0.f, 100.f), FVector(1400.f, 0.f, 0.f) };
	BridgeActor->SetBridge(Spec);

	UStaticMeshComponent* Meshes[] = {
		BridgeActor->GetRampFromMesh(),
		BridgeActor->GetDeckMesh(),
		BridgeActor->GetRampToMesh(),
	};
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		TestNotNull(TEXT("piece"), Mesh);
		UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
		TestNotNull(TEXT("mesh"), Asset);
		if (!Asset)
		{
			continue;
		}
		UBodySetup* Body = Asset->GetBodySetup();
		TestNotNull(TEXT("body"), Body);
		if (!Body)
		{
			continue;
		}
		TestEqual(TEXT("walk the triangles"),
			static_cast<uint8>(Body->CollisionTraceFlag),
			static_cast<uint8>(CTF_UseComplexAsSimple));
		TestEqual(TEXT("no box stands in front of the ramp"), Body->AggGeom.BoxElems.Num(), 0);
	}
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
		Marks.Add({ 4, 16, USolidWorldMap::LocationMarker });
		Marks.Add({ 12, 16, USolidWorldMap::RiverMarker });
		Marks.Add({ 20, 16, USolidWorldMap::LocationMarker });
		const FString Lines = FString::Printf(
			TEXT("! 17,5 = %s (%s)\n! 17,21 = %s (%s)\nRoad: %s -> %s, %s\n"),
			FromKind, FromName, ToKind, ToName, FromName, ToName, Type);
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
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

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("shipped map"), Shipped);
	if (TestTrue(TEXT("shipped map loads"), Shipped && Shipped->LoadDefault()))
	{
		TArray<SolidBridge::FBridge> ShippedBridges;
		TestTrue(TEXT("the shipped roads cross water"),
			SolidBridge::Collect(*Shipped, BridgeTestMin, BridgeTestMax, Height, ShippedBridges) >= 1);
		int32 IglinCount = 0;
		for (const SolidBridge::FBridge& Bridge : ShippedBridges)
		{
			if (Bridge.bIglinRelion)
			{
				++IglinCount;
				TestTrue(TEXT("the shipped Iglin bridge is broken"), Bridge.bBroken);
			}
			else
			{
				TestFalse(TEXT("another shipped bridge keeps its span"), Bridge.bBroken);
			}
		}
		TestEqual(TEXT("one shipped Iglin bridge"), IglinCount, 1);
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

	UStaticMeshComponent* Deck = Actor->GetDeckMesh();
	TestNotNull(TEXT("deck component"), Deck);
	if (Deck)
	{
		UStaticMesh* DeckAsset = Deck->GetStaticMesh();
		TestNull(TEXT("broken center is not drawn"), DeckAsset);
		TestFalse(TEXT("broken center stays hidden"), Deck->GetVisibleFlag());
		TestEqual(
			TEXT("broken center does not collide"),
			static_cast<uint8>(Deck->GetCollisionEnabled()),
			static_cast<uint8>(ECollisionEnabled::NoCollision));
	}

	UStaticMeshComponent* RampFrom = Actor->GetRampFromMesh();
	UStaticMeshComponent* RampTo = Actor->GetRampToMesh();
	UStaticMesh* RampFromAsset = RampFrom ? RampFrom->GetStaticMesh() : nullptr;
	UStaticMesh* RampToAsset = RampTo ? RampTo->GetStaticMesh() : nullptr;
	TestNotNull(TEXT("near ramp stays"), RampFromAsset);
	TestNotNull(TEXT("far ramp stays"), RampToAsset);

	Actor->SetBroken(false);
	TestFalse(TEXT("repair restores the span"), Actor->GetBridge().bBroken);
	TestTrue(TEXT("repair still knows the Iglin bridge"), Actor->GetBridge().bIglinRelion);
	UStaticMesh* Restored = Actor->GetDeckMesh() ? Actor->GetDeckMesh()->GetStaticMesh() : nullptr;
	TestNotNull(TEXT("the center is drawn"), Restored);
	if (UStaticMeshComponent* RestoredDeck = Actor->GetDeckMesh())
	{
		TestTrue(TEXT("the center is shown"), RestoredDeck->GetVisibleFlag());
		TestEqual(
			TEXT("the center collides"),
			static_cast<uint8>(RestoredDeck->GetCollisionEnabled()),
			static_cast<uint8>(ECollisionEnabled::QueryAndPhysics));
	}
	return true;
}

#endif
