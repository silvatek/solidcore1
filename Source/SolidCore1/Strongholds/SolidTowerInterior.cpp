#include "SolidTowerInterior.h"
#include "SolidCore1.h"
#include "SolidFurnishing.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"

namespace SolidTowerInteriorPrivate
{
	constexpr int32 SideCount = 64;
	constexpr float CubeSizeCm = 100.f;

	struct FMesh
	{
		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<int32> Triangles;

		void Add(const FVector& A, const FVector& B, const FVector& C, const FVector& Normal)
		{
			const int32 Base = Positions.Num();
			Positions.Add(A);
			Positions.Add(B);
			Positions.Add(C);
			Normals.Add(Normal);
			Normals.Add(Normal);
			Normals.Add(Normal);
			Triangles.Add(Base);
			Triangles.Add(Base + 1);
			Triangles.Add(Base + 2);
		}
	};

	/**
	 * Visible from TowardViewer. The shell's courses are seen when the right-hand
	 * normal points away from the camera, so a face that faces the viewer is flipped.
	 */
	void AddTri(FMesh& Mesh, FVector A, FVector B, FVector C, const FVector& TowardViewer)
	{
		const FVector RightHand = FVector::CrossProduct(B - A, C - A);
		if (FVector::DotProduct(RightHand, TowardViewer) > 0.f)
		{
			Mesh.Add(A, C, B, TowardViewer);
		}
		else
		{
			Mesh.Add(A, B, C, TowardViewer);
		}
	}

	void AddQuad(FMesh& Mesh, const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& TowardViewer)
	{
		AddTri(Mesh, A, B, C, TowardViewer);
		AddTri(Mesh, A, C, D, TowardViewer);
	}

	UStaticMesh* Commit(UObject* Outer, const FMesh& Mesh, const TCHAR* Label)
	{
		if (!Outer || Mesh.Triangles.Num() < 3)
		{
			return nullptr;
		}

		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		Attributes.Register();

		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> InstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> InstanceColors = Attributes.GetVertexInstanceColors();
		TPolygonGroupAttributesRef<FName> PolygonGroupNames = Attributes.GetPolygonGroupMaterialSlotNames();
		InstanceUVs.SetNumChannels(1);

		const FName SlotName(TEXT("Stone"));
		const FPolygonGroupID PolygonGroup = Description.CreatePolygonGroup();
		PolygonGroupNames[PolygonGroup] = SlotName;

		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(Mesh.Positions.Num());
		for (const FVector& Position : Mesh.Positions)
		{
			const FVertexID VertexID = Description.CreateVertex();
			VertexPositions[VertexID] = FVector3f(Position);
			VertexIDs.Add(VertexID);
		}

		for (int32 Tri = 0; Tri + 2 < Mesh.Triangles.Num(); Tri += 3)
		{
			TArray<FVertexInstanceID, TInlineAllocator<3>> Corners;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 Vert = Mesh.Triangles[Tri + Corner];
				const FVertexInstanceID InstanceID = Description.CreateVertexInstance(VertexIDs[Vert]);
				const FVector Normal = Mesh.Normals[Vert];
				InstanceNormals[InstanceID] = FVector3f(Normal);
				FVector Tangent = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
				if (Tangent.IsNearlyZero())
				{
					Tangent = FVector::ForwardVector;
				}
				InstanceTangents[InstanceID] = FVector3f(Tangent);
				InstanceBinormalSigns[InstanceID] = 1.f;
				InstanceUVs.Set(InstanceID, 0, FVector2f(0.f, 0.f));
				InstanceColors[InstanceID] = FVector4f(1.f, 1.f, 1.f, 1.f);
				Corners.Add(InstanceID);
			}
			Description.CreatePolygon(PolygonGroup, Corners);
		}

		UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
		StaticMesh->bAllowCPUAccess = true;
		StaticMesh->NeverStream = true;
		{
			FMeshNaniteSettings NaniteSettings = StaticMesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			StaticMesh->SetNaniteSettings(NaniteSettings);
		}
		StaticMesh->SetStaticMaterials({ FStaticMaterial(nullptr, SlotName, SlotName) });

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bFastBuild = false;
		BuildParams.bAllowCpuAccess = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &Description };
		if (!StaticMesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			UE_LOG(LogSolid, Error, TEXT("SolidTowerInterior: failed to build %s."), Label ? Label : TEXT("a mesh"));
			return nullptr;
		}
		{
			FMeshNaniteSettings NaniteSettings = StaticMesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			StaticMesh->SetNaniteSettings(NaniteSettings);
		}

		if (!StaticMesh->GetBodySetup())
		{
			StaticMesh->CreateBodySetup();
		}
		if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
		{
			BodySetup->AggGeom.BoxElems.Empty();
			BodySetup->AggGeom.ConvexElems.Empty();
			BodySetup->AggGeom.SphereElems.Empty();
			BodySetup->AggGeom.SphylElems.Empty();
			BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
			BodySetup->bDoubleSidedGeometry = true;
			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
		}
		return StaticMesh;
	}

	FVector2D DirAt(const int32 Side)
	{
		const float Angle = 2.f * PI * static_cast<float>(Side) / static_cast<float>(SideCount);
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}

	float AngleAt(const int32 Side)
	{
		return 2.f * PI * static_cast<float>(Side) / static_cast<float>(SideCount);
	}

	/**
	 * Disc at ZTop, Thickness downward. The hole is the gap wedge in the outer annulus.
	 * The inner disc stays solid, so the hole does not reach the middle.
	 */
	void BuildSlab(
		FMesh& Mesh,
		const float OuterR,
		const float InnerR,
		const float ZTop,
		const float Thickness,
		const SolidTowerStronghold::FWedge& Gap)
	{
		if (OuterR <= KINDA_SMALL_NUMBER || Thickness <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float ZBottom = ZTop - Thickness;
		const bool bAnnulus = InnerR > 1.f && InnerR < OuterR - 1.f;
		const FVector Up(0.f, 0.f, 1.f);
		const FVector Down(0.f, 0.f, -1.f);
		const FVector CenterTop(0.f, 0.f, ZTop);
		const FVector CenterBottom(0.f, 0.f, ZBottom);

		TArray<bool> Hole;
		Hole.SetNum(SideCount);
		for (int32 Side = 0; Side < SideCount; ++Side)
		{
			const float Mid = AngleAt(Side) + PI / static_cast<float>(SideCount);
			Hole[Side] = bAnnulus
				&& Gap.bOpen
				&& SolidTowerStronghold::AngleInSweep(Mid, Gap.StartRad, Gap.SweepRad);
		}

		if (bAnnulus)
		{
			for (int32 Side = 0; Side < SideCount; ++Side)
			{
				const FVector2D D0 = DirAt(Side);
				const FVector2D D1 = DirAt(Side + 1);
				const FVector Rim0(D0.X * InnerR, D0.Y * InnerR, ZTop);
				const FVector Rim1(D1.X * InnerR, D1.Y * InnerR, ZTop);
				const FVector Low0(D0.X * InnerR, D0.Y * InnerR, ZBottom);
				const FVector Low1(D1.X * InnerR, D1.Y * InnerR, ZBottom);
				AddTri(Mesh, CenterTop, Rim1, Rim0, Up);
				AddTri(Mesh, CenterBottom, Low0, Low1, Down);
			}
		}

		for (int32 Side = 0; Side < SideCount; ++Side)
		{
			if (Hole[Side])
			{
				continue;
			}

			const FVector2D D0 = DirAt(Side);
			const FVector2D D1 = DirAt(Side + 1);
			const float UseInner = bAnnulus ? InnerR : 0.f;
			const FVector Top0(D0.X * OuterR, D0.Y * OuterR, ZTop);
			const FVector Top1(D1.X * OuterR, D1.Y * OuterR, ZTop);
			const FVector Bot0(D0.X * OuterR, D0.Y * OuterR, ZBottom);
			const FVector Bot1(D1.X * OuterR, D1.Y * OuterR, ZBottom);
			if (UseInner <= 1.f)
			{
				AddTri(Mesh, CenterTop, Top1, Top0, Up);
				AddTri(Mesh, CenterBottom, Bot0, Bot1, Down);
			}
			else
			{
				const FVector InTop0(D0.X * UseInner, D0.Y * UseInner, ZTop);
				const FVector InTop1(D1.X * UseInner, D1.Y * UseInner, ZTop);
				const FVector InBot0(D0.X * UseInner, D0.Y * UseInner, ZBottom);
				const FVector InBot1(D1.X * UseInner, D1.Y * UseInner, ZBottom);
				AddQuad(Mesh, InTop1, Top1, Top0, InTop0, Up);
				AddQuad(Mesh, InBot0, Bot0, Bot1, InBot1, Down);
			}

			const FVector Out = FVector(D0.X + D1.X, D0.Y + D1.Y, 0.f).GetSafeNormal();
			AddQuad(Mesh, Bot1, Bot0, Top0, Top1, Out);
		}

		for (int32 Side = 0; Side < SideCount; ++Side)
		{
			if (!Hole[Side] || !bAnnulus)
			{
				continue;
			}
			const FVector2D D0 = DirAt(Side);
			const FVector2D D1 = DirAt(Side + 1);
			const FVector Top0(D0.X * InnerR, D0.Y * InnerR, ZTop);
			const FVector Top1(D1.X * InnerR, D1.Y * InnerR, ZTop);
			const FVector Bot0(D0.X * InnerR, D0.Y * InnerR, ZBottom);
			const FVector Bot1(D1.X * InnerR, D1.Y * InnerR, ZBottom);
			const FVector Out = FVector(D0.X + D1.X, D0.Y + D1.Y, 0.f).GetSafeNormal();
			AddQuad(Mesh, Bot1, Bot0, Top0, Top1, Out);
		}

		for (int32 Side = 0; Side < SideCount; ++Side)
		{
			const int32 Next = (Side + 1) % SideCount;
			if (Hole[Side] == Hole[Next])
			{
				continue;
			}
			const FVector2D D = DirAt(Side + 1);
			const FVector TopIn(D.X * InnerR, D.Y * InnerR, ZTop);
			const FVector TopOut(D.X * OuterR, D.Y * OuterR, ZTop);
			const FVector BotIn(D.X * InnerR, D.Y * InnerR, ZBottom);
			const FVector BotOut(D.X * OuterR, D.Y * OuterR, ZBottom);
			const FVector Tangent(-D.Y, D.X, 0.f);
			const FVector TowardHole = Hole[Next] ? Tangent : -Tangent;
			AddQuad(Mesh, BotIn, BotOut, TopOut, TopIn, TowardHole);
		}
	}

	void AddWallSlice(
		FMesh& Mesh,
		const SolidTowerStronghold::FPlan& Plan,
		const float Z0,
		const float Z1,
		const float Angle0,
		const float Angle1)
	{
		if (Angle1 <= Angle0 + 1.e-4f)
		{
			return;
		}
		const FVector2D D0(FMath::Cos(Angle0), FMath::Sin(Angle0));
		const FVector2D D1(FMath::Cos(Angle1), FMath::Sin(Angle1));
		const float R0 = SolidTowerStronghold::InteriorRadiusCm(Plan, Z0);
		const float R1 = SolidTowerStronghold::InteriorRadiusCm(Plan, Z1);
		const FVector B0(D0.X * R0, D0.Y * R0, Z0);
		const FVector B1(D1.X * R0, D1.Y * R0, Z0);
		const FVector T0(D0.X * R1, D0.Y * R1, Z1);
		const FVector T1(D1.X * R1, D1.Y * R1, Z1);
		const FVector Inward = -FVector(D0.X + D1.X, D0.Y + D1.Y, 0.f).GetSafeNormal();
		AddQuad(Mesh, B0, B1, T1, T0, Inward);
	}

	/**
	 * Inner face from Z0 to Z1. The ground band leaves a gap the width of the door.
	 * The gap is clipped to the door angle, so the jambs meet the slab.
	 */
	void AddWallBand(
		FMesh& Mesh,
		const SolidTowerStronghold::FPlan& Plan,
		const float Z0,
		const float Z1,
		const bool bDoorGap)
	{
		if (Z1 <= Z0 + KINDA_SMALL_NUMBER)
		{
			return;
		}

		float Gap0 = 0.f;
		float Gap1 = 0.f;
		if (bDoorGap)
		{
			const float Radius = FMath::Max(SolidTowerStronghold::InteriorRadiusCm(Plan, Z0), 1.f);
			const float Half = FMath::Asin(FMath::Clamp((Plan.DoorWidthCm * 0.5f) / Radius, 0.f, 1.f));
			Gap0 = SolidTowerStronghold::WrapTau(Plan.DoorAngleRad - Half);
			Gap1 = Gap0 + Half * 2.f;
		}

		for (int32 Side = 0; Side < SideCount; ++Side)
		{
			const float A0 = AngleAt(Side);
			const float A1 = AngleAt(Side + 1);
			if (!bDoorGap || A1 <= Gap0 || A0 >= Gap1)
			{
				AddWallSlice(Mesh, Plan, Z0, Z1, A0, A1);
				continue;
			}
			if (A0 < Gap0)
			{
				AddWallSlice(Mesh, Plan, Z0, Z1, A0, Gap0);
			}
			if (A1 > Gap1)
			{
				AddWallSlice(Mesh, Plan, Z0, Z1, Gap1, A1);
			}
		}
	}

	UStaticMeshComponent* AddStone(
		AActor* Owner,
		USceneComponent* Root,
		UStaticMesh* Mesh,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		if (!Owner || !Root || !Mesh)
		{
			return nullptr;
		}

		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		if (!Part)
		{
			return nullptr;
		}
		Part->SetupAttachment(Root);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(FVector::ZeroVector);
		Part->SetRelativeRotation(FRotator::ZeroRotator);
		Part->SetRelativeScale3D(FVector::OneVector);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Owner, Color, DebugName, 0.94f))
		{
			Part->SetMaterial(0, Material);
		}
		if (Owner->GetWorld())
		{
			Part->RegisterComponent();
		}
		return Part;
	}
}

