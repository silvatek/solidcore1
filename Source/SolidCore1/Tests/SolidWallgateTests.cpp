#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidWallgate.h"
#include "SolidTowns.h"
#include "SolidTownSigns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D WallgateTestMin(-51200.f, -51200.f);
	const FVector2D WallgateTestMax(51200.f, 51200.f);

	const FIntPoint WallgateMarkerCell(40, 30);

	FString WallgateGrassText(const TArray<FIntPoint>& Markers, const FString& Extra)
	{
		FString Text;
		Text.Reserve((USolidWorldChart::MapWidth + 1) * USolidWorldChart::MapHeight + Extra.Len());
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			FString Line = FString::ChrN(USolidWorldChart::MapWidth, USolidWorldChart::GrassMarker);
			for (const FIntPoint& Cell : Markers)
			{
				if (Cell.Y == Row && Cell.X >= 0 && Cell.X < USolidWorldChart::MapWidth)
				{
					Line[Cell.X] = USolidWorldChart::LocationMarker;
				}
			}
			Text += Line;
			Text.AppendChar(TEXT('\n'));
		}
		Text += Extra;
		return Text;
	}

	FString WallgateBang(const FIntPoint& Cell)
	{
		return FString::Printf(
			TEXT("! %d,%d = Town Dragongate\n"),
			Cell.Y + 1,
			Cell.X + 1);
	}

	USolidWorldChart* WallgateImport(
		FAutomationTestBase& Test,
		const TCHAR* What,
		const TArray<FIntPoint>& Markers,
		const FString& Extra)
	{
		USolidWorldChart* Map = NewObject<USolidWorldChart>();
		const bool bLoaded = Map && Map->ImportFromString(WallgateGrassText(Markers, Extra));
		Test.TestTrue(What, bLoaded);
		return bLoaded ? Map : nullptr;
	}

	float WallgateSpacing()
	{
		return (WallgateTestMax.X - WallgateTestMin.X) / static_cast<float>(USolidWorldChart::PointAxis - 1);
	}

	FIntPoint WallgatePoint(const USolidWorldChart& Map, const FVector2D& World)
	{
		FIntPoint Point = FIntPoint::ZeroValue;
		Map.TerrainPointAt(World.X, World.Y, WallgateTestMin, WallgateTestMax, Point);
		return Point;
	}

	bool WallgatePaint(
		FAutomationTestBase& Test,
		USolidWorldChart& Map,
		const FIntPoint& Origin,
		const int32 StepEast,
		const ESolidBiome Biome)
	{
		FString Error;
		const bool bPainted = Map.PaintBrush({ FIntPoint(Origin.X + StepEast, Origin.Y) }, 0, Biome, Error);
		Test.TestTrue(TEXT("point paints"), bPainted);
		return bPainted;
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
		for (const SolidWallgate::FPiece& Piece : Pieces)
		{
			Test.TestTrue(TEXT("the top is measured from one marker"),
				Piece.MarkerXY.Equals(Pieces[0].MarkerXY, 0.5f));
		}

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

	bool WallgateCovers(const TArray<SolidWallgate::FPiece>& Walls, const float X)
	{
		for (const SolidWallgate::FPiece& Wall : Walls)
		{
			const float Low = Wall.CenterXY.X - Wall.FootprintXCm * 0.5f;
			const float High = Wall.CenterXY.X + Wall.FootprintXCm * 0.5f;
			if (X >= Low - 0.5f && X <= High + 0.5f)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * EastSteps and WestSteps are the mountain, counted in points from the marker.
	 * The curtain ends one point further into each mountain. Open points between sit on a wall.
	 */
	bool WallgateExpectRun(
		FAutomationTestBase& Test,
		const TArray<SolidWallgate::FPiece>& Pieces,
		const FVector2D& Marker,
		const int32 EastSteps,
		const int32 WestSteps)
	{
		if (!WallgateTestExpectPass(Test, Pieces, true))
		{
			return false;
		}

		TArray<SolidWallgate::FPiece> Walls;
		for (const SolidWallgate::FPiece& Piece : Pieces)
		{
			if (Piece.bStoneWall)
			{
				Walls.Add(Piece);
			}
		}
		Walls.Sort([](const SolidWallgate::FPiece& A, const SolidWallgate::FPiece& B)
		{
			return A.CenterXY.X < B.CenterXY.X;
		});
		if (!Test.TestEqual(TEXT("two curtains"), Walls.Num(), 2))
		{
			return false;
		}

		const float Spacing = WallgateSpacing();
		const float GapLow = Walls[0].CenterXY.X + Walls[0].FootprintXCm * 0.5f;
		const float GapHigh = Walls[1].CenterXY.X - Walls[1].FootprintXCm * 0.5f;
		const float OuterMin = Walls[0].CenterXY.X - Walls[0].FootprintXCm * 0.5f;
		const float OuterMax = Walls[1].CenterXY.X + Walls[1].FootprintXCm * 0.5f;
		Test.TestTrue(TEXT("the gate stands on the marker"),
			WallgateTestNear((GapLow + GapHigh) * 0.5f, Marker.X));
		Test.TestTrue(TEXT("that marker is where the top is measured"),
			Pieces[0].MarkerXY.Equals(Marker, 0.5f));
		Test.TestTrue(TEXT("the curtain finishes inside the east mountain"),
			WallgateTestNear(OuterMax, Marker.X + static_cast<float>(EastSteps + 1) * Spacing));
		Test.TestTrue(TEXT("the curtain finishes inside the west mountain"),
			WallgateTestNear(OuterMin, Marker.X - static_cast<float>(WestSteps + 1) * Spacing));

		for (int32 Step = 1; Step < EastSteps; ++Step)
		{
			Test.TestTrue(TEXT("the east curtain covers the open point"),
				WallgateCovers(Walls, Marker.X + static_cast<float>(Step) * Spacing));
		}
		for (int32 Step = 1; Step < WestSteps; ++Step)
		{
			Test.TestTrue(TEXT("the west curtain covers the open point"),
				WallgateCovers(Walls, Marker.X - static_cast<float>(Step) * Spacing));
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

	USolidWorldChart* Grass = WallgateImport(*this, TEXT("grass map loads"), {}, FString());
	if (Grass)
	{
		TArray<SolidWallgate::FPiece> None;
		TestEqual(TEXT("grass builds nothing"),
			SolidWallgate::CollectPieces(Grass, WallgateTestMin, WallgateTestMax, None), 0);
	}

	USolidWorldChart* GrassRun = WallgateImport(
		*this, TEXT("grass run loads"), { WallgateMarkerCell }, WallgateBang(WallgateMarkerCell));
	if (GrassRun)
	{
		FVector2D Marker = FVector2D::ZeroVector;
		TestTrue(TEXT("marker projects"),
			GrassRun->CellToWorldXY(WallgateMarkerCell, WallgateTestMin, WallgateTestMax, Marker));
		const FIntPoint Origin = WallgatePoint(*GrassRun, Marker);
		// The town cell copies as town. Paint the open points back to grassland: M..!..M.
		const int32 GrassSteps[] = { 1, 2, -1, -2 };
		for (const int32 Step : GrassSteps)
		{
			WallgatePaint(*this, *GrassRun, Origin, Step, ESolidBiome::Grassland);
		}
		WallgatePaint(*this, *GrassRun, Origin, 3, ESolidBiome::Mountain);
		WallgatePaint(*this, *GrassRun, Origin, -3, ESolidBiome::Mountain);
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(GrassRun, WallgateTestMin, WallgateTestMax, Pieces);
		WallgateExpectRun(*this, Pieces, Marker, 3, 3);
	}

	USolidWorldChart* TownRun = WallgateImport(
		*this, TEXT("town run loads"), { WallgateMarkerCell }, WallgateBang(WallgateMarkerCell));
	if (TownRun)
	{
		FVector2D Marker = FVector2D::ZeroVector;
		TestTrue(TEXT("town marker projects"),
			TownRun->CellToWorldXY(WallgateMarkerCell, WallgateTestMin, WallgateTestMax, Marker));
		const FIntPoint Origin = WallgatePoint(*TownRun, Marker);
		const int32 TownSteps[] = { 1, 2, -1, -2 };
		for (const int32 Step : TownSteps)
		{
			WallgatePaint(*this, *TownRun, Origin, Step, ESolidBiome::Town);
		}
		WallgatePaint(*this, *TownRun, Origin, 3, ESolidBiome::Mountain);
		WallgatePaint(*this, *TownRun, Origin, -3, ESolidBiome::Mountain);
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(TownRun, WallgateTestMin, WallgateTestMax, Pieces);
		WallgateExpectRun(*this, Pieces, Marker, 3, 3);
	}

	USolidWorldChart* Mixed = WallgateImport(
		*this, TEXT("mixed run loads"), { WallgateMarkerCell }, WallgateBang(WallgateMarkerCell));
	if (Mixed)
	{
		FVector2D Marker = FVector2D::ZeroVector;
		TestTrue(TEXT("mixed marker projects"),
			Mixed->CellToWorldXY(WallgateMarkerCell, WallgateTestMin, WallgateTestMax, Marker));
		const FIntPoint Origin = WallgatePoint(*Mixed, Marker);
		// M.o!o..M. The town cell would otherwise leave the point 4 m east as town.
		WallgatePaint(*this, *Mixed, Origin, 1, ESolidBiome::Town);
		WallgatePaint(*this, *Mixed, Origin, 2, ESolidBiome::Grassland);
		WallgatePaint(*this, *Mixed, Origin, 3, ESolidBiome::Mountain);
		WallgatePaint(*this, *Mixed, Origin, -1, ESolidBiome::Town);
		WallgatePaint(*this, *Mixed, Origin, -4, ESolidBiome::Mountain);
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(Mixed, WallgateTestMin, WallgateTestMax, Pieces);
		WallgateExpectRun(*this, Pieces, Marker, 3, 4);
	}

	USolidWorldChart* Narrow = WallgateImport(
		*this, TEXT("narrow run loads"), { WallgateMarkerCell }, WallgateBang(WallgateMarkerCell));
	if (Narrow)
	{
		FVector2D Marker = FVector2D::ZeroVector;
		TestTrue(TEXT("narrow marker projects"),
			Narrow->CellToWorldXY(WallgateMarkerCell, WallgateTestMin, WallgateTestMax, Marker));
		const FIntPoint Origin = WallgatePoint(*Narrow, Marker);
		// M.!.M. One open point on a side is narrower than the gate.
		WallgatePaint(*this, *Narrow, Origin, 1, ESolidBiome::Grassland);
		WallgatePaint(*this, *Narrow, Origin, -1, ESolidBiome::Grassland);
		WallgatePaint(*this, *Narrow, Origin, 2, ESolidBiome::Mountain);
		WallgatePaint(*this, *Narrow, Origin, -2, ESolidBiome::Mountain);
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(Narrow, WallgateTestMin, WallgateTestMax, Pieces);
		TestEqual(TEXT("a one-point gap builds nothing"), Pieces.Num(), 0);
	}

	USolidWorldChart* OneSide = WallgateImport(
		*this, TEXT("one-sided run loads"), { WallgateMarkerCell }, WallgateBang(WallgateMarkerCell));
	if (OneSide)
	{
		FVector2D Marker = FVector2D::ZeroVector;
		TestTrue(TEXT("one-sided marker projects"),
			OneSide->CellToWorldXY(WallgateMarkerCell, WallgateTestMin, WallgateTestMax, Marker));
		const FIntPoint Origin = WallgatePoint(*OneSide, Marker);
		WallgatePaint(*this, *OneSide, Origin, 3, ESolidBiome::Mountain);
		WallgatePaint(*this, *OneSide, Origin, -3, ESolidBiome::Forest);
		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(OneSide, WallgateTestMin, WallgateTestMax, Pieces);
		TestEqual(TEXT("a mountain on one side builds nothing"), Pieces.Num(), 0);
		FVector2D Opening = FVector2D::ZeroVector;
		TestTrue(TEXT("the marker stays a gate"),
			SolidWallgate::GateCenterXY(OneSide, SolidTowns::Dragongate, WallgateTestMin, WallgateTestMax, Opening));
		TestTrue(TEXT("the opening stays on the marker"), Opening.Equals(Marker, 0.5f));
	}

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Shipped && Shipped->LoadDefault());
	if (Shipped && Shipped->IsLoaded())
	{
		FSolidWorldLocation Town;
		TestTrue(TEXT("shipped Dragongate"), Shipped->FindLocation(SolidTowns::Dragongate, Town));
		TestTrue(TEXT("shipped Dragongate has a marker"), Town.Cells.Num() > 0);
		FIntPoint Eastern = Town.Cells.Num() > 0 ? Town.Cells[0] : FIntPoint::ZeroValue;
		FIntPoint Western = Eastern;
		for (const FIntPoint& Cell : Town.Cells)
		{
			if (Cell.X < Eastern.X)
			{
				Eastern = Cell;
			}
			if (Cell.X > Western.X)
			{
				Western = Cell;
			}
		}
		FVector2D EastXY = FVector2D::ZeroVector;
		FVector2D WestXY = FVector2D::ZeroVector;
		FVector2D Opening = FVector2D::ZeroVector;
		TestTrue(TEXT("eastern marker projects"),
			Shipped->CellToWorldXY(Eastern, WallgateTestMin, WallgateTestMax, EastXY));
		TestTrue(TEXT("western marker projects"),
			Shipped->CellToWorldXY(Western, WallgateTestMin, WallgateTestMax, WestXY));
		TestTrue(TEXT("the shipped gate has an opening"),
			SolidWallgate::GateCenterXY(Shipped, SolidTowns::Dragongate, WallgateTestMin, WallgateTestMax, Opening));
		TestTrue(TEXT("the shipped opening is the eastern marker"), Opening.Equals(EastXY, 0.5f));

		TArray<SolidWallgate::FPiece> Pieces;
		SolidWallgate::CollectPieces(Shipped, WallgateTestMin, WallgateTestMax, Pieces);
		if (WallgateTestExpectPass(*this, Pieces, true))
		{
			float OuterMin = TNumericLimits<float>::Max();
			float GapLow = 0.f;
			float GapHigh = 0.f;
			TArray<SolidWallgate::FPiece> Walls;
			for (const SolidWallgate::FPiece& Piece : Pieces)
			{
				if (Piece.bStoneWall)
				{
					Walls.Add(Piece);
				}
			}
			Walls.Sort([](const SolidWallgate::FPiece& A, const SolidWallgate::FPiece& B)
			{
				return A.CenterXY.X < B.CenterXY.X;
			});
			if (Walls.Num() == 2)
			{
				OuterMin = Walls[0].CenterXY.X - Walls[0].FootprintXCm * 0.5f;
				GapLow = Walls[0].CenterXY.X + Walls[0].FootprintXCm * 0.5f;
				GapHigh = Walls[1].CenterXY.X - Walls[1].FootprintXCm * 0.5f;
				TestTrue(TEXT("the shipped gate stands on the eastern marker"),
					WallgateTestNear((GapLow + GapHigh) * 0.5f, EastXY.X));
				TestTrue(TEXT("the shipped top is measured from the eastern marker"),
					Pieces[0].MarkerXY.Equals(EastXY, 0.5f));
				if (Town.Cells.Num() > 1)
				{
					TestTrue(TEXT("the curtain passes the ignored marker"), OuterMin < WestXY.X);
				}
			}
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
	const FIntPoint EastCell = WallgateMarkerCell;
	const FIntPoint WestCell(WallgateMarkerCell.X + 1, WallgateMarkerCell.Y);
	USolidWorldChart* Map = WallgateImport(
		*this,
		TEXT("two markers load"),
		{ EastCell, WestCell },
		WallgateBang(EastCell) + WallgateBang(WestCell));
	if (!Map)
	{
		return true;
	}

	FSolidWorldLocation Gate;
	TestTrue(TEXT("both markers are one town"), Map->FindLocation(SolidTowns::Dragongate, Gate));
	TestEqual(TEXT("the pair is two cells"), Gate.Cells.Num(), 2);
	FIntPoint Used = FIntPoint::ZeroValue;
	TestTrue(TEXT("the gate uses one marker"), SolidWallgate::MarkerCell(Map, SolidTowns::Dragongate, Used));
	TestEqual(TEXT("the used marker is the eastern cell"), Used, EastCell);

	FVector2D East = FVector2D::ZeroVector;
	FVector2D West = FVector2D::ZeroVector;
	TestTrue(TEXT("east marker projects"), Map->CellToWorldXY(EastCell, WallgateTestMin, WallgateTestMax, East));
	TestTrue(TEXT("west marker projects"), Map->CellToWorldXY(WestCell, WallgateTestMin, WallgateTestMax, West));

	TArray<SolidTownSigns::FPlacement> Signs;
	TestEqual(TEXT("one Dragongate sign"),
		SolidTownSigns::CollectPlacements(Map, WallgateTestMin, WallgateTestMax, Signs), 1);
	if (Signs.Num() == 1)
	{
		const FVector2D Expected = East + FVector2D(SolidTowns::DragongateSignEastCm, SolidTowns::DragongateSignNorthCm);
		TestTrue(TEXT("the sign stands by the eastern marker"), Signs[0].WorldXY.Equals(Expected, 0.5f));
	}

	const FIntPoint Origin = WallgatePoint(*Map, East);
	WallgatePaint(*this, *Map, Origin, 3, ESolidBiome::Mountain);
	WallgatePaint(*this, *Map, Origin, -3, ESolidBiome::Mountain);

	TArray<SolidWallgate::FPiece> Pieces;
	SolidWallgate::CollectPieces(Map, WallgateTestMin, WallgateTestMax, Pieces);
	WallgateExpectRun(*this, Pieces, East, 3, 3);

	FVector2D Opening = FVector2D::ZeroVector;
	TestTrue(
		TEXT("the gate has a centre"),
		SolidWallgate::GateCenterXY(Map, SolidTowns::Dragongate, WallgateTestMin, WallgateTestMax, Opening));
	TestTrue(TEXT("the opening is the eastern marker"), Opening.Equals(East, 0.5f));
	TestFalse(TEXT("the opening is not the middle of the pair"), Opening.Equals((East + West) * 0.5f, 1.f));

	FVector2D Missing = FVector2D::ZeroVector;
	TestFalse(
		TEXT("Relion is not a gate"),
		SolidWallgate::GateCenterXY(Map, SolidTowns::Relion, WallgateTestMin, WallgateTestMax, Missing));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
