#include "SolidDungeonInterior.h"
#include "Strongholds/Furnishings/SolidArrangeSign.h"
#include "Strongholds/Furnishings/Actors/SolidFurnishing.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "SolidDungeonLevel.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Strongholds/SolidStronghold.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

namespace
{
	void PlaceInteriorPart(
		UStaticMeshComponent* Part,
		UStaticMesh* Cube,
		AActor* Outer,
		const FVector& Location,
		const FVector& SizeCm,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		if (!Part || !Cube)
		{
			return;
		}

		constexpr float ShapeSize = 100.f;
		Part->SetStaticMesh(Cube);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(FRotator::ZeroRotator);
		Part->SetRelativeScale3D(SizeCm / ShapeSize);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Outer, Color, DebugName, 0.95f))
		{
			Part->SetMaterial(0, Material);
		}
	}

	void DestroyOwnedFurnishings(AActor* Owner)
	{
		UWorld* World = Owner ? Owner->GetWorld() : nullptr;
		if (!World)
		{
			return;
		}

		TArray<ASolidFurnishing*> Owned;
		for (TActorIterator<ASolidFurnishing> It(World); It; ++It)
		{
			ASolidFurnishing* Piece = *It;
			if (Piece && Piece->GetOwner() == Owner && !Piece->IsActorBeingDestroyed())
			{
				Owned.Add(Piece);
			}
		}
		for (ASolidFurnishing* Piece : Owned)
		{
			Piece->Destroy();
		}
	}

	bool SameFurnishing(const SolidStrongholdFurniture::FUsed& A, const SolidStrongholdFurniture::FUsed& B)
	{
		return A.ItemIndex == B.ItemIndex
			&& A.Level == B.Level
			&& A.X == B.X
			&& A.Y == B.Y
			&& FMath::IsNearlyEqual(A.RotationDeg, B.RotationDeg, 0.01f);
	}

	bool FurnishingsMatch(
		const TArray<ASolidFurnishing*>& Owned,
		const TArray<SolidStrongholdFurniture::FUsed>& Used)
	{
		if (Owned.Num() != Used.Num())
		{
			return false;
		}
		TArray<int32> Taken;
		for (const SolidStrongholdFurniture::FUsed& Want : Used)
		{
			int32 Found = INDEX_NONE;
			for (int32 Index = 0; Index < Owned.Num(); ++Index)
			{
				if (Taken.Contains(Index) || !Owned[Index] || !Owned[Index]->HasPlacement())
				{
					continue;
				}
				if (SameFurnishing(Owned[Index]->GetPlacement(), Want))
				{
					Found = Index;
					break;
				}
			}
			if (Found == INDEX_NONE)
			{
				return false;
			}
			Taken.Add(Found);
		}
		return true;
	}

	struct FEdgeCut
	{
		float T = 0.f;
		FVector2D Point = FVector2D::ZeroVector;

		FEdgeCut() = default;
		FEdgeCut(const float InT, const FVector2D& InPoint)
			: T(InT)
			, Point(InPoint)
		{
		}
	};

	/** Rock cube. The inner face sits on the segment, and the shell stands outside it. */
	void AddRockWall(
		UInstancedStaticMeshComponent* Walls,
		const FVector2D& A,
		const FVector2D& B,
		const FVector2D& InteriorPoint,
		const float LengthExtra,
		const float Shell,
		const float Height,
		const float ShapeSize)
	{
		if (!Walls)
		{
			return;
		}
		const FVector2D Delta = B - A;
		const float Length = Delta.Size();
		if (Length < 1.f)
		{
			return;
		}
		const FVector2D Mid = (A + B) * 0.5f;
		const FVector2D Dir = Delta / Length;
		const FVector2D Left(-Dir.Y, Dir.X);
		const FVector2D ToInterior = InteriorPoint - Mid;
		const FVector2D Inward = FVector2D::DotProduct(Left, ToInterior) >= 0.f ? Left : -Left;
		const FVector2D Outward = -Inward;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		const FTransform Wall(
			FRotator(0.f, Yaw, 0.f),
			FVector(Mid.X, Mid.Y, Height * 0.5f) + FVector(Outward.X, Outward.Y, 0.f) * (Shell * 0.5f),
			FVector((Length + LengthExtra) / ShapeSize, Shell / ShapeSize, Height / ShapeSize));
		Walls->AddInstance(Wall);
	}

	/**
	 * Drops the part of an entrance edge that crosses the doorway, and keeps the rest.
	 * A return runs from the cut out to the door plane so the shell stays closed.
	 * Edges that do not meet the door plane are kept whole.
	 */
	void ClipEntranceEdge(
		const FVector2D& A,
		const FVector2D& B,
		const float MouthY,
		const float HalfWidth,
		const float DoorX,
		TArray<SolidStrongholdGrid::FSegment>& Kept,
		TArray<SolidStrongholdGrid::FSegment>& Returns)
	{
		const float MinX = FMath::Min(A.X, B.X);
		if (MinX > DoorX + 4.f)
		{
			SolidStrongholdGrid::FSegment Whole;
			Whole.A = A;
			Whole.B = B;
			Kept.Add(Whole);
			return;
		}

		TArray<FEdgeCut> Cuts;
		Cuts.Emplace(0.f, A);
		Cuts.Emplace(1.f, B);
		const FVector2D Delta = B - A;
		if (FMath::Abs(Delta.Y) > 0.01f)
		{
			const float Lines[2] = { MouthY - HalfWidth, MouthY + HalfWidth };
			for (const float LineY : Lines)
			{
				const float T = (LineY - A.Y) / Delta.Y;
				if (T > 0.001f && T < 0.999f)
				{
					Cuts.Emplace(T, A + Delta * T);
				}
			}
		}
		for (int32 Index = 1; Index < Cuts.Num(); ++Index)
		{
			FEdgeCut Key = Cuts[Index];
			int32 Slot = Index;
			while (Slot > 0 && Cuts[Slot - 1].T > Key.T)
			{
				Cuts[Slot] = Cuts[Slot - 1];
				--Slot;
			}
			Cuts[Slot] = Key;
		}

		bool bDropped = false;
		TArray<FVector2D> CutPoints;
		for (int32 Index = 0; Index + 1 < Cuts.Num(); ++Index)
		{
			const FVector2D P = Cuts[Index].Point;
			const FVector2D Q = Cuts[Index + 1].Point;
			if (FVector2D::Distance(P, Q) < 1.f)
			{
				continue;
			}
			const FVector2D Mid = (P + Q) * 0.5f;
			const bool bInDoor = FMath::Abs(Mid.Y - MouthY) < HalfWidth - 0.5f;
			if (bInDoor)
			{
				bDropped = true;
				if (Cuts[Index].T > 0.001f && Cuts[Index].T < 0.999f)
				{
					CutPoints.Add(P);
				}
				if (Cuts[Index + 1].T > 0.001f && Cuts[Index + 1].T < 0.999f)
				{
					CutPoints.Add(Q);
				}
			}
			else
			{
				SolidStrongholdGrid::FSegment Piece;
				Piece.A = P;
				Piece.B = Q;
				Kept.Add(Piece);
			}
		}

		if (!bDropped)
		{
			return;
		}

		if (CutPoints.Num() == 0)
		{
			const FVector2D Far = A.X >= B.X ? A : B;
			const float Sign = FMath::Sign(Far.Y - MouthY);
			if (FMath::Abs(Sign) < 0.5f)
			{
				return;
			}
			SolidStrongholdGrid::FSegment Return;
			Return.A = Far;
			Return.B = FVector2D(DoorX, MouthY + Sign * HalfWidth);
			if (FVector2D::Distance(Return.A, Return.B) > 1.f)
			{
				Returns.Add(Return);
			}
			return;
		}

		for (const FVector2D& Cut : CutPoints)
		{
			SolidStrongholdGrid::FSegment Return;
			Return.A = Cut;
			Return.B = FVector2D(DoorX, Cut.Y);
			if (FVector2D::Distance(Return.A, Return.B) > 1.f)
			{
				Returns.Add(Return);
			}
		}
	}

	/**
	 * Hex-slab mesh. Z0 is the bottom, Z1 the top, in component space.
	 * Top faces are wound the way the fireplace hearth shows from above.
	 */
	UStaticMesh* BuildCaveSlab(
		UObject* Outer,
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const TArray<SolidStrongholdGrid::FSegment>& Boundary,
		const float Z0,
		const float Z1)
	{
		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<FVector> Tangents;
		TArray<FVector2D> UVs;
		TArray<int32> Triangles;

		auto AddTri = [&](const FVector& A, const FVector& B, const FVector& C)
		{
			FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
			if (Normal.IsNearlyZero())
			{
				Normal = FVector::UpVector;
			}
			FVector Tangent = FVector::CrossProduct(Normal, FVector::UpVector).GetSafeNormal();
			if (Tangent.IsNearlyZero())
			{
				Tangent = FVector::ForwardVector;
			}
			const int32 Base = Positions.Num();
			Positions.Add(A);
			Positions.Add(B);
			Positions.Add(C);
			Normals.Add(Normal);
			Normals.Add(Normal);
			Normals.Add(Normal);
			Tangents.Add(Tangent);
			Tangents.Add(Tangent);
			Tangents.Add(Tangent);
			UVs.Add(FVector2D(0.f, 0.f));
			UVs.Add(FVector2D(1.f, 0.f));
			UVs.Add(FVector2D(0.5f, 1.f));
			Triangles.Add(Base);
			Triangles.Add(Base + 1);
			Triangles.Add(Base + 2);
		};

		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			FVector2D Vertices[6];
			SolidStrongholdGrid::HexVertices(Hex.Center, Vertices);
			const FVector CenterTop(Hex.Center.X, Hex.Center.Y, Z1);
			const FVector CenterBottom(Hex.Center.X, Hex.Center.Y, Z0);
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const int32 Prev = (Index + 5) % 6;
				const FVector Top(Vertices[Index].X, Vertices[Index].Y, Z1);
				const FVector TopPrev(Vertices[Prev].X, Vertices[Prev].Y, Z1);
				const FVector Bottom(Vertices[Index].X, Vertices[Index].Y, Z0);
				const FVector BottomPrev(Vertices[Prev].X, Vertices[Prev].Y, Z0);
				AddTri(CenterTop, Top, TopPrev);
				AddTri(CenterBottom, BottomPrev, Bottom);
			}
		}

		for (const SolidStrongholdGrid::FSegment& Edge : Boundary)
		{
			const FVector BottomA(Edge.A.X, Edge.A.Y, Z0);
			const FVector BottomB(Edge.B.X, Edge.B.Y, Z0);
			const FVector TopA(Edge.A.X, Edge.A.Y, Z1);
			const FVector TopB(Edge.B.X, Edge.B.Y, Z1);
			AddTri(BottomA, TopA, TopB);
			AddTri(BottomA, TopB, BottomB);
		}

		FMeshDescription MeshDescription;
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();

		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> InstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TPolygonGroupAttributesRef<FName> PolygonGroupNames = Attributes.GetPolygonGroupMaterialSlotNames();
		TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
		InstanceUVs.SetNumChannels(1);

		const FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
		const FName SlotName(TEXT("Cave"));
		PolygonGroupNames[PolygonGroupID] = SlotName;

		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(Positions.Num());
		for (const FVector& Position : Positions)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = FVector3f(Position);
			VertexIDs.Add(VertexID);
		}

		const int32 TriCount = Triangles.Num() / 3;
		for (int32 TriIndex = 0; TriIndex < TriCount; ++TriIndex)
		{
			TArray<FVertexInstanceID, TInlineAllocator<3>> InstanceIDs;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 VertIndex = Triangles[TriIndex * 3 + Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[VertIndex]);
				InstanceNormals[InstanceID] = FVector3f(Normals[VertIndex]);
				InstanceTangents[InstanceID] = FVector3f(Tangents[VertIndex]);
				InstanceBinormalSigns[InstanceID] = 1.f;
				InstanceUVs.Set(InstanceID, 0, FVector2f(UVs[VertIndex]));
				InstanceIDs.Add(InstanceID);
			}
			MeshDescription.CreatePolygon(PolygonGroupID, InstanceIDs);
		}

		UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
		Mesh->bAllowCPUAccess = true;
		Mesh->NeverStream = true;
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}
		Mesh->SetStaticMaterials({ FStaticMaterial(nullptr, SlotName, SlotName) });

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = true;
		BuildParams.bFastBuild = true;
		BuildParams.bAllowCpuAccess = false;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: failed to build the cave slab."));
			return nullptr;
		}
		return Mesh;
	}
}