ASolidTowerInterior::ASolidTowerInterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FloorGrid = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorGrid"));
	FloorGrid->SetupAttachment(SceneRoot);
	FloorGrid->SetMobility(EComponentMobility::Movable);
	FloorGrid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloorGrid->SetCastShadow(false);

	Stairs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stairs"));
	Stairs->SetupAttachment(SceneRoot);
	Stairs->SetMobility(EComponentMobility::Movable);
	Stairs->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Stairs->SetCollisionResponseToAllChannels(ECR_Block);
	Stairs->SetCastShadow(true);

	Rails = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rails"));
	Rails->SetupAttachment(SceneRoot);
	Rails->SetMobility(EComponentMobility::Movable);
	Rails->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Rails->SetCollisionResponseToAllChannels(ECR_Block);
	Rails->SetCastShadow(true);

	FloorSignDigits = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorSignDigits"));
	FloorSignDigits->SetupAttachment(SceneRoot);
	FloorSignDigits->SetMobility(EComponentMobility::Movable);
	FloorSignDigits->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FloorSignDigits->SetCastShadow(false);
}

void ASolidTowerInterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidTowerInterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidTowerInterior::SetPlan(const SolidTowerStronghold::FPlan& InPlan)
{
	Plan = InPlan;
}

UStaticMeshComponent* ASolidTowerInterior::GetFloor(const int32 Index) const
{
	if (!Floors.IsValidIndex(Index))
	{
		return nullptr;
	}
	return Floors[Index].Get();
}

