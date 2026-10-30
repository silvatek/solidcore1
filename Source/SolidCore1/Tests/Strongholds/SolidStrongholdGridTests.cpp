#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Strongholds/Dungeon/SolidDungeonInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
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

	const FLinearColor Color = SolidStrongholdGrid::LineColor();
	TestTrue(
		TEXT("the line color stays"),
		FMath::IsNearlyEqual(Color.R, 1.f, 0.001f)
			&& FMath::IsNearlyEqual(Color.G, 0.86f, 0.001f)
			&& FMath::IsNearlyEqual(Color.B, 0.08f, 0.001f));
	TestTrue(TEXT("lines are thin"), SolidStrongholdGrid::LineThicknessCm < 15.f);

	const FVector2D Origin = FVector2D::ZeroVector;
	const FVector2D East = SolidStrongholdGrid::HexCenter(1, 0, Origin);
	const FVector2D North = SolidStrongholdGrid::HexCenter(0, 1, Origin);
	TestTrue(
		TEXT("east neighbor is one flat width away"),
		FMath::IsNearlyEqual(FVector2D::Distance(Origin, East), SolidStrongholdGrid::FlatToFlatCm, 0.05f));
	TestTrue(
		TEXT("diagonal neighbor shares an edge"),
		FMath::IsNearlyEqual(FVector2D::Distance(Origin, North), SolidStrongholdGrid::FlatToFlatCm, 0.05f));
	TestTrue(TEXT("a cell center is inside its hex"), SolidStrongholdGrid::PointInHex(Origin, Origin));

	SolidStrongholdGrid::FHex Left;
	Left.Center = Origin;
	Left.Col = 0;
	Left.Row = 0;
	SolidStrongholdGrid::FHex Right;
	Right.Center = East;
	Right.Col = 1;
	Right.Row = 0;
	const TArray<SolidStrongholdGrid::FHex> Pair = { Left, Right };

	TArray<SolidStrongholdGrid::FSegment> Segments;
	SolidStrongholdGrid::CollectSegments(Pair, Segments);
	TestEqual(TEXT("a shared edge is drawn once"), Segments.Num(), 11);

	TArray<SolidStrongholdGrid::FSegment> Boundary;
	SolidStrongholdGrid::CollectBoundarySegments(Pair, Boundary);
	TestEqual(TEXT("the shared edge is not an outline"), Boundary.Num(), 10);

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
	SolidDungeonLevel::CollectMeagreHexes(Hexes);
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
	TestNotNull(TEXT("grid material"), Grid->GetMaterial(0));

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
