#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonGrid.h"
#include "SolidDungeonInterior.h"
#include "Components/InstancedStaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonGridMeasureTest,
	"SolidCore1.Stronghold.DungeonHexGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonGridMeasureTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("edge to edge is 1.2 m"),
		FMath::IsNearlyEqual(SolidDungeonGrid::FlatToFlatCm, 120.f, 0.01f));
	TestTrue(
		TEXT("point to point is longer"),
		SolidDungeonGrid::VertexToVertexCm() > SolidDungeonGrid::FlatToFlatCm + 10.f);
	TestTrue(
		TEXT("a side matches the point radius"),
		FMath::IsNearlyEqual(SolidDungeonGrid::SideCm(), SolidDungeonGrid::VertexToVertexCm() * 0.5f, 0.01f));

	const FLinearColor Yellow = SolidDungeonGrid::LineColor();
	TestTrue(TEXT("lines are yellow"), Yellow.R > 0.9f && Yellow.G > 0.7f && Yellow.B < 0.2f);
	TestTrue(TEXT("lines are thin"), SolidDungeonGrid::LineThicknessCm < 15.f);

	const FBox2D Floor = SolidDungeonGrid::MeagreFloorBounds();
	const FVector2D Origin = Floor.GetCenter();
	const FVector2D East = SolidDungeonGrid::HexCenter(1, 0, Origin);
	const FVector2D North = SolidDungeonGrid::HexCenter(0, 1, Origin);
	TestTrue(
		TEXT("east neighbor is one flat width away"),
		FMath::IsNearlyEqual(FVector2D::Distance(Origin, East), SolidDungeonGrid::FlatToFlatCm, 0.05f));
	TestTrue(
		TEXT("diagonal neighbor shares an edge"),
		FMath::IsNearlyEqual(FVector2D::Distance(Origin, North), SolidDungeonGrid::FlatToFlatCm, 0.05f));

	TArray<SolidDungeonGrid::FHex> Hexes;
	SolidDungeonGrid::CollectHexes(Floor, Hexes);
	TestTrue(TEXT("the floor holds several cells"), Hexes.Num() > 20);

	bool bHasCenter = false;
	for (const SolidDungeonGrid::FHex& Hex : Hexes)
	{
		if (Hex.Center.Equals(Origin, 0.05f))
		{
			bHasCenter = true;
		}
		TestTrue(TEXT("cell sits fully on the floor"), SolidDungeonGrid::HexFitsOnFloor(Floor, Hex.Center));
	}
	TestTrue(TEXT("a cell is centered in the room"), bHasCenter);

	TArray<SolidDungeonGrid::FSegment> Segments;
	SolidDungeonGrid::CollectSegments(Hexes, Segments);
	TestTrue(TEXT("shared edges are drawn once"), Segments.Num() < Hexes.Num() * 6);
	TestTrue(TEXT("the grid has edges"), Segments.Num() > Hexes.Num());

	for (const SolidDungeonGrid::FSegment& Segment : Segments)
	{
		TestTrue(
			TEXT("each edge is one side long"),
			FMath::IsNearlyEqual(FVector2D::Distance(Segment.A, Segment.B), SolidDungeonGrid::SideCm(), 0.05f));
		auto OnFloor = [&Floor](const FVector2D& Point)
		{
			return Point.X >= Floor.Min.X - 0.1f && Point.X <= Floor.Max.X + 0.1f
				&& Point.Y >= Floor.Min.Y - 0.1f && Point.Y <= Floor.Max.Y + 0.1f;
		};
		TestTrue(TEXT("edge starts on the floor"), OnFloor(Segment.A));
		TestTrue(TEXT("edge ends on the floor"), OnFloor(Segment.B));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonGridVisualTest,
	"SolidCore1.Stronghold.DungeonHexGridVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonGridVisualTest::RunTest(const FString& Parameters)
{
	TArray<SolidDungeonGrid::FHex> Hexes;
	TArray<SolidDungeonGrid::FSegment> Segments;
	SolidDungeonGrid::CollectHexes(SolidDungeonGrid::MeagreFloorBounds(), Hexes);
	SolidDungeonGrid::CollectSegments(Hexes, Segments);

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
		FMath::IsNearlyEqual(Line.GetScale3D().X * 100.f, SolidDungeonGrid::SideCm(), 0.1f));
	TestTrue(
		TEXT("line sits on the floor"),
		Line.GetLocation().Z > 0.f && Line.GetLocation().Z < 10.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
