#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidWallgate.h"
#include "SolidTowns.h"
#include "SolidWorldMap.h"

#include <initializer_list>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D WallgateTestMin(-51200.f, -51200.f);
	const FVector2D WallgateTestMax(51200.f, 51200.f);

	struct FWallgateTestMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = USolidWorldMap::GrassMarker;
	};

	FString WallgateTestGrid(std::initializer_list<FWallgateTestMark> Marks, const FString& Extra = FString())
	{
		FString Text;
		Text.Reserve((USolidWorldMap::MapWidth + 1) * USolidWorldMap::MapHeight + Extra.Len());
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldMap::GrassMarker;
				for (const FWallgateTestMark& Mark : Marks)
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
		Text += Extra;
		return Text;
	}

	USolidWorldMap* WallgateTestLoad(
		FAutomationTestBase& Test,
		const TCHAR* What,
		std::initializer_list<FWallgateTestMark> Marks,
		const FString& Extra = FString())
	{
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		const bool bLoaded = Map && Map->LoadFromString(WallgateTestGrid(Marks, Extra));
		Test.TestTrue(What, bLoaded);
		return bLoaded ? Map : nullptr;
	}

	bool WallgateTestNear(const float A, const float B)
	{
		return FMath::IsNearlyEqual(A, B, 0.5f);
	}

	float WallgateTestAlong(const FVector2D& Point, const bool bEastWest)
	{
		return bEastWest ? Point.X : Point.Y;
	}

	float WallgateTestSide(const FVector2D& Point, const bool bEastWest)
	{
		return bEastWest ? Point.Y : Point.X;
	}

	bool WallgateTestExpectPass(
		FAutomationTestBase& Test,
		TArray<SolidWallgate::FPiece> Pieces,
		const bool bEastWest)
	{
		if (!Test.TestEqual(TEXT("two walls and two towers"), Pieces.Num(), 4))
		{
			return false;
		}

		TArray<SolidWallgate::FPiece> Walls;
		TArray<SolidWallgate::FPiece> Towers;
		for (const SolidWallgate::FPiece& Piece : Pieces)
		{
			if (Piece.bStoneWall)
			{
				Walls.Add(Piece);
			}
			else
			{
				Towers.Add(Piece);
			}
		}
		if (!Test.TestEqual(TEXT("two wall lengths"), Walls.Num(), 2)
			|| !Test.TestEqual(TEXT("two towers"), Towers.Num(), 2))
		{
			return false;
		}

		for (const SolidWallgate::FPiece& Wall : Walls)
		{
			Test.TestTrue(TEXT("wall is stone"), Wall.bStoneWall);
			Test.TestTrue(TEXT("wall height"), WallgateTestNear(Wall.BodyHeightCm, SolidWallgate::WallHeightCm));
			Test.TestTrue(TEXT("wall has no roof"), WallgateTestNear(Wall.RoofHeightCm, 0.f));
			Test.TestTrue(TEXT("wall thickness"), WallgateTestNear(Wall.FootprintYCm, SolidWallgate::WallThicknessCm));
			Test.TestTrue(
				TEXT("wall yaw follows the pass"),
				WallgateTestNear(Wall.YawDeg, bEastWest ? 0.f : 90.f));
		}

		for (const SolidWallgate::FPiece& Tower : Towers)
		{
			Test.TestFalse(TEXT("tower uses the building mesh"), Tower.bStoneWall);
			Test.TestTrue(TEXT("tower body"), WallgateTestNear(Tower.BodyHeightCm, SolidWallgate::TowerBodyCm));
			Test.TestTrue(TEXT("tower roof"), WallgateTestNear(Tower.RoofHeightCm, SolidWallgate::TowerRoofCm));
			Test.TestTrue(TEXT("tower length"), WallgateTestNear(Tower.FootprintXCm, SolidWallgate::TowerFootprintXCm));
			Test.TestTrue(TEXT("tower depth"), WallgateTestNear(Tower.FootprintYCm, SolidWallgate::TowerFootprintYCm));
			Test.TestTrue(
				TEXT("tower yaw matches the wall"),
				WallgateTestNear(Tower.YawDeg, bEastWest ? 0.f : 90.f));
		}

		Test.TestTrue(TEXT("towers are the same height"),
			WallgateTestNear(Towers[0].BodyHeightCm, Towers[1].BodyHeightCm)
			&& WallgateTestNear(Towers[0].RoofHeightCm, Towers[1].RoofHeightCm));
		Test.TestTrue(TEXT("towers stand above the wall"),
			Towers[0].BodyHeightCm > Walls[0].BodyHeightCm
			&& Towers[1].BodyHeightCm > Walls[1].BodyHeightCm);

		Walls.Sort([bEastWest](const SolidWallgate::FPiece& A, const SolidWallgate::FPiece& B)
		{
			return WallgateTestAlong(A.CenterXY, bEastWest) < WallgateTestAlong(B.CenterXY, bEastWest);
		});
		Towers.Sort([bEastWest](const SolidWallgate::FPiece& A, const SolidWallgate::FPiece& B)
		{
			return WallgateTestAlong(A.CenterXY, bEastWest) < WallgateTestAlong(B.CenterXY, bEastWest);
		});

		const float GapLow = WallgateTestAlong(Walls[0].CenterXY, bEastWest) + Walls[0].FootprintXCm * 0.5f;
		const float GapHigh = WallgateTestAlong(Walls[1].CenterXY, bEastWest) - Walls[1].FootprintXCm * 0.5f;
		Test.TestTrue(TEXT("the gate is 4 m"), WallgateTestNear(GapHigh - GapLow, SolidWallgate::GapCm));

		const float TowerLowInner = WallgateTestAlong(Towers[0].CenterXY, bEastWest) + Towers[0].FootprintXCm * 0.5f;
		const float TowerHighInner = WallgateTestAlong(Towers[1].CenterXY, bEastWest) - Towers[1].FootprintXCm * 0.5f;
		Test.TestTrue(TEXT("first tower stops at the gap"), WallgateTestNear(TowerLowInner, GapLow));
		Test.TestTrue(TEXT("second tower stops at the gap"), WallgateTestNear(TowerHighInner, GapHigh));

		for (const SolidWallgate::FPiece& Tower : Towers)
		{
			const float WallSide = WallgateTestSide(Walls[0].CenterXY, bEastWest);
			const float TowerSide = WallgateTestSide(Tower.CenterXY, bEastWest);
			Test.TestTrue(TEXT("tower stands on the north or east face"), TowerSide > WallSide);
			const float WallOuter = WallSide + Walls[0].FootprintYCm * 0.5f;
			const float TowerInner = TowerSide - Tower.FootprintYCm * 0.5f;
			Test.TestTrue(
				TEXT("tower connects to the wall"),
				WallgateTestNear(WallOuter - TowerInner, SolidWallgate::ConnectCm));
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWallgateWallTest,
	"SolidCore1.Towns.WallGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWallgateWallTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("the wall is 4 m"), FMath::IsNearlyEqual(SolidWallgate::WallHeightCm, 400.f));
	TestTrue(TEXT("the tower body is taller than the wall"),
		SolidWallgate::TowerBodyCm > SolidWallgate::WallHeightCm);

	TArray<SolidWallgate::FPiece> Missing;
	TestEqual(TEXT("null map builds nothing"),
		SolidWallgate::CollectPieces(nullptr, WallgateTestMin, WallgateTestMax, Missing), 0);

	USolidWorldMap* Grass = WallgateTestLoad(*this, TEXT("grass map loads"), {});
	if (Grass)
	{
		TArray<SolidWallgate::FPiece> None;
		TestEqual(TEXT("grass builds nothing"),
			SolidWallgate::CollectPieces(Grass, WallgateTestMin, WallgateTestMax, None), 0);
	}

	USolidWorldMap* EastWest = WallgateTestLoad(*this, TEXT("east-west pass loads"), {
		{ 11, 21, TEXT('M') },
		{ 12, 21, USolidWorldMap::LocationMarker },
		{ 13, 21, TEXT('M') },
	}, TEXT("! 22,13 = Town Dragongate\n"));
	if (EastWest)
	{
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(EastWest, WallgateTestMin, WallgateTestMax, Pieces);
		if (WallgateTestExpectPass(*this, Pieces, true))
		{
			const float HalfCell = (WallgateTestMax.X - WallgateTestMin.X)
				/ static_cast<float>(USolidWorldMap::MapWidth) * 0.5f;
			FVector2D Center = FVector2D::ZeroVector;
			TestTrue(TEXT("pass cell projects"),
				EastWest->CellToWorldXY(FIntPoint(12, 21), WallgateTestMin, WallgateTestMax, Center));
			const float SpanMin = Center.X - HalfCell;
			const float SpanMax = Center.X + HalfCell;

			float OuterMin = TNumericLimits<float>::Max();
			float OuterMax = TNumericLimits<float>::Lowest();
			for (const SolidWallgate::FPiece& Piece : Pieces)
			{
				if (!Piece.bStoneWall)
				{
					continue;
				}
				OuterMin = FMath::Min(OuterMin, Piece.CenterXY.X - Piece.FootprintXCm * 0.5f);
				OuterMax = FMath::Max(OuterMax, Piece.CenterXY.X + Piece.FootprintXCm * 0.5f);
			}
			TestTrue(TEXT("wall meets the outer edges of the town cell"),
				WallgateTestNear(OuterMin, SpanMin) && WallgateTestNear(OuterMax, SpanMax));
			TestTrue(TEXT("one cell of wall"), WallgateTestNear(SpanMax - SpanMin, HalfCell * 2.f));
		}
	}

	// A vertical stack of named cells is one north-south gate.
	USolidWorldMap* NorthSouth = WallgateTestLoad(*this, TEXT("north-south pass loads"), {
		{ 12, 19, TEXT('M') },
		{ 12, 20, USolidWorldMap::LocationMarker },
		{ 12, 21, USolidWorldMap::LocationMarker },
		{ 12, 22, USolidWorldMap::LocationMarker },
		{ 12, 23, TEXT('M') },
	}, TEXT("! 21,13 = Town Dragongate\n! 22,13 = Town Dragongate\n! 23,13 = Town Dragongate\n"));
	if (NorthSouth)
	{
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(NorthSouth, WallgateTestMin, WallgateTestMax, Pieces);
		WallgateTestExpectPass(*this, Pieces, false);
	}

	USolidWorldMap* Lone = WallgateTestLoad(*this, TEXT("lone town loads"), {
		{ 12, 21, USolidWorldMap::LocationMarker },
	}, TEXT("! 22,13 = Town Dragongate\n"));
	if (Lone)
	{
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(Lone, WallgateTestMin, WallgateTestMax, Pieces);
		WallgateTestExpectPass(*this, Pieces, true);
	}

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped && Shipped->LoadDefault());
	if (Shipped && Shipped->IsLoaded())
	{
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(Shipped, WallgateTestMin, WallgateTestMax, Pieces);
		int32 WallGateId = INDEX_NONE;
		for (const SolidTowns::FTownInfo& Info : SolidTowns::Definitions)
		{
			if (Info.Type == SolidTowns::ETownType::WallGate)
			{
				WallGateId = Info.Id;
				break;
			}
		}
		FSolidWorldLocation Town;
		if (WallGateId == INDEX_NONE || !Shipped->FindLocation(WallGateId, Town))
		{
			TestEqual(TEXT("a wall gate is absent until it is named"), Pieces.Num(), 0);
		}
		else
		{
			const bool bEastWest = Pieces.Num() > 0 && FMath::Abs(Pieces[0].YawDeg) < 1.f;
			WallgateTestExpectPass(*this, Pieces, bEastWest);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWallgateDigitPairTest,
	"SolidCore1.Towns.WallGateDigitPair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWallgateDigitPairTest::RunTest(const FString& Parameters)
{
	// Mountains stand north of both cells, south of the west cell, and on the
	// outer sides. Both cells are the same town. One cell in this pattern
	// would run north-south; the pair is one east-west gate.
	USolidWorldMap* Map = WallgateTestLoad(*this, TEXT("two-cell pass loads"), {
		{ 11, 20, TEXT('M') },
		{ 12, 20, TEXT('M') },
		{ 10, 21, TEXT('M') },
		{ 11, 21, USolidWorldMap::LocationMarker },
		{ 12, 21, USolidWorldMap::LocationMarker },
		{ 13, 21, TEXT('M') },
		{ 12, 22, TEXT('M') },
	}, TEXT("! 22,12 = Town Dragongate\n! 22,13 = Town Dragongate\n"));
	if (!Map)
	{
		return true;
	}

	TestEqual(TEXT("east cell is Dragongate"), Map->GetLocationIdAtCell(11, 21), SolidTowns::Dragongate);
	TestEqual(TEXT("west cell is Dragongate"), Map->GetLocationIdAtCell(12, 21), SolidTowns::Dragongate);
	FSolidWorldLocation Gate;
	TestTrue(TEXT("location 5"), Map->FindLocation(SolidTowns::Dragongate, Gate));
	TestEqual(TEXT("the pair is two cells"), Gate.Cells.Num(), 2);

	TArray<SolidWallgate::FPiece> Pieces;
	SolidWallgate::CollectPieces(Map, WallgateTestMin, WallgateTestMax, Pieces);
	if (!WallgateTestExpectPass(*this, Pieces, true))
	{
		return true;
	}

	const float HalfCell = (WallgateTestMax.X - WallgateTestMin.X)
		/ static_cast<float>(USolidWorldMap::MapWidth) * 0.5f;
	FVector2D East = FVector2D::ZeroVector;
	FVector2D West = FVector2D::ZeroVector;
	TestTrue(TEXT("east cell projects"),
		Map->CellToWorldXY(FIntPoint(11, 21), WallgateTestMin, WallgateTestMax, East));
	TestTrue(TEXT("west cell projects"),
		Map->CellToWorldXY(FIntPoint(12, 21), WallgateTestMin, WallgateTestMax, West));
	const float SpanMax = East.X + HalfCell;
	const float SpanMin = West.X - HalfCell;

	float OuterMin = TNumericLimits<float>::Max();
	float OuterMax = TNumericLimits<float>::Lowest();
	for (const SolidWallgate::FPiece& Piece : Pieces)
	{
		if (!Piece.bStoneWall)
		{
			continue;
		}
		OuterMin = FMath::Min(OuterMin, Piece.CenterXY.X - Piece.FootprintXCm * 0.5f);
		OuterMax = FMath::Max(OuterMax, Piece.CenterXY.X + Piece.FootprintXCm * 0.5f);
	}
	TestTrue(TEXT("wall meets the outer edges of both cells"),
		WallgateTestNear(OuterMin, SpanMin) && WallgateTestNear(OuterMax, SpanMax));
	TestTrue(TEXT("two cells of wall"), WallgateTestNear(OuterMax - OuterMin, HalfCell * 4.f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
