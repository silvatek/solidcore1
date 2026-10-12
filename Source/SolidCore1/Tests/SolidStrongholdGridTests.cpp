#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidStrongholdGrid.h"
#include "SolidDungeonInterior.h"
#include "Components/InstancedStaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdGridMeasureTest,
	"SolidCore1.Stronghold.StrongholdHexGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdGridMeasureTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("edge to edge is 1.2 m"),
		FMath::IsNearlyEqual(SolidStrongholdGrid::FlatToFlatCm, 120.f, 0.01f));
	TestTrue(
		TEXT("point to point is longer"),
		SolidStrongholdGrid::VertexToVertexCm() > SolidStrongholdGrid::FlatToFlatCm + 10.f);
	TestTrue(
		TEXT("a side matches the point radius"),
		FMath::IsNearlyEqual(SolidStrongholdGrid::SideCm(), SolidStrongholdGrid::VertexToVertexCm() * 0.5f, 0.01f));

	const FLinearColor Yellow = SolidStrongholdGrid::LineColor();
	TestTrue(TEXT("lines are yellow"), Yellow.R > 0.9f && Yellow.G > 0.7f && Yellow.B < 0.2f);
	TestTrue(TEXT("lines are thin"), SolidStrongholdGrid::LineThicknessCm < 15.f);

	const FVector2D Origin = SolidStrongholdGrid::MeagreOrigin();
	const FVector2D East = SolidStrongholdGrid::HexCenter(1, 0, Origin);
	const FVector2D North = SolidStrongholdGrid::HexCenter(0, 1, Origin);
	TestTrue(
		TEXT("east neighbor is one flat width away"),
		FMath::IsNearlyEqual(FVector2D::Distance(Origin, East), SolidStrongholdGrid::FlatToFlatCm, 0.05f));
	TestTrue(
		TEXT("diagonal neighbor shares an edge"),
		FMath::IsNearlyEqual(FVector2D::Distance(Origin, North), SolidStrongholdGrid::FlatToFlatCm, 0.05f));

	TArray<SolidStrongholdGrid::FCell> Cells;
	SolidStrongholdGrid::MeagreCells(Cells);
	TestEqual(TEXT("meagre den is fourteen hexes"), Cells.Num(), SolidStrongholdGrid::MeagreCellCount);
	TestEqual(TEXT("fourteen cells"), SolidStrongholdGrid::MeagreCellCount, 14);

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidStrongholdGrid::CollectMeagreHexes(Hexes);
	TestEqual(TEXT("each cell has a center"), Hexes.Num(), Cells.Num());

	TMap<int32, int32> RowCounts;
	bool bHasCaptain = false;
	bool bHasEntrance = false;
	bool bHasNotch = false;
	const SolidStrongholdGrid::FCell Captain = SolidStrongholdGrid::MeagreCaptainCell();
	const SolidStrongholdGrid::FCell Entrance = SolidStrongholdGrid::MeagreEntranceCell();
	for (const SolidStrongholdGrid::FHex& Hex : Hexes)
	{
		RowCounts.FindOrAdd(Hex.Row) += 1;
		bHasCaptain |= Hex.Col == Captain.Col && Hex.Row == Captain.Row;
		bHasEntrance |= Hex.Col == Entrance.Col && Hex.Row == Entrance.Row;
		bHasNotch |= Hex.Col == -2 && Hex.Row == 1;
		TestTrue(TEXT("cell center is inside its hex"), SolidStrongholdGrid::PointInHex(Hex.Center, Hex.Center));
	}
	TestTrue(TEXT("captain has a cell"), bHasCaptain);
	TestTrue(TEXT("entrance has a cell"), bHasEntrance);
	TestFalse(TEXT("the north-west notch is open"), bHasNotch);

	const FVector2D EntranceCenter = SolidStrongholdGrid::HexCenter(Entrance.Col, Entrance.Row, Origin);
	const FVector2D CaptainCenter = SolidStrongholdGrid::HexCenter(Captain.Col, Captain.Row, Origin);
	TestTrue(
		TEXT("entrance is the west neighbor of the captain"),
		EntranceCenter.X < CaptainCenter.X
			&& FMath::IsNearlyEqual(
				FVector2D::Distance(EntranceCenter, CaptainCenter),
				SolidStrongholdGrid::FlatToFlatCm,
				0.05f));
	const FVector2D Mouth = SolidStrongholdGrid::MeagreMouthMidpoint();
	TestTrue(
		TEXT("mouth is the west flat of the entrance"),
		FMath::IsNearlyEqual(Mouth.X, EntranceCenter.X - SolidStrongholdGrid::FlatToFlatCm * 0.5f, 0.05f)
			&& FMath::IsNearlyEqual(Mouth.Y, EntranceCenter.Y, 0.05f));

	int32 NarrowestRow = MAX_int32;
	int32 WidestRow = 0;
	for (const TPair<int32, int32>& Row : RowCounts)
	{
		NarrowestRow = FMath::Min(NarrowestRow, Row.Value);
		WidestRow = FMath::Max(WidestRow, Row.Value);
	}
	TestTrue(TEXT("the outline is not a rectangle"), NarrowestRow > 0 && NarrowestRow != WidestRow);

	const FVector2D Notch = SolidStrongholdGrid::HexCenter(-2, 1, Origin);
	bool bNotchInside = false;
	for (const SolidStrongholdGrid::FHex& Hex : Hexes)
	{
		bNotchInside |= SolidStrongholdGrid::PointInHex(Notch, Hex.Center);
	}
	TestFalse(TEXT("notch center is outside every cell"), bNotchInside);

	TArray<SolidStrongholdGrid::FSegment> Segments;
	SolidStrongholdGrid::CollectSegments(Hexes, Segments);
	TestTrue(TEXT("shared edges are drawn once"), Segments.Num() < Hexes.Num() * 6);
	TestTrue(TEXT("the grid has edges"), Segments.Num() > Hexes.Num());

	TArray<SolidStrongholdGrid::FSegment> Boundary;
	SolidStrongholdGrid::CollectBoundarySegments(Hexes, Boundary);
	TestTrue(TEXT("the wall is shorter than every edge"), Boundary.Num() < Segments.Num());
	TestTrue(TEXT("the den has an outline"), Boundary.Num() > 8);

	for (const SolidStrongholdGrid::FSegment& Segment : Segments)
	{
		TestTrue(
			TEXT("each edge is one side long"),
			FMath::IsNearlyEqual(FVector2D::Distance(Segment.A, Segment.B), SolidStrongholdGrid::SideCm(), 0.05f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdGridVisualTest,
	"SolidCore1.Stronghold.StrongholdHexGridVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdGridVisualTest::RunTest(const FString& Parameters)
{
	TArray<SolidStrongholdGrid::FHex> Hexes;
	TArray<SolidStrongholdGrid::FSegment> Segments;
	SolidStrongholdGrid::CollectMeagreHexes(Hexes);
	SolidStrongholdGrid::CollectSegments(Hexes, Segments);

	ASolidDungeonInterior* Interior = NewObject<ASolidDungeonInterior>();
	TestNotNull(TEXT("interior"), Interior);
	if (!Interior)
	{
		return false;
	}
	Interior->BuildVisuals();

	UInstancedStaticMeshComponent* Grid = Interior->GetFloorGrid();
	TestNotNull(TEXT("floor grid"), Grid);
	if (!Grid)
	{
		return false;
	}

	TestEqual(TEXT("one line per edge"), Grid->GetInstanceCount(), Segments.Num());
	TestNotNull(TEXT("yellow material"), Grid->GetMaterial(0));

	FTransform Line;
	TestTrue(TEXT("first line transform"), Grid->GetInstanceTransform(0, Line, /*bWorldSpace=*/false));
	TestTrue(
		TEXT("line length is one side"),
		FMath::IsNearlyEqual(Line.GetScale3D().X * 100.f, SolidStrongholdGrid::SideCm(), 0.1f));
	TestTrue(
		TEXT("line sits on the floor"),
		Line.GetLocation().Z > 0.f && Line.GetLocation().Z < 10.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
