#include "SolidBuilding.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshDescription.h"

namespace
{
	/** Unit-scale gable prism: base half-extents 50cm on X/Y, height 100cm, ridge along +Y. */
	UStaticMesh* BuildUnitGablePrismMesh(UObject* Outer)
	{
		// Unique verts per triangle corner so faces stay flat-shaded.
		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<FVector> Tangents;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<int32> Triangles;

		auto AddTri = [&](const FVector& A, const FVector& B, const FVector& C,
			const FVector2D& UvA, const FVector2D& UvB, const FVector2D& UvC)
		{
			FVector N = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
			if (N.IsNearlyZero())
			{
				N = FVector::UpVector;
			}
			FVector T = FVector::CrossProduct(N, FVector::UpVector).GetSafeNormal();
			if (T.IsNearlyZero())
			{
				T = FVector::ForwardVector;
			}

			const int32 Base = Positions.Num();
			Positions.Add(A);
			Positions.Add(B);
			Positions.Add(C);
			Normals.Add(N);
			Normals.Add(N);
			Normals.Add(N);
			Tangents.Add(T);
			Tangents.Add(T);
			Tangents.Add(T);
			UVs.Add(UvA);
			UVs.Add(UvB);
			UVs.Add(UvC);
			Colors.Add(FLinearColor::White);
			Colors.Add(FLinearColor::White);
			Colors.Add(FLinearColor::White);
			Triangles.Add(Base + 0);
			Triangles.Add(Base + 1);
			Triangles.Add(Base + 2);
		};

		constexpr float Hx = 50.f;
		constexpr float Hy = 50.f;
		constexpr float Hh = 100.f;
		const FVector EavesFL(-Hx, -Hy, 0.f);
		const FVector EavesFR(Hx, -Hy, 0.f);
		const FVector EavesBR(Hx, Hy, 0.f);
		const FVector EavesBL(-Hx, Hy, 0.f);
		const FVector RidgeF(0.f, -Hy, Hh);
		const FVector RidgeB(0.f, Hy, Hh);

		// Front / back gables.
		AddTri(EavesFL, EavesFR, RidgeF, FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.5f, 1.f));
		AddTri(EavesBR, EavesBL, RidgeB, FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.5f, 1.f));
		// Left / right slopes (two tris each).
		AddTri(EavesFL, RidgeF, RidgeB, FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(1.f, 1.f));
		AddTri(EavesFL, RidgeB, EavesBL, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), FVector2D(0.f, 1.f));
		AddTri(EavesFR, EavesBR, RidgeB, FVector2D(0.f, 0.f), FVector2D(0.f, 1.f), FVector2D(1.f, 1.f));
		AddTri(EavesFR, RidgeB, RidgeF, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), FVector2D(1.f, 0.f));
		// Underside so the volume is closed.
		AddTri(EavesFL, EavesBL, EavesBR, FVector2D(0.f, 0.f), FVector2D(0.f, 1.f), FVector2D(1.f, 1.f));
		AddTri(EavesFL, EavesBR, EavesFR, FVector2D(0.f, 0.f), FVector2D(1.f, 1.f), FVector2D(1.f, 0.f));

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

		const FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
		PolygonGroupNames[PolygonGroupID] = FName(TEXT("Roof"));

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
				InstanceColors[InstanceID] = FVector4f(Colors[VertIndex]);
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

		const FName SlotName(TEXT("Roof"));
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
			UE_LOG(LogSolid, Error, TEXT("SolidBuilding: failed to build gable prism roof mesh."));
			return nullptr;
		}

		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		return Mesh;
	}
}

ASolidBuilding::ASolidBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BodyMesh->SetCastShadow(true);

	RoofMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoofMesh"));
	RoofMesh->SetupAttachment(SceneRoot);
	RoofMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RoofMesh->SetCollisionResponseToAllChannels(ECR_Block);
	RoofMesh->SetCastShadow(true);
}

void ASolidBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidBuilding::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidBuilding::ApplyRandomVariation(FRandomStream& Rng)
{
	FootprintXCm = Rng.FRandRange(180.f, 420.f);
	FootprintYCm = Rng.FRandRange(160.f, 380.f);
	BodyHeightCm = Rng.FRandRange(220.f, 520.f);
	RoofHeightCm = Rng.FRandRange(90.f, 220.f);

	const float Grey = Rng.FRandRange(-0.04f, 0.05f);
	BodyColor = FLinearColor(
		FMath::Clamp(0.40f + Grey, 0.28f, 0.52f),
		FMath::Clamp(0.40f + Grey, 0.28f, 0.52f),
		FMath::Clamp(0.42f + Grey, 0.30f, 0.55f));

	const float RedTint = Rng.FRandRange(-0.06f, 0.08f);
	RoofColor = FLinearColor(
		FMath::Clamp(0.55f + RedTint, 0.40f, 0.70f),
		FMath::Clamp(0.10f + RedTint * 0.2f, 0.04f, 0.18f),
		FMath::Clamp(0.08f + RedTint * 0.1f, 0.03f, 0.14f));
}

void ASolidBuilding::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidBuilding: missing Engine BasicShapes Cube."));
		return;
	}

	// Engine cube is ~100cm with pivot at center.
	constexpr float ShapeSize = 100.f;
	constexpr float RoofOverhang = 1.08f;

	BodyMesh->SetStaticMesh(Cube);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, BodyHeightCm * 0.5f));
	BodyMesh->SetRelativeScale3D(FVector(
		FootprintXCm / ShapeSize,
		FootprintYCm / ShapeSize,
		BodyHeightCm / ShapeSize));
	if (UMaterialInterface* BodyMat =
			SolidMaterials::CreateSolidColor(this, BodyColor, TEXT("BuildingBody"), 0.92f))
	{
		BodyMesh->SetMaterial(0, BodyMat);
	}

	// Unit gable prism: 100x100 base, 100 tall, ridge along local Y. Scale to footprint + overhang.
	UStaticMesh* Roof = BuildUnitGablePrismMesh(this);
	if (!Roof)
	{
		return;
	}

	RoofMesh->SetStaticMesh(Roof);
	// Mesh base sits at z=0; place that plane on top of the body.
	RoofMesh->SetRelativeLocation(FVector(0.f, 0.f, BodyHeightCm));
	const bool bRidgeAlongY = FootprintYCm >= FootprintXCm;
	if (bRidgeAlongY)
	{
		RoofMesh->SetRelativeRotation(FRotator::ZeroRotator);
		RoofMesh->SetRelativeScale3D(FVector(
			(FootprintXCm * RoofOverhang) / ShapeSize,
			(FootprintYCm * RoofOverhang) / ShapeSize,
			RoofHeightCm / ShapeSize));
	}
	else
	{
		// Rotate so the ridge follows the longer X axis.
		RoofMesh->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		RoofMesh->SetRelativeScale3D(FVector(
			(FootprintYCm * RoofOverhang) / ShapeSize,
			(FootprintXCm * RoofOverhang) / ShapeSize,
			RoofHeightCm / ShapeSize));
	}
	if (UMaterialInterface* RoofMat =
			SolidMaterials::CreateSolidColor(this, RoofColor, TEXT("BuildingRoof"), 0.90f))
	{
		RoofMesh->SetMaterial(0, RoofMat);
	}
}