UStaticMeshComponent* ASolidTowerInterior::GetWall(const int32 Index) const
{
	if (!Walls.IsValidIndex(Index))
	{
		return nullptr;
	}
	return Walls[Index].Get();
}

UStaticMeshComponent* ASolidTowerInterior::GetFloorSignPlate(const int32 Index) const
{
	if (!FloorSignPlates.IsValidIndex(Index))
	{
		return nullptr;
	}
	return FloorSignPlates[Index].Get();
}

FVector ASolidTowerInterior::GetCaptainSpawnWorld() const
{
	const FVector2D Center = SolidTowerStronghold::DoorHexCenter(Plan);
	return FVector(Center.X, Center.Y, 0.f);
}

FRotator ASolidTowerInterior::GetCaptainView() const
{
	return FRotator(0.f, 0.f, 0.f);
}

void ASolidTowerInterior::DiscardParts()
{
	auto DropMesh = [](TObjectPtr<UStaticMeshComponent>& Part)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
			Part = nullptr;
		}
	};
	for (TObjectPtr<UStaticMeshComponent>& Floor : Floors)
	{
		DropMesh(Floor);
	}
	Floors.Reset();
	for (TObjectPtr<UStaticMeshComponent>& Wall : Walls)
	{
		DropMesh(Wall);
	}
	Walls.Reset();
	DropMesh(CapMesh);
	DropMesh(DoorMesh);
	if (FloorGrid)
	{
		FloorGrid->ClearInstances();
	}
	if (Stairs)
	{
		Stairs->ClearInstances();
	}
	if (Rails)
	{
		Rails->ClearInstances();
	}
	for (TObjectPtr<UPointLightComponent>& Light : Lights)
	{
		if (UPointLightComponent* Point = Light.Get())
		{
			Point->DestroyComponent();
			Light = nullptr;
		}
	}
	Lights.Reset();
	for (TObjectPtr<UStaticMeshComponent>& Plate : FloorSignPlates)
	{
		if (UStaticMeshComponent* Mesh = Plate.Get())
		{
			Mesh->DestroyComponent();
			Plate = nullptr;
		}
	}
	FloorSignPlates.Reset();
	if (FloorSignDigits)
	{
		FloorSignDigits->ClearInstances();
	}
}