ASolidDungeonInterior::ASolidDungeonInterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	CeilingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CeilingMesh"));
	WallShell = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallShell"));
	FloorWalk = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorWalk"));
	MouthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MouthMesh"));
	RoomLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RoomLight"));
	FloorGrid = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorGrid"));
	ArrangePlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrangePlate"));
	ArrangeLetters = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArrangeLetters"));

	FloorMesh->SetupAttachment(SceneRoot);
	CeilingMesh->SetupAttachment(SceneRoot);
	MouthMesh->SetupAttachment(SceneRoot);
	UStaticMeshComponent* SolidParts[] = { FloorMesh.Get(), CeilingMesh.Get(), MouthMesh.Get() };
	for (UStaticMeshComponent* Part : SolidParts)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
	}
	FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WallShell->SetupAttachment(SceneRoot);
	WallShell->SetMobility(EComponentMobility::Movable);
	WallShell->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallShell->SetCollisionResponseToAllChannels(ECR_Block);
	WallShell->SetCastShadow(true);

	FloorWalk->SetupAttachment(SceneRoot);
	FloorWalk->SetMobility(EComponentMobility::Movable);
	FloorWalk->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FloorWalk->SetCollisionResponseToAllChannels(ECR_Block);
	FloorWalk->SetVisibility(false);
	FloorWalk->SetHiddenInGame(true);
	FloorWalk->SetCastShadow(false);

	FloorGrid->SetupAttachment(SceneRoot);
	FloorGrid->SetMobility(EComponentMobility::Movable);
	FloorGrid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloorGrid->SetCastShadow(false);

	ArrangePlate->SetupAttachment(SceneRoot);
	ArrangePlate->SetMobility(EComponentMobility::Movable);
	ArrangePlate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ArrangePlate->SetCollisionResponseToAllChannels(ECR_Ignore);
	ArrangePlate->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ArrangePlate->SetCastShadow(false);

	ArrangeLetters->SetupAttachment(SceneRoot);
	ArrangeLetters->SetMobility(EComponentMobility::Movable);
	ArrangeLetters->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrangeLetters->SetCastShadow(false);

	RoomLight->SetupAttachment(SceneRoot);
	RoomLight->SetMobility(EComponentMobility::Movable);
	RoomLight->SetCastShadows(true);
	RoomLight->SetIntensity(6000.f);
	RoomLight->SetAttenuationRadius(2200.f);
	RoomLight->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
}

void ASolidDungeonInterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidDungeonInterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidDungeonInterior::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyOwnedFurnishings(this);
	Super::EndPlay(EndPlayReason);
}

int32 ASolidDungeonInterior::ArrangeFloorCount() const
{
	return 1;
}

bool ASolidDungeonInterior::CollectArrangeFloor(
	const int32 Floor,
	TArray<SolidStrongholdGrid::FHex>& OutCells) const
{
	if (Floor != 0)
	{
		OutCells.Reset();
		return false;
	}
	SolidDungeonLevel::CollectMeagreHexes(OutCells);
	return true;
}

bool ASolidDungeonInterior::ArrangeFloorZ(const int32 Floor, float& OutZ) const
{
	if (Floor != 0)
	{
		OutZ = 0.f;
		return false;
	}
	OutZ = 0.f;
	return true;
}

bool ASolidDungeonInterior::ArrangeEntrance(const int32 Floor, int32& OutX, int32& OutY) const
{
	if (Floor != 0)
	{
		OutX = 0;
		OutY = 0;
		return false;
	}
	const SolidStrongholdGrid::FCell Entrance = SolidDungeonLevel::MeagreEntranceCell();
	OutX = Entrance.Col;
	OutY = Entrance.Row;
	return true;
}

void ASolidDungeonInterior::BuildVisuals()
{
	HideArrangeHighlight();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: missing Engine BasicShapes Cube."));
		return;
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	TArray<SolidStrongholdGrid::FSegment> Segments;
	TArray<SolidStrongholdGrid::FSegment> Boundary;
	SolidDungeonLevel::CollectMeagreHexes(Hexes);
	SolidStrongholdGrid::CollectSegments(Hexes, Segments);
	SolidStrongholdGrid::CollectBoundarySegments(Hexes, Boundary);

	const float Height = SolidDungeonLevel::RoomHeightCm;
	const float Shell = SolidDungeonLevel::ShellCm;
	const float MouthHeight = SolidDungeonLevel::MouthHeightCm;
	const FLinearColor Rock = SolidStrongholds::RockColor();
	const FVector2D MouthMid = SolidDungeonLevel::MeagreMouthMidpoint();

	if (UStaticMesh* FloorSlab = BuildCaveSlab(this, Hexes, Boundary, -Shell, 0.f))
	{
		FloorMesh->SetStaticMesh(FloorSlab);
		FloorMesh->SetRelativeLocation(FVector::ZeroVector);
		FloorMesh->SetRelativeRotation(FRotator::ZeroRotator);
		FloorMesh->SetRelativeScale3D(FVector::OneVector);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Rock, TEXT("CaveFloor"), 0.95f))
		{
			FloorMesh->SetMaterial(0, Material);
		}
	}
	if (UStaticMesh* CeilingSlab = BuildCaveSlab(this, Hexes, Boundary, Height, Height + Shell))
	{
		CeilingMesh->SetStaticMesh(CeilingSlab);
		CeilingMesh->SetRelativeLocation(FVector::ZeroVector);
		CeilingMesh->SetRelativeRotation(FRotator::ZeroRotator);
		CeilingMesh->SetRelativeScale3D(FVector::OneVector);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Rock, TEXT("CaveCeiling"), 0.95f))
		{
			CeilingMesh->SetMaterial(0, Material);
		}
	}

	if (FloorWalk)
	{
		FloorWalk->SetStaticMesh(Cube);
		FloorWalk->ClearInstances();
		FloorWalk->SetVisibility(false);
		FloorWalk->SetHiddenInGame(true);
		const float ShapeSize = 100.f;
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			const FTransform Step(
				FRotator::ZeroRotator,
				FVector(Hex.Center.X, Hex.Center.Y, -Shell * 0.5f),
				FVector(
					SolidStrongholdGrid::FlatToFlatCm / ShapeSize,
					SolidStrongholdGrid::VertexToVertexCm() / ShapeSize,
					Shell / ShapeSize));
			FloorWalk->AddInstance(Step);
		}
	}

	FVector2D Centroid = FVector2D::ZeroVector;
	if (Hexes.Num() > 0)
	{
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			Centroid += Hex.Center;
		}
		Centroid /= static_cast<float>(Hexes.Num());
	}
	if (RoomLight)
	{
		RoomLight->SetRelativeLocation(FVector(Centroid.X, Centroid.Y, Height * 0.62f));
		RoomLight->SetAttenuationRadius(900.f);
		RoomLight->SetIntensity(4500.f);
	}

	if (WallShell)
	{
		WallShell->SetStaticMesh(Cube);
		WallShell->ClearInstances();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Rock, TEXT("CaveWall"), 0.95f))
		{
			WallShell->SetMaterial(0, Material);
		}
	}

	const float ShapeSize = 100.f;
	const float DoorHalf = SolidDungeonLevel::DoorwayWidthCm * 0.5f;
	const float LipThickness = 10.f;
	const float CornerOverlap = Shell * 0.35f;
	const SolidStrongholdGrid::FCell EntranceCell = SolidDungeonLevel::MeagreEntranceCell();
	const FVector2D EntranceCenter = SolidStrongholdGrid::HexCenter(
		EntranceCell.Col, EntranceCell.Row, SolidDungeonLevel::MeagreOrigin());
	bool bPlacedMouth = false;
	for (const SolidStrongholdGrid::FSegment& Edge : Boundary)
	{
		const FVector2D Delta = Edge.B - Edge.A;
		const float Length = Delta.Size();
		if (Length < 1.f)
		{
			continue;
		}
		const FVector2D Mid2((Edge.A.X + Edge.B.X) * 0.5f, (Edge.A.Y + Edge.B.Y) * 0.5f);
		const SolidStrongholdGrid::FHex* EdgeHex = nullptr;
		float Best = TNumericLimits<float>::Max();
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			const float Dist = FVector2D::Distance(Hex.Center, Mid2);
			if (Dist < Best)
			{
				Best = Dist;
				EdgeHex = &Hex;
			}
		}
		const FVector2D Interior = EdgeHex ? EdgeHex->Center : EntranceCenter;
		const FVector2D Outward = (Mid2 - Interior).GetSafeNormal();
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		const bool bMouth = Mid2.Equals(MouthMid, 2.f);
		const bool bEntranceEdge = EdgeHex
			&& EdgeHex->Col == EntranceCell.Col
			&& EdgeHex->Row == EntranceCell.Row;
		if (bMouth)
		{
			// Thin lip outside the flat. The clear width is the doorway, not one hex edge.
			const FVector LipCenter = FVector(Mid2.X, Mid2.Y, MouthHeight * 0.5f)
				+ FVector(Outward.X, Outward.Y, 0.f) * (LipThickness * 0.5f);
			PlaceInteriorPart(
				MouthMesh,
				Cube,
				this,
				LipCenter,
				FVector(SolidDungeonLevel::DoorwayWidthCm, LipThickness, MouthHeight),
				SolidStrongholds::MouthColor(),
				TEXT("CaveInteriorMouth"));
			if (MouthMesh)
			{
				MouthMesh->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
				MouthMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				MouthMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			}
			if (WallShell)
			{
				const FVector2D Across(-Outward.Y, Outward.X);
				const FVector2D Side = Across.Y >= 0.f ? Across : -Across;
				const float SideYaw = FMath::RadiansToDegrees(FMath::Atan2(Side.Y, Side.X));
				const float Header = FMath::Max(Height - MouthHeight, 1.f);
				const FVector FrameOut(Outward.X, Outward.Y, 0.f);
				const FTransform Lintel(
					FRotator(0.f, SideYaw, 0.f),
					FVector(Mid2.X, Mid2.Y, MouthHeight + Header * 0.5f) + FrameOut * (Shell * 0.5f),
					FVector(SolidDungeonLevel::DoorwayWidthCm / ShapeSize, Shell / ShapeSize, Header / ShapeSize));
				WallShell->AddInstance(Lintel);
				const float JambSigns[] = { 1.f, -1.f };
				for (const float Sign : JambSigns)
				{
					const FVector2D JambMid = Mid2
						+ Outward * (Shell * 0.5f)
						+ Side * (Sign * (DoorHalf + Shell * 0.5f));
					const FTransform Jamb(
						FRotator(0.f, SideYaw, 0.f),
						FVector(JambMid.X, JambMid.Y, Height * 0.5f),
						FVector(Shell / ShapeSize, Shell / ShapeSize, Height / ShapeSize));
					WallShell->AddInstance(Jamb);
				}
			}
			bPlacedMouth = true;
			continue;
		}

		TArray<SolidStrongholdGrid::FSegment> Pieces;
		TArray<SolidStrongholdGrid::FSegment> Returns;
		if (bEntranceEdge)
		{
			ClipEntranceEdge(Edge.A, Edge.B, MouthMid.Y, DoorHalf, MouthMid.X, Pieces, Returns);
		}
		else
		{
			Pieces.Add(Edge);
		}

		// A clipped entrance edge stays short so it does not close the doorway.
		// Edges that never meet the door still overlap their corners.
		const float Overlap = (bEntranceEdge && Returns.Num() > 0) ? 0.f : CornerOverlap;
		for (const SolidStrongholdGrid::FSegment& Piece : Pieces)
		{
			AddRockWall(WallShell, Piece.A, Piece.B, Interior, Overlap, Shell, Height, ShapeSize);
		}
		for (const SolidStrongholdGrid::FSegment& Return : Returns)
		{
			AddRockWall(WallShell, Return.A, Return.B, EntranceCenter, 0.f, Shell, Height, ShapeSize);
		}
	}
	if (!bPlacedMouth)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: entrance edge is missing from the outline."));
	}

	if (FloorGrid)
	{
		FloorGrid->SetStaticMesh(Cube);
		FloorGrid->ClearInstances();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			this, SolidStrongholdGrid::LineColor(), TEXT("DungeonHexGrid"), 0.45f))
		{
			FloorGrid->SetMaterial(0, Material);
		}

		const float LineZ = SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm * 0.5f;
		for (const SolidStrongholdGrid::FSegment& Segment : Segments)
		{
			const FVector2D Delta = Segment.B - Segment.A;
			const float Length = Delta.Size();
			if (Length < 1.f)
			{
				continue;
			}
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			const FVector Mid((Segment.A.X + Segment.B.X) * 0.5f, (Segment.A.Y + Segment.B.Y) * 0.5f, LineZ);
			const FTransform Line(
				FRotator(0.f, Yaw, 0.f),
				Mid,
				FVector(
					Length / ShapeSize,
					SolidStrongholdGrid::LineThicknessCm / ShapeSize,
					SolidStrongholdGrid::LineHeightCm / ShapeSize));
			FloorGrid->AddInstance(Line);
		}
	}

	PlaceStarterFurnishings();

	const FSolidArrangeSign Sign = FSolidArrangeSign::ForCave();
	if (ArrangeLetters)
	{
		ArrangeLetters->SetStaticMesh(Cube);
		ArrangeLetters->ClearInstances();
		if (UMaterialInterface* Ink = SolidMaterials::CreateSolidColor(
			this, FSolidArrangeSign::TextColor(), TEXT("ArrangeLetters"), 0.9f))
		{
			ArrangeLetters->SetMaterial(0, Ink);
		}
		Sign.AppendLetters(ArrangeLetters, 100.f);
	}
	if (ArrangePlate)
	{
		Sign.ApplyPlate(ArrangePlate, Cube, 100.f);
	}
}

