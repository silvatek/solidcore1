#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonExterior.h"
#include "SolidGameState.h"
#include "SolidStrongholds.h"
#include "SolidStrongholdType.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidDungeonExteriorTestPrivate
{
	struct FBounds
	{
		FVector Min = FVector(TNumericLimits<float>::Max());
		FVector Max = FVector(-TNumericLimits<float>::Max());
		int32 Vertices = 0;
		float MidCrownZ = -1.f;
		float FrontOpeningY = 0.f;
		FVector CrownNormal = FVector::ZeroVector;
		FVector SideNormal = FVector::ZeroVector;
		float CrownWoundZ = 1.f;
	};

	bool Facing(const UStaticMeshComponent* Part, FBounds& Out)
	{
		if (!Part)
		{
			return false;
		}
		UStaticMesh* Mesh = Part->GetStaticMesh();
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return false;
		}

		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		float CrownZ = -TNumericLimits<float>::Max();
		float SideY = CrownZ;
		FVertexID CrownVertex = INDEX_NONE;
		FVertexID SideVertex = INDEX_NONE;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			if (Position.Z > CrownZ)
			{
				CrownZ = Position.Z;
				CrownVertex = VertexID;
			}
			if (Position.Y > SideY)
			{
				SideY = Position.Y;
				SideVertex = VertexID;
			}
		}
		if (CrownVertex == INDEX_NONE || SideVertex == INDEX_NONE)
		{
			return false;
		}

		auto AverageNormal = [&Description, &Normals](const FVertexID VertexID) -> FVector
		{
			FVector Sum = FVector::ZeroVector;
			for (const FVertexInstanceID InstanceID : Description->GetVertexVertexInstanceIDs(VertexID))
			{
				const FVector3f Normal = Normals[InstanceID];
				Sum += FVector(Normal.X, Normal.Y, Normal.Z);
			}
			return Sum.GetSafeNormal();
		};
		Out.CrownNormal = AverageNormal(CrownVertex);
		Out.SideNormal = AverageNormal(SideVertex);

		Out.CrownWoundZ = 1.f;
		for (const FTriangleID TriangleID : Description->Triangles().GetElementIDs())
		{
			const TArrayView<const FVertexInstanceID> Corners = Description->GetTriangleVertexInstances(TriangleID);
			if (Corners.Num() < 3)
			{
				continue;
			}
			const FVertexID Vertices[3] = {
				Description->GetVertexInstanceVertex(Corners[0]),
				Description->GetVertexInstanceVertex(Corners[1]),
				Description->GetVertexInstanceVertex(Corners[2]),
			};
			const FVector3f P0 = Positions[Vertices[0]];
			const FVector3f P1 = Positions[Vertices[1]];
			const FVector3f P2 = Positions[Vertices[2]];
			const FVector Centroid = FVector(P0 + P1 + P2) / 3.f;
			if (Centroid.Z < CrownZ - 40.f || FMath::Abs(Centroid.Y) > 80.f)
			{
				continue;
			}
			const FVector Cross = FVector::CrossProduct(
				FVector(P1 - P0),
				FVector(P2 - P0));
			Out.CrownWoundZ = FMath::Min(Out.CrownWoundZ, Cross.GetSafeNormal().Z);
		}
		return true;
	}

	bool Measure(
		const UStaticMeshComponent* Part,
		const float SlopeRun,
		const float OpeningHeight,
		FBounds& Out)
	{
		if (!Part)
		{
			return false;
		}
		UStaticMesh* Mesh = Part->GetStaticMesh();
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return false;
		}

		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const FVector Location = Part->GetRelativeLocation();
		const FVector Scale = Part->GetRelativeScale3D();
		const float MidX = -SlopeRun * 0.5f;
		const float MidBand = SlopeRun / 12.f;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Raw = Positions[VertexID];
			const FVector Point(
				Location.X + Raw.X * Scale.X,
				Location.Y + Raw.Y * Scale.Y,
				Location.Z + Raw.Z * Scale.Z);
			Out.Min.X = FMath::Min(Out.Min.X, Point.X);
			Out.Min.Y = FMath::Min(Out.Min.Y, Point.Y);
			Out.Min.Z = FMath::Min(Out.Min.Z, Point.Z);
			Out.Max.X = FMath::Max(Out.Max.X, Point.X);
			Out.Max.Y = FMath::Max(Out.Max.Y, Point.Y);
			Out.Max.Z = FMath::Max(Out.Max.Z, Point.Z);
			++Out.Vertices;
			if (FMath::Abs(Point.X - MidX) <= MidBand && FMath::Abs(Point.Y) < 40.f)
			{
				Out.MidCrownZ = FMath::Max(Out.MidCrownZ, Point.Z);
			}
			if (FMath::Abs(Point.X) < 5.f && FMath::Abs(Point.Z - OpeningHeight) < 30.f)
			{
				Out.FrontOpeningY = FMath::Max(Out.FrontOpeningY, FMath::Abs(Point.Y));
			}
		}
		return Out.Vertices > 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonExteriorTest,
	"SolidCore1.Stronghold.DungeonExterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonExteriorTest::RunTest(const FString& Parameters)
{
	FGameState State;
	FSolidStronghold* Cave = State.ActiveStronghold.Get();
	TestNotNull(TEXT("a new game has a stronghold"), Cave);
	if (!Cave)
	{
		return false;
	}
	TestEqual(
		TEXT("the cave exit faces north"),
		static_cast<uint8>(Cave->Orientation),
		static_cast<uint8>(ESolidStrongholdOrientation::North));
	TestTrue(
		TEXT("the cave names the dungeon exterior"),
		Cave->ExteriorClass() == ASolidDungeonExterior::StaticClass());
	TestTrue(
		TEXT("the cave exterior faces north"),
		FMath::IsNearlyEqual(Cave->ExteriorYawDeg(nullptr), 90.f, 0.01f));
	TestTrue(
		TEXT("north is world +Y"),
		FMath::IsNearlyEqual(SolidStronghold::OrientationYawDeg(ESolidStrongholdOrientation::North), 90.f, 0.01f));
	TestTrue(
		TEXT("east is world +X"),
		FMath::IsNearlyEqual(SolidStronghold::OrientationYawDeg(ESolidStrongholdOrientation::East), 0.f, 0.01f));
	TestTrue(
		TEXT("south is world -Y"),
		FMath::IsNearlyEqual(SolidStronghold::OrientationYawDeg(ESolidStrongholdOrientation::South), -90.f, 0.01f));
	TestTrue(
		TEXT("west is world -X"),
		FMath::IsNearlyEqual(SolidStronghold::OrientationYawDeg(ESolidStrongholdOrientation::West), 180.f, 0.01f));

	Cave->Orientation = ESolidStrongholdOrientation::East;
	TestTrue(
		TEXT("the cave exterior follows the exit"),
		FMath::IsNearlyEqual(Cave->ExteriorYawDeg(nullptr), 0.f, 0.01f));
	TUniquePtr<FSolidStronghold> Copy = Cave->Clone();
	FSolidStronghold* Cloned = Copy.Get();
	TestNotNull(TEXT("the cave clones"), Cloned);
	if (Cloned)
	{
		TestEqual(
			TEXT("a clone keeps the exit direction"),
			static_cast<uint8>(Cloned->Orientation),
			static_cast<uint8>(ESolidStrongholdOrientation::East));
	}

	const SolidStronghold::FSolidDungeon& Dungeon = SolidStronghold::FSolidDungeon::Instance();
	TestFalse(TEXT("the dungeon does not use the cliff mouth"), Dungeon.UsesCliffMouth());
	ESolidBiome Shelf = ESolidBiome::Mountain;
	TestTrue(TEXT("the cave names a shelf"), SolidStronghold::BiomeFor(*Cave, Shelf));
	TestEqual(
		TEXT("the cave shelf is grassland"),
		static_cast<uint8>(Shelf),
		static_cast<uint8>(ESolidBiome::Grassland));

	const float FaceWidth = SolidStrongholds::OpeningWidthCm + 2.f * SolidStrongholds::FrameThicknessCm;
	const float FaceHeight = SolidStrongholds::OpeningHeightCm + SolidStrongholds::FrameThicknessCm;
	TestTrue(
		TEXT("face width is the opening plus the frame"),
		FMath::IsNearlyEqual(Dungeon.FaceWidthCm(), FaceWidth, 0.01f));
	TestTrue(
		TEXT("face height is the opening plus the frame"),
		FMath::IsNearlyEqual(Dungeon.FaceHeightCm(), FaceHeight, 0.01f));
	TestTrue(
		TEXT("the slope runs twice the face height"),
		FMath::IsNearlyEqual(Dungeon.SlopeRunCm(), 2.f * Dungeon.FaceHeightCm(), 0.01f));

	const FVector2D CellCenter(1000.f, 2000.f);
	const FVector2D NorthStand = Dungeon.LeaveStandXY(CellCenter, ESolidStrongholdOrientation::North);
	TestTrue(
		TEXT("leaving stands north of the cell center"),
		FMath::IsNearlyEqual(NorthStand.X, CellCenter.X, 0.1f)
			&& FMath::IsNearlyEqual(NorthStand.Y, CellCenter.Y + Dungeon.LeaveStandCm(), 0.1f));
	const FVector2D EastStand = Dungeon.LeaveStandXY(CellCenter, ESolidStrongholdOrientation::East);
	TestTrue(
		TEXT("leaving stands east of the cell center"),
		FMath::IsNearlyEqual(EastStand.X, CellCenter.X + Dungeon.LeaveStandCm(), 0.1f)
			&& FMath::IsNearlyEqual(EastStand.Y, CellCenter.Y, 0.1f));

	ASolidDungeonExterior* Rock = NewObject<ASolidDungeonExterior>();
	TestNotNull(TEXT("rock"), Rock);
	if (!Rock)
	{
		return false;
	}
	Cave->ConfigureExterior(Rock);

	UStaticMeshComponent* Stone = Rock->GetRockMesh();
	UStaticMeshComponent* Mouth = Rock->GetMouthMesh();
	TestNotNull(TEXT("stone"), Stone);
	TestNotNull(TEXT("mouth"), Mouth);
	if (!Stone || !Mouth)
	{
		return false;
	}
	TestTrue(TEXT("the mouth is not the stone"), Stone != Mouth);

	UStaticMesh* MouthMesh = Mouth->GetStaticMesh();
	TestNotNull(TEXT("mouth cube"), MouthMesh);
	TestTrue(
		TEXT("the mouth is the opening"),
		FMath::IsNearlyEqual(Mouth->GetRelativeScale3D().Y * 100.f, SolidStrongholds::OpeningWidthCm, 0.5f)
			&& FMath::IsNearlyEqual(Mouth->GetRelativeScale3D().Z * 100.f, SolidStrongholds::OpeningHeightCm, 0.5f));
	TestTrue(
		TEXT("the mouth meets the ground"),
		FMath::IsNearlyEqual(
			Mouth->GetRelativeLocation().Z - Mouth->GetRelativeScale3D().Z * 50.f,
			0.f,
			0.5f));
	TestTrue(
		TEXT("the mouth is centered"),
		FMath::IsNearlyZero(Mouth->GetRelativeLocation().Y, 0.5f));
	const float MouthFront = Mouth->GetRelativeLocation().X + Mouth->GetRelativeScale3D().X * 50.f;
	TestTrue(
		TEXT("the mouth sits behind the front"),
		FMath::IsNearlyEqual(MouthFront, -SolidStrongholds::MouthRecessCm, 0.5f));
	TestTrue(
		TEXT("the mouth blocks a click"),
		Mouth->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
	TestTrue(
		TEXT("the stone blocks a step"),
		Stone->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);

	SolidDungeonExteriorTestPrivate::FBounds Bounds;
	TestTrue(
		TEXT("rock mesh"),
		SolidDungeonExteriorTestPrivate::Measure(Stone, Dungeon.SlopeRunCm(), SolidStrongholds::OpeningHeightCm, Bounds));
	TestTrue(TEXT("the rock is not a box"), Bounds.Vertices > 100);
	TestTrue(
		TEXT("the rock reaches the face height"),
		FMath::IsNearlyEqual(Bounds.Max.Z, Dungeon.FaceHeightCm(), 1.f));
	TestTrue(
		TEXT("the rock reaches the face width"),
		FMath::IsNearlyEqual(FMath::Max(FMath::Abs(Bounds.Min.Y), Bounds.Max.Y), Dungeon.FaceWidthCm() * 0.5f, 1.f));
	TestTrue(
		TEXT("the slope runs the full distance"),
		FMath::IsNearlyEqual(Bounds.Min.X, -Dungeon.SlopeRunCm(), 1.f)
			&& FMath::IsNearlyEqual(Bounds.Max.X, 0.f, 1.f));
	TestTrue(
		TEXT("the rock stays inside the face"),
		Bounds.Max.Z <= Dungeon.FaceHeightCm() + 1.f
			&& FMath::Max(FMath::Abs(Bounds.Min.Y), Bounds.Max.Y) <= Dungeon.FaceWidthCm() * 0.5f + 1.f
			&& Bounds.Min.Z >= -1.f);
	TestTrue(
		TEXT("the slope is rounder than a wedge"),
		Bounds.MidCrownZ > Dungeon.FaceHeightCm() * 0.65f);
	TestTrue(
		TEXT("the front is wider than a triangle"),
		Bounds.FrontOpeningY > Dungeon.FaceWidthCm() * 0.35f);

	SolidDungeonExteriorTestPrivate::FBounds Facing;
	TestTrue(TEXT("rock facing"), SolidDungeonExteriorTestPrivate::Facing(Stone, Facing));
	TestTrue(
		TEXT("the crown normal points up"),
		Facing.CrownNormal.Z > 0.5f);
	TestTrue(
		TEXT("the side normal points outward"),
		Facing.SideNormal.Y > 0.2f);
	TestTrue(
		TEXT("the crown is wound clockwise from outside"),
		Facing.CrownWoundZ < 0.f);

	return true;
}

#endif