void ASolidTowerInterior::BuildVisuals()
{
	DiscardParts();
	if (!SceneRoot || Plan.FloorCount <= 0)
	{
		return;
	}

	const FLinearColor Rock = SolidStrongholds::RockColor();
	Floors.Reserve(Plan.FloorCount);
	Walls.Reserve(Plan.FloorCount);
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		const float Z = SolidTowerStronghold::FloorZCm(Plan, Floor);
		const float Outer = SolidTowerStronghold::InteriorRadiusCm(Plan, Z);
		const float Inner = FMath::Max(Outer - Plan.StairWidthCm, 0.f);
		SolidTowerInteriorPrivate::FMesh Slab;
		// The hole is the arriving flight. The landing that follows it stays in this slab.
		SolidTowerInteriorPrivate::BuildSlab(
			Slab,
			Outer,
			Inner,
			Z,
			Plan.SlabCm,
			SolidTowerStronghold::GapWedge(Plan, Floor));
		UStaticMesh* SlabMesh = SolidTowerInteriorPrivate::Commit(this, Slab, TEXT("a floor"));
		if (UStaticMeshComponent* Part = SolidTowerInteriorPrivate::AddStone(this, SceneRoot, SlabMesh, Rock, TEXT("TowerFloor")))
		{
			Floors.Add(Part);
		}

		const float NextZ = (Floor + 1 < Plan.FloorCount)
			? SolidTowerStronghold::FloorZCm(Plan, Floor + 1)
			: Plan.HeightCm();
		SolidTowerInteriorPrivate::FMesh Wall;
		if (SolidTowerStronghold::HasDoor(Floor))
		{
			const float Lintel = FMath::Min(Z + Plan.DoorHeightCm, NextZ);
			SolidTowerInteriorPrivate::AddWallBand(Wall, Plan, Z, Lintel, true);
			SolidTowerInteriorPrivate::AddWallBand(Wall, Plan, Lintel, NextZ, false);
		}
		else
		{
			SolidTowerInteriorPrivate::AddWallBand(Wall, Plan, Z, NextZ, false);
		}
		UStaticMesh* WallMesh = SolidTowerInteriorPrivate::Commit(this, Wall, TEXT("a wall"));
		if (UStaticMeshComponent* Part = SolidTowerInteriorPrivate::AddStone(this, SceneRoot, WallMesh, Rock, TEXT("TowerWall")))
		{
			Walls.Add(Part);
		}

		UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
		if (Light)
		{
			Light->SetupAttachment(SceneRoot);
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetCastShadows(false);
			Light->SetIntensity(2500.f);
			Light->SetAttenuationRadius(FMath::Max(Outer * 1.6f, 400.f));
			Light->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
			Light->SetRelativeLocation(FVector(0.f, 0.f, Z + Plan.ClearHeightCm() * 0.55f));
			if (GetWorld())
			{
				Light->RegisterComponent();
			}
			Lights.Add(Light);
		}
	}

	{
		SolidTowerInteriorPrivate::FMesh Cap;
		const float Z = Plan.HeightCm();
		SolidTowerInteriorPrivate::BuildSlab(
			Cap,
			SolidTowerStronghold::InteriorRadiusCm(Plan, Z),
			0.f,
			Z,
			Plan.SlabCm,
			SolidTowerStronghold::FWedge{});
		UStaticMesh* CapBuilt = SolidTowerInteriorPrivate::Commit(this, Cap, TEXT("the cap"));
		CapMesh = SolidTowerInteriorPrivate::AddStone(this, SceneRoot, CapBuilt, Rock, TEXT("TowerCeiling"));
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidTowerInterior: missing Engine BasicShapes Cube."));
		return;
	}

	if (Plan.DoorWidthCm > 0.f && Plan.DoorHeightCm > 0.f)
	{
		const FVector2D Out = SolidTowerStronghold::DoorOutward(Plan);
		const float HalfDepth = SolidTowerStronghold::DoorSlabDepthCm * 0.5f;
		const FVector Center(
			Out.X * (Plan.GroundInteriorRadiusCm() - HalfDepth),
			Out.Y * (Plan.GroundInteriorRadiusCm() - HalfDepth),
			Plan.DoorHeightCm * 0.5f);
		DoorMesh = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (DoorMesh)
		{
			DoorMesh->SetupAttachment(SceneRoot);
			DoorMesh->SetMobility(EComponentMobility::Movable);
			DoorMesh->SetStaticMesh(Cube);
			DoorMesh->SetRelativeLocation(Center);
			DoorMesh->SetRelativeRotation(FRotator(
				0.f,
				FMath::RadiansToDegrees(FMath::Atan2(Out.Y, Out.X)),
				0.f));
			DoorMesh->SetRelativeScale3D(FVector(
				SolidTowerStronghold::DoorSlabDepthCm / SolidTowerInteriorPrivate::CubeSizeCm,
				Plan.DoorWidthCm / SolidTowerInteriorPrivate::CubeSizeCm,
				Plan.DoorHeightCm / SolidTowerInteriorPrivate::CubeSizeCm));
			DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			DoorMesh->SetCollisionResponseToAllChannels(ECR_Block);
			DoorMesh->SetCastShadow(true);
			if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
				this, SolidStrongholds::MouthColor(), TEXT("TowerLeaveDoor"), 0.94f))
			{
				DoorMesh->SetMaterial(0, Material);
			}
			if (GetWorld())
			{
				DoorMesh->RegisterComponent();
			}
		}
	}

	if (FloorGrid)
	{
		FloorGrid->SetStaticMesh(Cube);
		FloorGrid->ClearInstances();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			this, SolidStrongholdGrid::LineColor(), TEXT("TowerHexGrid"), 0.45f))
		{
			FloorGrid->SetMaterial(0, Material);
		}
	}
	if (Stairs)
	{
		Stairs->SetStaticMesh(Cube);
		Stairs->ClearInstances();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Rock, TEXT("TowerTread"), 0.94f))
		{
			Stairs->SetMaterial(0, Material);
		}
	}
	if (Rails)
	{
		Rails->SetStaticMesh(Cube);
		Rails->ClearInstances();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Rock, TEXT("TowerRail"), 0.94f))
		{
			Rails->SetMaterial(0, Material);
		}
	}

	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		const float Z = SolidTowerStronghold::FloorZCm(Plan, Floor);
		TArray<SolidStrongholdGrid::FHex> Hexes;
		TArray<SolidStrongholdGrid::FSegment> Segments;
		SolidTowerStronghold::CollectKeptHexes(Plan, Floor, Hexes);
		SolidStrongholdGrid::CollectSegments(Hexes, Segments);
		if (FloorGrid)
		{
			const float LineZ = Z + SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm * 0.5f;
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
				FloorGrid->AddInstance(FTransform(
					FRotator(0.f, Yaw, 0.f),
					Mid,
					FVector(
						Length / SolidTowerInteriorPrivate::CubeSizeCm,
						SolidStrongholdGrid::LineThicknessCm / SolidTowerInteriorPrivate::CubeSizeCm,
						SolidStrongholdGrid::LineHeightCm / SolidTowerInteriorPrivate::CubeSizeCm)));
			}
		}

		const SolidTowerStronghold::FWedge Flight = SolidTowerStronghold::FlightWedge(Plan, Floor);
		if (!Stairs || !Flight.bOpen || Plan.StairTreadCount <= 0)
		{
			continue;
		}
		const float Centerline = SolidTowerStronghold::InteriorRadiusCm(Plan, Z) - Plan.StairWidthCm * 0.5f;
		const float Riser = Plan.RiserCm();
		for (int32 Tread = 0; Tread < Plan.StairTreadCount; ++Tread)
		{
			const float Along = (static_cast<float>(Tread) + 0.5f) / static_cast<float>(Plan.StairTreadCount);
			const float Angle = Flight.StartRad + Along * Flight.SweepRad;
			const FVector Location(
				FMath::Cos(Angle) * Centerline,
				FMath::Sin(Angle) * Centerline,
				Z + (static_cast<float>(Tread) + 0.5f) * Riser);
			const float Yaw = FMath::RadiansToDegrees(Angle) + 90.f;
			Stairs->AddInstance(FTransform(
				FRotator(0.f, Yaw, 0.f),
				Location,
				FVector(
					Plan.StairTreadCm / SolidTowerInteriorPrivate::CubeSizeCm,
					Plan.StairWidthCm / SolidTowerInteriorPrivate::CubeSizeCm,
					Riser / SolidTowerInteriorPrivate::CubeSizeCm)));
		}
	}

	if (Rails)
	{
		for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
		{
			TArray<SolidTowerStronghold::FRailPiece> Hole;
			TArray<SolidTowerStronghold::FRailPiece> Flight;
			SolidTowerStronghold::CollectHoleRail(Plan, Floor, Hole);
			SolidTowerStronghold::CollectFlightRail(Plan, Floor, Flight);
			auto Place = [this](const TArray<SolidTowerStronghold::FRailPiece>& Pieces)
			{
				for (const SolidTowerStronghold::FRailPiece& Piece : Pieces)
				{
					Rails->AddInstance(FTransform(
						Piece.Rotation,
						Piece.Location,
						Piece.SizeCm / SolidTowerInteriorPrivate::CubeSizeCm));
				}
			};
			Place(Hole);
			Place(Flight);
		}
	}

	if (FloorSignDigits)
	{
		FloorSignDigits->SetStaticMesh(Cube);
		FloorSignDigits->ClearInstances();
		const FColor Ink = SolidTowerStronghold::FloorSignTextColor();
		const FLinearColor InkLinear(Ink.R / 255.f, Ink.G / 255.f, Ink.B / 255.f, 1.f);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			this, InkLinear, TEXT("TowerFloorNumber"), 0.9f))
		{
			FloorSignDigits->SetMaterial(0, Material);
		}
	}

	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		const SolidTowerStronghold::FFloorSign Sign = SolidTowerStronghold::FloorSignFor(Plan, Floor);
		if (!Sign.bPresent)
		{
			continue;
		}

		const FRotator Facing(0.f, Sign.YawDeg, 0.f);
		UStaticMeshComponent* Plate = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (Plate)
		{
			Plate->SetupAttachment(SceneRoot);
			Plate->SetMobility(EComponentMobility::Movable);
			Plate->SetStaticMesh(Cube);
			Plate->SetRelativeLocation(SolidTowerStronghold::FloorSignPlateLocation(Sign.Location, Sign.YawDeg));
			Plate->SetRelativeRotation(Facing);
			Plate->SetRelativeScale3D(FVector(
				SolidTowerStronghold::FloorSignPlateDepthCm / SolidTowerInteriorPrivate::CubeSizeCm,
				SolidTowerStronghold::FloorSignPlateWidthCm / SolidTowerInteriorPrivate::CubeSizeCm,
				SolidTowerStronghold::FloorSignPlateHeightCm / SolidTowerInteriorPrivate::CubeSizeCm));
			Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Plate->SetCastShadow(false);
			Plate->SetVisibility(true);
			if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
				this, SolidTowerStronghold::FloorSignPlateColor(), TEXT("TowerFloorPlate"), 0.88f))
			{
				Plate->SetMaterial(0, Material);
			}
			if (GetWorld())
			{
				Plate->RegisterComponent();
			}
			FloorSignPlates.Add(Plate);
		}

		if (!FloorSignDigits)
		{
			continue;
		}
		TArray<SolidTowerStronghold::FFloorSignStroke> Strokes;
		SolidTowerStronghold::CollectFloorSignStrokes(Sign.Number, Strokes);
		for (const SolidTowerStronghold::FFloorSignStroke& Stroke : Strokes)
		{
			FloorSignDigits->AddInstance(FTransform(
				Facing,
				Sign.Location + Facing.RotateVector(Stroke.CenterCm),
				Stroke.SizeCm / SolidTowerInteriorPrivate::CubeSizeCm));
		}
	}
}