void ASolidDungeonInterior::SetFurnishings(const TArray<SolidStrongholdFurniture::FUsed>& Used)
{
	FurnishingsToPlace = Used;
	bUseFurnishings = true;
}

void ASolidDungeonInterior::PlaceStarterFurnishings()
{
	UWorld* World = GetWorld();
	// OnConstruction runs before the actor is initialized. Spawning then re-enters
	// construction, so the pieces are placed from the post-spawn BuildVisuals and BeginPlay.
	if (!World || !World->IsGameWorld() || World->bIsTearingDown || !IsActorInitialized())
	{
		return;
	}

	TArray<SolidStrongholdFurniture::FUsed> Used;
	if (bUseFurnishings)
	{
		Used = FurnishingsToPlace;
	}
	else
	{
		Used = SolidStrongholdFurniture::StartingFurnishings();
	}

	TArray<ASolidFurnishing*> Owned;
	for (TActorIterator<ASolidFurnishing> It(World); It; ++It)
	{
		ASolidFurnishing* Piece = *It;
		if (Piece && Piece->GetOwner() == this && !Piece->IsActorBeingDestroyed())
		{
			Owned.Add(Piece);
		}
	}
	if (FurnishingsMatch(Owned, Used))
	{
		return;
	}
	for (ASolidFurnishing* Piece : Owned)
	{
		Piece->Destroy();
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidDungeonLevel::CollectMeagreHexes(Hexes);

	int32 Placed = 0;
	for (const SolidStrongholdFurniture::FUsed& UsedPiece : Used)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(UsedPiece.ItemIndex);
		if (!Item || UsedPiece.Level != 0)
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: furnishing %d is not a level-0 catalog piece."),
				UsedPiece.ItemIndex);
			continue;
		}

		const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(Hexes, UsedPiece.X, UsedPiece.Y);
		const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Hexes, UsedPiece);
		if (!Anchor || !Span)
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: furnishing hex (%d,%d) is not on the floor."),
				UsedPiece.X, UsedPiece.Y);
			continue;
		}

		const FVector2D Mid = (Anchor->Center + Span->Center) * 0.5f;
		const FTransform ActorXf = GetActorTransform();
		const FVector WorldPos = ActorXf.TransformPosition(FVector(Mid.X, Mid.Y, 0.f));
		const FRotator WorldRot = ActorXf.TransformRotation(
			FRotator(0.f, UsedPiece.RotationDeg, 0.f).Quaternion()).Rotator();
		const FTransform SpawnXf(WorldRot, WorldPos);

		ASolidFurnishing* Piece = World->SpawnActorDeferred<ASolidFurnishing>(
			SolidStrongholdFurniture::ActorClass(Item->Kind),
			SpawnXf,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Piece)
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: failed to spawn %s."), Item->Name);
			continue;
		}

		Piece->FinishSpawning(SpawnXf);
		Piece->SetPlacement(UsedPiece);
		Piece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		++Placed;
	}

	UE_LOG(LogSolid, Warning, TEXT("Dungeon furnishings placed: %d."), Placed);
}

FVector ASolidDungeonInterior::GetCaptainSpawnWorld() const
{
	return GetActorTransform().TransformPosition(SolidDungeonLevel::CaptainSpawnLocal());
}

FRotator ASolidDungeonInterior::GetCaptainView() const
{
	const SolidStronghold::FVisitView View = SolidStronghold::VisitViewFor(
		FSolidStronghold(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre));
	return FRotator(View.Enter.PitchDeg, View.Enter.YawDeg, 0.f);
}

float ASolidDungeonInterior::GetCaptainYaw() const
{
	return GetCaptainView().Yaw;
}
