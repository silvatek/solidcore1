#include "SolidDungeonInterior.h"
#include "SolidFurnishing.h"
#include "SolidStrongholdFurniture.h"
#include "SolidStrongholdGrid.h"
#include "SolidDungeonLevel.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
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

	const SolidStrongholdGrid::FHex* FindFloorHex(
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const int32 Col,
		const int32 Row)
	{
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			if (Hex.Col == Col && Hex.Row == Row)
			{
				return &Hex;
			}
		}
		return nullptr;
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

void ASolidDungeonInterior::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: missing Engine BasicShapes Cube."));
		return;
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	TArray<SolidStrongholdGrid::FSegment> Segments;
	TArray<SolidStrongholdGrid::FSegment> Boundary;
	SolidStrongholdGrid::CollectMeagreHexes(Hexes);
	SolidStrongholdGrid::CollectSegments(Hexes, Segments);
	SolidStrongholdGrid::CollectBoundarySegments(Hexes, Boundary);

	const float Height = SolidDungeonLevel::RoomHeightCm;
	const float Shell = SolidDungeonLevel::ShellCm;
	const float MouthHeight = SolidDungeonLevel::MouthHeightCm;
	const FLinearColor Rock = SolidStrongholds::RockColor();
	const FVector2D MouthMid = SolidStrongholdGrid::MeagreMouthMidpoint();

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
	const float CornerOverlap = Shell * 0.45f;
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
		const FVector2D Outward = EdgeHex
			? (Mid2 - EdgeHex->Center).GetSafeNormal()
			: FVector2D(-1.f, 0.f);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		const bool bMouth = Mid2.Equals(MouthMid, 2.f);
		if (bMouth)
		{
			const FVector MouthCenter(Mid2.X, Mid2.Y, MouthHeight * 0.5f);
			PlaceInteriorPart(
				MouthMesh,
				Cube,
				this,
				MouthCenter + FVector(Outward.X, Outward.Y, 0.f) * (Shell * 0.5f),
				FVector(Length, Shell, MouthHeight),
				SolidStrongholds::MouthColor(),
				TEXT("CaveInteriorMouth"));
			if (MouthMesh)
			{
				MouthMesh->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
			}
			if (WallShell)
			{
				const float Header = FMath::Max(Height - MouthHeight, 1.f);
				const FTransform Lintel(
					FRotator(0.f, Yaw, 0.f),
					FVector(Mid2.X, Mid2.Y, MouthHeight + Header * 0.5f) + FVector(Outward.X, Outward.Y, 0.f) * (Shell * 0.5f),
					FVector(Length / ShapeSize, Shell / ShapeSize, Header / ShapeSize));
				WallShell->AddInstance(Lintel);
			}
			bPlacedMouth = true;
			continue;
		}

		if (WallShell)
		{
			const FTransform Wall(
				FRotator(0.f, Yaw, 0.f),
				FVector(Mid2.X, Mid2.Y, Height * 0.5f) + FVector(Outward.X, Outward.Y, 0.f) * (Shell * 0.5f),
				FVector((Length + CornerOverlap) / ShapeSize, Shell / ShapeSize, Height / ShapeSize));
			WallShell->AddInstance(Wall);
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

	TArray<SolidStrongholdFurniture::FPlacement> Placements;
	SolidStrongholdFurniture::MeagreStarter(Placements);

	TArray<ASolidFurnishing*> Owned;
	for (TActorIterator<ASolidFurnishing> It(World); It; ++It)
	{
		ASolidFurnishing* Piece = *It;
		if (Piece && Piece->GetOwner() == this && !Piece->IsActorBeingDestroyed())
		{
			Owned.Add(Piece);
		}
	}
	if (Owned.Num() >= Placements.Num())
	{
		return;
	}
	for (ASolidFurnishing* Piece : Owned)
	{
		Piece->Destroy();
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidStrongholdGrid::CollectMeagreHexes(Hexes);

	int32 Placed = 0;
	for (const SolidStrongholdFurniture::FPlacement& Placement : Placements)
	{
		const SolidStrongholdGrid::FHex* Anchor = FindFloorHex(Hexes, Placement.Col, Placement.Row);
		const SolidStrongholdGrid::FHex* Span = FindFloorHex(Hexes, Placement.SpanCol, Placement.SpanRow);
		if (!Anchor || !Span)
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: starter furnishing hex (%d,%d) is not on the floor."),
				Placement.Col, Placement.Row);
			continue;
		}

		const FVector2D Mid = (Anchor->Center + Span->Center) * 0.5f;
		const FVector2D Delta = Span->Center - Anchor->Center;
		const float Yaw = Delta.IsNearlyZero() ? 0.f : FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		const FTransform ActorXf = GetActorTransform();
		const FVector WorldPos = ActorXf.TransformPosition(FVector(Mid.X, Mid.Y, 0.f));
		const FRotator WorldRot = ActorXf.TransformRotation(FRotator(0.f, Yaw, 0.f).Quaternion()).Rotator();
		const FTransform SpawnXf(WorldRot, WorldPos);

		ASolidFurnishing* Piece = World->SpawnActorDeferred<ASolidFurnishing>(
			ASolidFurnishing::StaticClass(),
			SpawnXf,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Piece)
		{
			UE_LOG(LogSolid, Error, TEXT("SolidDungeonInterior: failed to spawn a starter furnishing."));
			continue;
		}

		Piece->SetKind(Placement.Kind);
		Piece->FinishSpawning(SpawnXf);
		Piece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		++Placed;
	}

	UE_LOG(LogSolid, Warning, TEXT("Starter furnishings placed: %d."), Placed);
}

FVector ASolidDungeonInterior::GetCaptainSpawnWorld() const
{
	return GetActorTransform().TransformPosition(SolidDungeonLevel::CaptainSpawnLocal());
}

float ASolidDungeonInterior::GetCaptainYaw() const
{
	return SolidDungeonLevel::CaptainYawDeg;
}
