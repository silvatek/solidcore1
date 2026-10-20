#include "SolidTowerExterior.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Strongholds/SolidStronghold.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"

namespace SolidTowerExteriorPrivate
{
	constexpr float CubeSizeCm = 100.f;
	constexpr int32 SideCount = 64;

	void AddCorner(
		TArray<FVector>& Positions,
		TArray<FVector>& Normals,
		TArray<int32>& Triangles,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& Normal)
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

	/**
	 * Truncated cone. Z is 0 at the wide-or-narrow foot and Height at the head.
	 * Sides are wound clockwise from the outside, the same way the terrain is.
	 * The foot and the head are closed only when asked, so two courses can meet
	 * without two faces on the same disc.
	 */
	UStaticMesh* BuildFrustum(
		UObject* Outer,
		const float BottomRadius,
		const float TopRadius,
		const float Height,
		const bool bCloseBottom,
		const bool bCloseTop)
	{
		if (!Outer || Height <= KINDA_SMALL_NUMBER || BottomRadius <= KINDA_SMALL_NUMBER || TopRadius <= KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<int32> Triangles;
		Positions.Reserve(SideCount * 12);
		Normals.Reserve(SideCount * 12);
		Triangles.Reserve(SideCount * 12);

		const float Slope = (TopRadius - BottomRadius) / Height;
		const float NormalZ = -Slope;
		const float NormalLength = FMath::Sqrt(1.f + NormalZ * NormalZ);

		for (int32 Side = 0; Side < SideCount; ++Side)
		{
			const float Angle0 = 2.f * PI * static_cast<float>(Side) / static_cast<float>(SideCount);
			const float Angle1 = 2.f * PI * static_cast<float>(Side + 1) / static_cast<float>(SideCount);
			const FVector2D Dir0(FMath::Cos(Angle0), FMath::Sin(Angle0));
			const FVector2D Dir1(FMath::Cos(Angle1), FMath::Sin(Angle1));
			const FVector Bottom0(Dir0.X * BottomRadius, Dir0.Y * BottomRadius, 0.f);
			const FVector Bottom1(Dir1.X * BottomRadius, Dir1.Y * BottomRadius, 0.f);
			const FVector Top0(Dir0.X * TopRadius, Dir0.Y * TopRadius, Height);
			const FVector Top1(Dir1.X * TopRadius, Dir1.Y * TopRadius, Height);
			const FVector Normal0(Dir0.X / NormalLength, Dir0.Y / NormalLength, NormalZ / NormalLength);
			const FVector Normal1(Dir1.X / NormalLength, Dir1.Y / NormalLength, NormalZ / NormalLength);
			const FVector SideNormal = (Normal0 + Normal1).GetSafeNormal();

			// Clockwise when seen from outside: the next angle, this angle, then up.
			AddCorner(Positions, Normals, Triangles, Bottom1, Bottom0, Top0, SideNormal);
			AddCorner(Positions, Normals, Triangles, Bottom1, Top0, Top1, SideNormal);
		}

		if (bCloseBottom)
		{
			const FVector Center(0.f, 0.f, 0.f);
			const FVector Down(0.f, 0.f, -1.f);
			for (int32 Side = 0; Side < SideCount; ++Side)
			{
				const float Angle0 = 2.f * PI * static_cast<float>(Side) / static_cast<float>(SideCount);
				const float Angle1 = 2.f * PI * static_cast<float>(Side + 1) / static_cast<float>(SideCount);
				const FVector Rim0(FMath::Cos(Angle0) * BottomRadius, FMath::Sin(Angle0) * BottomRadius, 0.f);
				const FVector Rim1(FMath::Cos(Angle1) * BottomRadius, FMath::Sin(Angle1) * BottomRadius, 0.f);
				AddCorner(Positions, Normals, Triangles, Center, Rim0, Rim1, Down);
			}
		}

		if (bCloseTop)
		{
			const FVector Center(0.f, 0.f, Height);
			const FVector Up(0.f, 0.f, 1.f);
			for (int32 Side = 0; Side < SideCount; ++Side)
			{
				const float Angle0 = 2.f * PI * static_cast<float>(Side) / static_cast<float>(SideCount);
				const float Angle1 = 2.f * PI * static_cast<float>(Side + 1) / static_cast<float>(SideCount);
				const FVector Rim0(FMath::Cos(Angle0) * TopRadius, FMath::Sin(Angle0) * TopRadius, Height);
				const FVector Rim1(FMath::Cos(Angle1) * TopRadius, FMath::Sin(Angle1) * TopRadius, Height);
				AddCorner(Positions, Normals, Triangles, Center, Rim1, Rim0, Up);
			}
		}

		FMeshDescription MeshDescription;
		FStaticMeshAttributes Attributes(MeshDescription);
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
		const FPolygonGroupID PolygonGroup = MeshDescription.CreatePolygonGroup();
		PolygonGroupNames[PolygonGroup] = SlotName;

		TArray<FVertexID> VertexIDs;
		VertexIDs.Reserve(Positions.Num());
		for (const FVector& Position : Positions)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = FVector3f(Position);
			VertexIDs.Add(VertexID);
		}

		for (int32 Tri = 0; Tri + 2 < Triangles.Num(); Tri += 3)
		{
			TArray<FVertexInstanceID, TInlineAllocator<3>> Corners;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 Vert = Triangles[Tri + Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[Vert]);
				const FVector Normal = Normals[Vert];
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
			MeshDescription.CreatePolygon(PolygonGroup, Corners);
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
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bFastBuild = false;
		BuildParams.bAllowCpuAccess = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			UE_LOG(LogSolid, Error, TEXT("SolidTowerExterior: failed to build a course."));
			return nullptr;
		}
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		if (!Mesh->GetBodySetup())
		{
			Mesh->CreateBodySetup();
		}
		if (UBodySetup* BodySetup = Mesh->GetBodySetup())
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
		return Mesh;
	}
}

