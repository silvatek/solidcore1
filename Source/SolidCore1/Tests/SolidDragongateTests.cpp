#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDragongate.h"
#include "SolidTowns.h"
#include "SolidWorldMap.h"

#include <initializer_list>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D DragongateTestMin(-51200.f, -51200.f);
	const FVector2D DragongateTestMax(51200.f, 51200.f);

	struct FDragongateTestMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = TEXT('G');
	};

	FString DragongateTestGrid(std::initializer_list<FDragongateTestMark> Marks)
	{
		FString Text;
		Text.Reserve((USolidWorldMap::MapWidth + 1) * USolidWorldMap::MapHeight);
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				TCHAR Marker = TEXT('G');
				for (const FDragongateTestMark& Mark : Marks)
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

	USolidWorldMap* DragongateTestLoad(
		FAutomationTestBase& Test,
		const TCHAR* What,
		std::initializer_list<FDragongateTestMark> Marks)
	{
		USolidWorldMap* Map = NewObject<USolidWorldMap>();
		const bool bLoaded = Map && Map->LoadFromString(DragongateTestGrid(Marks));
		Test.TestTrue(What, bLoaded);
		return bLoaded ? Map : nullptr;
	}

	bool DragongateTestNear(const float A, const float B)
	{
		return FMath::IsNearlyEqual(A, B, 0.5f);
	}

	float DragongateTestAlong(const FVector2D& Point, const bool bEastWest)
	{
		return bEastWest ? Point.X : Point.Y;
	}

	float DragongateTestSide(const FVector2D& Point, const bool bEastWest)
	{
		return bEastWest ? Point.Y : Point.X;
	}

	bool DragongateTestExpectPass(
		FAutomationTestBase& Test,
		TArray<SolidDragongate::FPiece> Pieces,
		const bool bEastWest)
	{
		if (!Test.TestEqual(TEXT("two walls and two towers"), Pieces.Num(), 4))
		{
			return false;
		}

		TArray<SolidDragongate::FPiece> Walls;
		TArray<SolidDragongate::FPiece> Towers;
		for (const SolidDragongate::FPiece& Piece : Pieces)
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

		for (const SolidDragongate::FPiece& Wall : Walls)
		{
			Test.TestTrue(TEXT("wall is stone"), Wall.bStoneWall);
			Test.TestTrue(TEXT("wall height"), DragongateTestNear(Wall.BodyHeightCm, SolidDragongate::WallHeightCm));
			Test.TestTrue(TEXT("wall has no roof"), DragongateTestNear(Wall.RoofHeightCm, 0.f));
			Test.TestTrue(TEXT("wall thickness"), DragongateTestNear(Wall.FootprintYCm, SolidDragongate::WallThicknessCm));
			Test.TestTrue(
				TEXT("wall yaw follows the pass"),
				DragongateTestNear(Wall.YawDeg, bEastWest ? 0.f : 90.f));
		}

		for (const SolidDragongate::FPiece& Tower : Towers)
		{
			Test.TestFalse(TEXT("tower uses the building mesh"), Tower.bStoneWall);
			Test.TestTrue(TEXT("tower body"), DragongateTestNear(Tower.BodyHeightCm, SolidDragongate::TowerBodyCm));
			Test.TestTrue(TEXT("tower roof"), DragongateTestNear(Tower.RoofHeightCm, SolidDragongate::TowerRoofCm));
			Test.TestTrue(TEXT("tower length"), DragongateTestNear(Tower.FootprintXCm, SolidDragongate::TowerFootprintXCm));
			Test.TestTrue(TEXT("tower depth"), DragongateTestNear(Tower.FootprintYCm, SolidDragongate::TowerFootprintYCm));
			Test.TestTrue(
				TEXT("tower yaw matches the wall"),
				DragongateTestNear(Tower.YawDeg, bEastWest ? 0.f : 90.f));
		}

		Test.TestTrue(TEXT("towers are the same height"),
			DragongateTestNear(Towers[0].BodyHeightCm, Towers[1].BodyHeightCm)
			&& DragongateTestNear(Towers[0].RoofHeightCm, Towers[1].RoofHeightCm));

		Walls.Sort([bEastWest](const SolidDragongate::FPiece& A, const SolidDragongate::FPiece& B)
		{
			return DragongateTestAlong(A.CenterXY, bEastWest) < DragongateTestAlong(B.CenterXY, bEastWest);
		});
		Towers.Sort([bEastWest](const SolidDragongate::FPiece& A, const SolidDragongate::FPiece& B)
		{
			return DragongateTestAlong(A.CenterXY, bEastWest) < DragongateTestAlong(B.CenterXY, bEastWest);
		});

		const float GapLow = DragongateTestAlong(Walls[0].CenterXY, bEastWest) + Walls[0].FootprintXCm * 0.5f;
		const float GapHigh = DragongateTestAlong(Walls[1].CenterXY, bEastWest) - Walls[1].FootprintXCm * 0.5f;
		Test.TestTrue(TEXT("the gate is 4 m"), DragongateTestNear(GapHigh - GapLow, SolidDragongate::GapCm));

		const float TowerLowInner = DragongateTestAlong(Towers[0].CenterXY, bEastWest) + Towers[0].FootprintXCm * 0.5f;
		const float TowerHighInner = DragongateTestAlong(Towers[1].CenterXY, bEastWest) - Towers[1].FootprintXCm * 0.5f;
		Test.TestTrue(TEXT("first tower stops at the gap"), DragongateTestNear(TowerLowInner, GapLow));
		Test.TestTrue(TEXT("second tower stops at the gap"), DragongateTestNear(TowerHighInner, GapHigh));

		for (const SolidDragongate::FPiece& Tower : Towers)
		{
			const float WallSide = DragongateTestSide(Walls[0].CenterXY, bEastWest);
			const float TowerSide = DragongateTestSide(Tower.CenterXY, bEastWest);
			Test.TestTrue(TEXT("tower stands on the north or east face"), TowerSide > WallSide);
			const float WallOuter = WallSide + Walls[0].FootprintYCm * 0.5f;
			const float TowerInner = TowerSide - Tower.FootprintYCm * 0.5f;
			Test.TestTrue(
				TEXT("tower connects to the wall"),
				DragongateTestNear(WallOuter - TowerInner, SolidDragongate::ConnectCm));
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDragongateWallTest,
	"SolidCore1.Towns.Dragongate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDragongateWallTest::RunTest(const FString& Parameters)
{
	TArray<SolidDragongate::FPiece> Missing;
	TestEqual(TEXT("null map builds nothing"),
		SolidDragongate::CollectPieces(nullptr, DragongateTestMin, DragongateTestMax, Missing), 0);

	USolidWorldMap* Grass = DragongateTestLoad(*this, TEXT("grass map loads"), {});
	if (Grass)
	{
		TArray<SolidDragongate::FPiece> None;
		TestEqual(TEXT("no digit 5 builds nothing"),
			SolidDragongate::CollectPieces(Grass, DragongateTestMin, DragongateTestMax, None), 0);
	}

	USolidWorldMap* EastWest = DragongateTestLoad(*this, TEXT("east-west pass loads"), {
		{ 10, 21, TEXT('M') },
		{ 11, 21, TEXT('5') },
		{ 12, 21, TEXT('5') },
		{ 13, 21, TEXT('5') },
		{ 14, 21, TEXT('M') },
	});
	if (EastWest)
	{
		TArray<SolidDragongate::FPiece> Pieces;
		SolidDragongate::CollectPieces(EastWest, DragongateTestMin, DragongateTestMax, Pieces);
		if (DragongateTestExpectPass(*this, Pieces, true))
		{
			const float HalfCell = (DragongateTestMax.X - DragongateTestMin.X)
				/ static_cast<float>(USolidWorldMap::MapWidth) * 0.5f;
			float SpanMin = TNumericLimits<float>::Max();
			float SpanMax = TNumericLimits<float>::Lowest();
			for (int32 Col = 11; Col <= 13; ++Col)
			{
				FVector2D Center = FVector2D::ZeroVector;
				TestTrue(TEXT("pass cell projects"),
					EastWest->CellToWorldXY(FIntPoint(Col, 21), DragongateTestMin, DragongateTestMax, Center));
				SpanMin = FMath::Min(SpanMin, Center.X - HalfCell);
				SpanMax = FMath::Max(SpanMax, Center.X + HalfCell);
			}

			float OuterMin = TNumericLimits<float>::Max();
			float OuterMax = TNumericLimits<float>::Lowest();
			for (const SolidDragongate::FPiece& Piece : Pieces)
			{
				if (!Piece.bStoneWall)
				{
					continue;
				}
				OuterMin = FMath::Min(OuterMin, Piece.CenterXY.X - Piece.FootprintXCm * 0.5f);
				OuterMax = FMath::Max(OuterMax, Piece.CenterXY.X + Piece.FootprintXCm * 0.5f);
			}
			TestTrue(TEXT("wall meets the outer edges of the town cells"),
				DragongateTestNear(OuterMin, SpanMin) && DragongateTestNear(OuterMax, SpanMax));
			TestTrue(TEXT("three cells of wall"), DragongateTestNear(SpanMax - SpanMin, HalfCell * 6.f));
		}
	}

	USolidWorldMap* NorthSouth = DragongateTestLoad(*this, TEXT("north-south pass loads"), {
		{ 11, 20, TEXT('M') }, { 12, 20, TEXT('M') }, { 13, 20, TEXT('M') },
		{ 11, 21, TEXT('5') }, { 12, 21, TEXT('5') }, { 13, 21, TEXT('5') },
		{ 11, 22, TEXT('M') }, { 12, 22, TEXT('M') }, { 13, 22, TEXT('M') },
	});
	if (NorthSouth)
	{
		TArray<SolidDragongate::FPiece> Pieces;
		SolidDragongate::CollectPieces(NorthSouth, DragongateTestMin, DragongateTestMax, Pieces);
		DragongateTestExpectPass(*this, Pieces, false);
	}

	USolidWorldMap* Lone = DragongateTestLoad(*this, TEXT("lone town loads"), {
		{ 12, 21, TEXT('5') },
	});
	if (Lone)
	{
		TArray<SolidDragongate::FPiece> Pieces;
		SolidDragongate::CollectPieces(Lone, DragongateTestMin, DragongateTestMax, Pieces);
		DragongateTestExpectPass(*this, Pieces, true);
	}

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped && Shipped->LoadDefault());
	if (Shipped && Shipped->IsLoaded())
	{
		TArray<SolidDragongate::FPiece> Pieces;
		SolidDragongate::CollectPieces(Shipped, DragongateTestMin, DragongateTestMax, Pieces);
		FSolidWorldLocation Town;
		if (!Shipped->FindLocation(SolidTowns::DragongateId, Town))
		{
			TestEqual(TEXT("Dragongate is absent until its digit is on the map"), Pieces.Num(), 0);
		}
		else
		{
			const bool bEastWest = Pieces.Num() > 0 && FMath::Abs(Pieces[0].YawDeg) < 1.f;
			DragongateTestExpectPass(*this, Pieces, bEastWest);
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