void ASolidTowerInterior::PlaceFurnishings(const TArray<SolidStrongholdFurniture::FUsed>& Used)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || World->bIsTearingDown || !IsActorInitialized() || Used.Num() == 0)
	{
		return;
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
	if (Owned.Num() >= Used.Num())
	{
		return;
	}
	for (ASolidFurnishing* Piece : Owned)
	{
		Piece->Destroy();
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidTowerStronghold::CollectKeptHexes(Plan, 0, Hexes);

	int32 Placed = 0;
	for (const SolidStrongholdFurniture::FUsed& UsedPiece : Used)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(UsedPiece.ItemIndex);
		if (!Item || UsedPiece.Level != 0)
		{
			continue;
		}

		const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(Hexes, UsedPiece.X, UsedPiece.Y);
		const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Hexes, UsedPiece);
		if (!Anchor || !Span)
		{
			continue;
		}

		const FVector2D Mid = (Anchor->Center + Span->Center) * 0.5f;
		const FTransform ActorXf = GetActorTransform();
		const FVector WorldPos = ActorXf.TransformPosition(FVector(Mid.X, Mid.Y, 0.f));
		const FRotator WorldRot = ActorXf.TransformRotation(
			FRotator(0.f, UsedPiece.RotationDeg, 0.f).Quaternion()).Rotator();
		const FTransform SpawnXf(WorldRot, WorldPos);

		ASolidFurnishing* Piece = World->SpawnActorDeferred<ASolidFurnishing>(
			ASolidFurnishing::StaticClass(),
			SpawnXf,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Piece)
		{
			continue;
		}

		Piece->SetKind(Item->Kind);
		Piece->FinishSpawning(SpawnXf);
		Piece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		++Placed;
	}

	UE_LOG(LogSolid, Warning, TEXT("Tower furnishings placed: %d."), Placed);
}
