#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Dungeon/SolidDungeonExterior.h"
#include "Company/SolidCompany.h"
#include "SolidStronghold.h"
#include "Strongholds/Dungeon/SolidDungeon.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidWorldMap.h"
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
		const float MidX = Location.X - SlopeRun * 0.5f;
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
			if (FMath::Abs(Point.X - MidX) <= MidBand && FMath::Abs(Point.Y - Location.Y) < 40.f)
			{
				Out.MidCrownZ = FMath::Max(Out.MidCrownZ, Point.Z);
			}
			if (FMath::Abs(Point.X - Location.X) < 5.f && FMath::Abs(Point.Z - OpeningHeight) < 30.f)
			{
				Out.FrontOpeningY = FMath::Max(Out.FrontOpeningY, FMath::Abs(Point.Y - Location.Y));
			}
		}
		return Out.Vertices > 0;
	}

	float HalfWidthNear(const UStaticMeshComponent* Part, const float Z, const float Band)
	{
		if (!Part)
		{
			return 0.f;
		}
		UStaticMesh* Mesh = Part->GetStaticMesh();
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return 0.f;
		}
		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const FVector Location = Part->GetRelativeLocation();
		const FVector Scale = Part->GetRelativeScale3D();
		float Half = 0.f;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Raw = Positions[VertexID];
			const float PointZ = Location.Z + Raw.Z * Scale.Z;
			if (FMath::Abs(PointZ - Z) > Band)
			{
				continue;
			}
			const float PointY = Raw.Y * Scale.Y;
			Half = FMath::Max(Half, FMath::Abs(PointY));
		}
		return Half;
	}

	float ReachFrom(const FBounds& Bounds, const float CenterY)
	{
		return FMath::Max(FMath::Abs(Bounds.Min.Y - CenterY), FMath::Abs(Bounds.Max.Y - CenterY));
	}

	bool FrontCovers(const UStaticMeshComponent* Part, const float LocalY, const float LocalZ)
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
		for (const FTriangleID TriangleID : Description->Triangles().GetElementIDs())
		{
			const TArrayView<const FVertexInstanceID> Corners = Description->GetTriangleVertexInstances(TriangleID);
			if (Corners.Num() < 3)
			{
				continue;
			}
			const FVector3f P0 = Positions[Description->GetVertexInstanceVertex(Corners[0])];
			const FVector3f P1 = Positions[Description->GetVertexInstanceVertex(Corners[1])];
			const FVector3f P2 = Positions[Description->GetVertexInstanceVertex(Corners[2])];
			if (FMath::Abs(P0.X) > 1.f || FMath::Abs(P1.X) > 1.f || FMath::Abs(P2.X) > 1.f)
			{
				continue;
			}
			const FVector2D A(P0.Y, P0.Z);
			const FVector2D B(P1.Y, P1.Z);
			const FVector2D C(P2.Y, P2.Z);
			const FVector2D P(LocalY, LocalZ);
			const float Den = (B.Y - C.Y) * (A.X - C.X) + (C.X - B.X) * (A.Y - C.Y);
			if (FMath::Abs(Den) < 0.01f)
			{
				continue;
			}
			const float Wa = ((B.Y - C.Y) * (P.X - C.X) + (C.X - B.X) * (P.Y - C.Y)) / Den;
			const float Wb = ((C.Y - A.Y) * (P.X - C.X) + (A.X - C.X) * (P.Y - C.Y)) / Den;
			const float Wc = 1.f - Wa - Wb;
			if (Wa >= -0.02f && Wb >= -0.02f && Wc >= -0.02f)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonExteriorTest,
	"SolidCore1.Stronghold.DungeonExterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonExteriorTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	Company->InitializeStartingStronghold();
	FSolidStronghold* Cave = Company->GetStronghold();
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

	const float NorthHalf = SolidStronghold::CellExtentAlongExitCm(ESolidStrongholdOrientation::North) * 0.5f;
	const float EastHalf = SolidStronghold::CellExtentAlongExitCm(ESolidStrongholdOrientation::East) * 0.5f;
	const float NorthForward = Dungeon.FaceForwardCm(ESolidStrongholdOrientation::North);
	const float EastForward = Dungeon.FaceForwardCm(ESolidStrongholdOrientation::East);
	TestTrue(TEXT("a north-south cell is taller"), NorthHalf > EastHalf);
	TestTrue(
		TEXT("the north face sits just inside the next cell"),
		NorthForward < NorthHalf
			&& NorthForward > NorthHalf - SolidStrongholds::MouthBorderGapCm - 1.f);
	TestTrue(
		TEXT("the east face uses the shorter cell"),
		FMath::IsNearlyEqual(EastForward, EastHalf - SolidStrongholds::MouthBorderGapCm, 0.1f));
	TestTrue(
		TEXT("the north face uses the taller cell"),
		FMath::IsNearlyEqual(NorthForward, NorthHalf - SolidStrongholds::MouthBorderGapCm, 0.1f));

	const float NorthLateral = Dungeon.ShelfLateralCm(ESolidStrongholdOrientation::North);
	const float EastLateral = Dungeon.ShelfLateralCm(ESolidStrongholdOrientation::East);
	TestTrue(
		TEXT("the north shelf center is east of the cell center"),
		FMath::IsNearlyEqual(NorthLateral, -100.f, 0.1f));
	TestTrue(
		TEXT("the east shelf center is north of the cell center"),
		FMath::IsNearlyEqual(EastLateral, 100.f, 0.1f));
	const FVector2D CellCenter(1000.f, 2000.f);
	const FVector2D NorthStand = Dungeon.LeaveStandXY(CellCenter, ESolidStrongholdOrientation::North);
	const float NorthYaw = FMath::DegreesToRadians(
		SolidStronghold::OrientationYawDeg(ESolidStrongholdOrientation::North));
	const FVector2D NorthLeft(-FMath::Sin(NorthYaw), FMath::Cos(NorthYaw));
	TestTrue(
		TEXT("leaving stands in front of the north face"),
		FMath::IsNearlyEqual(NorthStand.X, CellCenter.X + NorthLeft.X * NorthLateral, 0.1f)
			&& FMath::IsNearlyEqual(NorthStand.Y, CellCenter.Y + NorthForward + Dungeon.LeaveStandCm(), 0.1f));
	const FVector2D EastStand = Dungeon.LeaveStandXY(CellCenter, ESolidStrongholdOrientation::East);
	const float EastYaw = FMath::DegreesToRadians(
		SolidStronghold::OrientationYawDeg(ESolidStrongholdOrientation::East));
	const FVector2D EastLeft(-FMath::Sin(EastYaw), FMath::Cos(EastYaw));
	TestTrue(
		TEXT("leaving stands in front of the east face"),
		FMath::IsNearlyEqual(EastStand.X, CellCenter.X + EastForward + Dungeon.LeaveStandCm(), 0.1f)
			&& FMath::IsNearlyEqual(EastStand.Y, CellCenter.Y + EastLeft.Y * EastLateral, 0.1f));

	TUniquePtr<FSolidStronghold> Leaving = Cave->Clone();
	TestNotNull(TEXT("the cave clones"), Leaving.Get());
	if (Leaving)
	{
		Leaving->Location = CellCenter;
		Leaving->Orientation = ESolidStrongholdOrientation::North;
		const SolidStronghold::FExitStand NorthExit = SolidStronghold::ExitStand(*Leaving);
		TestTrue(TEXT("exit stands in front of the north mouth"), NorthExit.XY.Equals(NorthStand, 0.1f));
		TestTrue(TEXT("exit faces away to the north"), FMath::IsNearlyEqual(NorthExit.YawDeg, 90.f, 0.1f));
		Leaving->Orientation = ESolidStrongholdOrientation::East;
		const SolidStronghold::FExitStand EastExit = SolidStronghold::ExitStand(*Leaving);
		TestTrue(TEXT("exit stands in front of the east mouth"), EastExit.XY.Equals(EastStand, 0.1f));
		TestTrue(TEXT("exit faces away to the east"), FMath::IsNearlyEqual(EastExit.YawDeg, 0.f, 0.1f));
	}

	Cave->Orientation = ESolidStrongholdOrientation::North;
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
	TestNotNull(TEXT("mouth mesh"), MouthMesh);
	SolidDungeonExteriorTestPrivate::FBounds MouthBounds;
	TestTrue(
		TEXT("mouth shape"),
		SolidDungeonExteriorTestPrivate::Measure(Mouth, 0.f, SolidStrongholds::OpeningHeightCm, MouthBounds));
	const float ShelfY = Mouth->GetRelativeLocation().Y;
	const float MouthReach = SolidDungeonExteriorTestPrivate::ReachFrom(MouthBounds, ShelfY);
	TestTrue(
		TEXT("the mouth is the opening"),
		FMath::IsNearlyEqual(MouthBounds.Max.Z - MouthBounds.Min.Z, SolidStrongholds::OpeningHeightCm, 30.f)
			&& MouthReach > SolidStrongholds::OpeningWidthCm * 0.35f
			&& MouthReach <= SolidStrongholds::OpeningWidthCm * 0.5f + 1.f);
	TestTrue(
		TEXT("the mouth meets the ground"),
		FMath::IsNearlyEqual(MouthBounds.Min.Z, 0.f, 1.f));
	TestTrue(
		TEXT("the mouth is centered on the shelf"),
		FMath::IsNearlyEqual(ShelfY, NorthLateral, 0.1f)
			&& FMath::IsNearlyEqual((MouthBounds.Min.Y + MouthBounds.Max.Y) * 0.5f, ShelfY, 1.f));
	const float GroundHalf = SolidDungeonExteriorTestPrivate::HalfWidthNear(Mouth, 20.f, 25.f);
	const float MiddleHalf = SolidDungeonExteriorTestPrivate::HalfWidthNear(
		Mouth,
		SolidStrongholds::OpeningHeightCm * 0.45f,
		40.f);
	const float CrownHalf = SolidDungeonExteriorTestPrivate::HalfWidthNear(
		Mouth,
		SolidStrongholds::OpeningHeightCm * 0.82f,
		30.f);
	TestTrue(
		TEXT("the mouth stays wide at the ground"),
		GroundHalf > SolidStrongholds::OpeningWidthCm * 0.4f
			&& GroundHalf + 1.f >= MiddleHalf
			&& CrownHalf + 30.f < GroundHalf);
	TestTrue(
		TEXT("the mouth covers the bottom of the opening"),
		SolidDungeonExteriorTestPrivate::FrontCovers(Mouth, 0.f, 12.f));
	const float MouthFront = MouthBounds.Max.X;
	TestTrue(
		TEXT("the mouth sits behind the front"),
		FMath::IsNearlyEqual(MouthFront, NorthForward - SolidStrongholds::MouthRecessCm, 0.5f));
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
	const float RockReach = SolidDungeonExteriorTestPrivate::ReachFrom(Bounds, Stone->GetRelativeLocation().Y);
	TestTrue(
		TEXT("the rock reaches the face width"),
		FMath::IsNearlyEqual(RockReach, Dungeon.FaceWidthCm() * 0.5f, 1.f));
	TestTrue(
		TEXT("the slope runs the full distance"),
		FMath::IsNearlyEqual(Bounds.Min.X, NorthForward - Dungeon.SlopeRunCm(), 1.f)
			&& FMath::IsNearlyEqual(Bounds.Max.X, NorthForward, 1.f));
	TestTrue(
		TEXT("the rock stays inside the face"),
		Bounds.Max.Z <= Dungeon.FaceHeightCm() + 1.f
			&& RockReach <= Dungeon.FaceWidthCm() * 0.5f + 1.f
			&& Bounds.Min.Z >= -1.f);
	TestTrue(
		TEXT("the rock sits on the shelf center"),
		FMath::IsNearlyEqual(Stone->GetRelativeLocation().Y, NorthLateral, 0.1f));
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

	USolidTerrainMap* Terrain = NewObject<USolidTerrainMap>();
	TestNotNull(TEXT("terrain"), Terrain);
	if (Terrain)
	{
		Terrain->Build(1337, 0.00012f, 3000.f, 0.f, 513, 513, 200.f);
		const FVector2D CaveXY = SolidStronghold::WorldXY(SolidStronghold::StartingCell());
		const float Bias = 3.f;
		const FVector ActorLocation(CaveXY.X, CaveXY.Y, Terrain->SampleHeight(CaveXY.X, CaveXY.Y) + Bias + 4.f);
		const float Drop = ASolidDungeonExterior::GroundDropCm(Terrain, Bias, ActorLocation);
		const USolidWorldMap* Map = Terrain->GetWorldMap();
		FIntPoint Cell = FIntPoint::ZeroValue;
		float Lowest = TNumericLimits<float>::Max();
		TestTrue(
			TEXT("the cave cell is on the map"),
			Map && Map->CellAtWorld(CaveXY.X, CaveXY.Y, Terrain->GetWorldMinXY(), Terrain->GetWorldMaxXY(), Cell));
		if (Map)
		{
			for (int32 IndexY = 0; IndexY < Terrain->GridHeight; ++IndexY)
			{
				for (int32 IndexX = 0; IndexX < Terrain->GridWidth; ++IndexX)
				{
					const FSolidTerrainPoint& Point = Terrain->GetPoint(IndexX, IndexY);
					FIntPoint PointCell = FIntPoint::ZeroValue;
					if (Map->CellAtWorld(Point.X, Point.Y, Terrain->GetWorldMinXY(), Terrain->GetWorldMaxXY(), PointCell)
						&& PointCell == Cell)
					{
						Lowest = FMath::Min(Lowest, Point.Height);
					}
				}
			}
		}
		TestTrue(TEXT("the cell has a terrain point"), Lowest < TNumericLimits<float>::Max());
		TestTrue(
			TEXT("the ground edge is 10 cm under the lowest point"),
			FMath::IsNearlyEqual(ActorLocation.Z - Drop, Lowest + Bias - ASolidDungeonExterior::GroundBurialCm, 0.1f));
	}

	ASolidDungeonExterior* Buried = NewObject<ASolidDungeonExterior>();
	TestNotNull(TEXT("buried rock"), Buried);
	if (Buried)
	{
		constexpr float ForcedDrop = 40.f;
		Buried->SetGroundDrop(ForcedDrop);
		Cave->ConfigureExterior(Buried);
		SolidDungeonExteriorTestPrivate::FBounds BuriedBounds;
		TestTrue(
			TEXT("buried rock mesh"),
			Buried->GetRockMesh()
				&& SolidDungeonExteriorTestPrivate::Measure(
					Buried->GetRockMesh(),
					Dungeon.SlopeRunCm(),
					SolidStrongholds::OpeningHeightCm,
					BuriedBounds));
		TestTrue(
			TEXT("the ground edge drops by the burial"),
			FMath::IsNearlyEqual(BuriedBounds.Min.Z, -ForcedDrop, 1.f));
		TestTrue(
			TEXT("the crown stays at the face height"),
			FMath::IsNearlyEqual(BuriedBounds.Max.Z, Dungeon.FaceHeightCm(), 1.f));
	}

	return true;
}

#endif