ASolidTowerExterior::ASolidTowerExterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolidTowerExterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidTowerExterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidTowerExterior::DiscardParts()
{
	auto Drop = [](TObjectPtr<UStaticMeshComponent>& Part)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
			Part = nullptr;
		}
	};

	for (TObjectPtr<UStaticMeshComponent>& Course : Courses)
	{
		Drop(Course);
	}
	Courses.Reset();
	Drop(CapMesh);
	Drop(DoorMesh);
}

UStaticMeshComponent* ASolidTowerExterior::AddPart(
	UStaticMesh* Mesh,
	const FVector& Location,
	const FVector& Scale,
	const FLinearColor& Color,
	const TCHAR* DebugName)
{
	if (!Mesh || !SceneRoot)
	{
		return nullptr;
	}

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
	if (!Part)
	{
		return nullptr;
	}

	Part->SetupAttachment(SceneRoot);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(FRotator::ZeroRotator);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Part->SetCollisionResponseToAllChannels(ECR_Block);
	Part->SetCastShadow(true);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Color, DebugName, 0.94f))
	{
		Part->SetMaterial(0, Material);
	}
	if (GetWorld())
	{
		Part->RegisterComponent();
	}
	return Part;
}

void ASolidTowerExterior::BuildVisuals()
{
	DiscardParts();
	if (!Tower)
	{
		return;
	}
	const SolidStronghold::FPlan& Plan = Tower->Plan();

	if (Plan.FloorCount <= 0)
	{
		return;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidTowerExterior: missing Engine BasicShapes Cube."));
		return;
	}

	const FLinearColor Rock = SolidStrongholds::RockColor();
	const bool bHasCap = Plan.SlabCm > 1.f;
	const float CapBottomZ = bHasCap ? Tower->HeightCm() - Plan.SlabCm : Tower->HeightCm();

	TArray<float> RingZ;
	RingZ.Reserve(Plan.FloorCount + 2);
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		RingZ.Add(Tower->FloorZCm(Floor));
	}
	RingZ.Add(CapBottomZ);
	if (bHasCap)
	{
		RingZ.Add(Tower->HeightCm());
	}

	TArray<float> RingRadius;
	RingRadius.Reserve(RingZ.Num());
	for (const float Z : RingZ)
	{
		RingRadius.Add(Tower->ExteriorRadiusCm(Z));
	}

	Courses.Reserve(Plan.FloorCount);
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		const float BottomZ = RingZ[Floor];
		const float TopZ = RingZ[Floor + 1];
		const float Height = TopZ - BottomZ;
		UStaticMesh* CourseMesh = SolidTowerExteriorPrivate::BuildFrustum(
			this,
			RingRadius[Floor],
			RingRadius[Floor + 1],
			Height,
			Floor == 0,
			!bHasCap && Floor == Plan.FloorCount - 1);
		UStaticMeshComponent* Course = AddPart(
			CourseMesh,
			FVector(0.f, 0.f, BottomZ),
			FVector::OneVector,
			Rock,
			TEXT("TowerCourse"));
		if (Course)
		{
			Courses.Add(Course);
		}
	}

	if (bHasCap)
	{
		const int32 CapRing = RingZ.Num() - 2;
		const float BottomZ = RingZ[CapRing];
		const float TopZ = RingZ[CapRing + 1];
		UStaticMesh* Cap = SolidTowerExteriorPrivate::BuildFrustum(
			this,
			RingRadius[CapRing],
			RingRadius[CapRing + 1],
			TopZ - BottomZ,
			false,
			true);
		CapMesh = AddPart(
			Cap,
			FVector(0.f, 0.f, BottomZ),
			FVector::OneVector,
			Rock,
			TEXT("TowerCap"));
	}

	if (Tower->HasDoor(0) && Plan.DoorWidthCm > 0.f && Plan.DoorHeightCm > 0.f)
	{
		DoorMesh = AddPart(
			Cube,
			Tower->DoorSlabCenterCm(),
			FVector(
				SolidStronghold::DoorSlabDepthCm / SolidTowerExteriorPrivate::CubeSizeCm,
				Plan.DoorWidthCm / SolidTowerExteriorPrivate::CubeSizeCm,
				Plan.DoorHeightCm / SolidTowerExteriorPrivate::CubeSizeCm),
			SolidStrongholds::MouthColor(),
			TEXT("TowerDoor"));
	}
}
