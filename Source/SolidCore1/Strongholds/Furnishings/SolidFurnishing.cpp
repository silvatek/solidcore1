#include "SolidFurnishing.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholdGrid.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

namespace
{
	void PlaceFurnishingPart(
		UStaticMeshComponent* Part,
		UStaticMesh* Mesh,
		AActor* Outer,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& SizeCm,
		const FVector& MeshSizeCm,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		if (!Part || !Mesh)
		{
			return;
		}

		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(Rotation);
		Part->SetRelativeScale3D(FVector(
			SizeCm.X / MeshSizeCm.X,
			SizeCm.Y / MeshSizeCm.Y,
			SizeCm.Z / MeshSizeCm.Z));
		Part->SetVisibility(true);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Outer, Color, DebugName, 0.85f))
		{
			Part->SetMaterial(0, Material);
		}
	}

	/** Pointy-top prism. Flat-to-flat 100, height 100, bottom on Z = 0. */
	UStaticMesh* BuildFurnishingHexPrism(UObject* Outer)
	{
		const float Radius = 100.f / FMath::Sqrt(3.f);
		FVector Bottom[6];
		FVector Top[6];
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(30.f + 60.f * static_cast<float>(Index));
			const float X = Radius * FMath::Cos(Angle);
			const float Y = Radius * FMath::Sin(Angle);
			Bottom[Index] = FVector(X, Y, 0.f);
			Top[Index] = FVector(X, Y, 100.f);
		}

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

		const FVector CenterTop(0.f, 0.f, 100.f);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const int32 Next = (Index + 1) % 6;
			// Clockwise from the outside, matching the town roof (UE shows that winding).
			AddTri(Bottom[Index], Top[Index], Top[Next]);
			AddTri(Bottom[Index], Top[Next], Bottom[Next]);
			AddTri(CenterTop, Top[Index], Top[(Index + 5) % 6]);
		}

		FMeshDescription MeshDescription;
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();

		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> InstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
		TPolygonGroupAttributesRef<FName> PolygonGroupNames = Attributes.GetPolygonGroupMaterialSlotNames();
		InstanceUVs.SetNumChannels(1);

		const FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
		const FName SlotName(TEXT("Furnishing"));
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
			UE_LOG(LogSolid, Error, TEXT("SolidFurnishing: failed to build the hex prism."));
			return nullptr;
		}
		return Mesh;
	}
}

ASolidFurnishing::ASolidFurnishing()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	TopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopMesh"));
	UStaticMeshComponent* Parts[] = { BodyMesh.Get(), TopMesh.Get() };
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetupAttachment(SceneRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
	}
}

void ASolidFurnishing::SetKind(const SolidStrongholdFurniture::EKind InKind)
{
	Kind = InKind;
}

void ASolidFurnishing::SetPlacement(const SolidStrongholdFurniture::FUsed& InPlacement)
{
	Placement = InPlacement;
	bHasPlacement = true;
}

void ASolidFurnishing::SetGhost(const bool bInGhost)
{
	if (bInGhost == bGhostLook)
	{
		return;
	}
	bGhostLook = bInGhost;
	if (!bGhostLook)
	{
		BuildVisuals();
		return;
	}

	auto Wash = [](const FLinearColor& Color)
	{
		return FMath::Lerp(Color, FLinearColor::White, 0.62f);
	};
	FLinearColor Body = SolidStrongholdFurniture::BeigeColor();
	FLinearColor Top = Body;
	switch (Kind)
	{
	case SolidStrongholdFurniture::EKind::Fireplace:
		Body = SolidStrongholdFurniture::StoneColor();
		Top = SolidStrongholdFurniture::EmberColor();
		break;
	case SolidStrongholdFurniture::EKind::Bed:
		Body = SolidStrongholdFurniture::BeigeColor();
		Top = SolidStrongholdFurniture::PillowColor();
		break;
	case SolidStrongholdFurniture::EKind::Chest:
	default:
		Body = SolidStrongholdFurniture::BeigeColor();
		Top = SolidStrongholdFurniture::BeigeColor();
		break;
	}
	if (BodyMesh)
	{
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			this, Wash(Body), TEXT("ArrangeGhost"), 0.85f))
		{
			BodyMesh->SetMaterial(0, Material);
		}
	}
	if (TopMesh)
	{
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			this, Wash(Top), TEXT("ArrangeGhostTop"), 0.85f))
		{
			TopMesh->SetMaterial(0, Material);
		}
	}
}

void ASolidFurnishing::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidFurnishing::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidFurnishing::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidFurnishing: missing Engine BasicShapes Cube."));
		return;
	}

	const float Lift = SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm;

	switch (Kind)
	{
	case SolidStrongholdFurniture::EKind::Fireplace:
	{
		UStaticMesh* Hex = BuildFurnishingHexPrism(this);
		PlaceFurnishingPart(
			BodyMesh,
			Hex,
			this,
			FVector(0.f, 0.f, Lift),
			FRotator::ZeroRotator,
			FVector(
				SolidStrongholdFurniture::FireplaceAcrossCm(),
				SolidStrongholdFurniture::FireplaceAcrossCm(),
				SolidStrongholdFurniture::FireplaceHearthCm()),
			FVector(100.f, 100.f, 100.f),
			SolidStrongholdFurniture::StoneColor(),
			TEXT("FireplaceHearth"));
		PlaceFurnishingPart(
			TopMesh,
			Cylinder,
			this,
			FVector(0.f, 0.f, Lift + SolidStrongholdFurniture::FireplaceHearthCm()
				+ SolidStrongholdFurniture::FireplaceEmberHeightCm() * 0.5f),
			FRotator::ZeroRotator,
			FVector(
				SolidStrongholdFurniture::FireplaceEmberRadiusCm() * 2.f,
				SolidStrongholdFurniture::FireplaceEmberRadiusCm() * 2.f,
				SolidStrongholdFurniture::FireplaceEmberHeightCm()),
			FVector(100.f, 100.f, 100.f),
			SolidStrongholdFurniture::EmberColor(),
			TEXT("FireplaceEmber"));
		break;
	}
	case SolidStrongholdFurniture::EKind::Bed:
	{
		const float Length = SolidStrongholdFurniture::BedLengthCm();
		const float Width = SolidStrongholdFurniture::BedWidthCm();
		const float Height = SolidStrongholdFurniture::BedHeightCm();
		const float PillowLength = SolidStrongholdFurniture::PillowLengthCm();
		const float PillowWidth = SolidStrongholdFurniture::PillowWidthCm();
		const float PillowHeight = SolidStrongholdFurniture::PillowHeightCm();
		PlaceFurnishingPart(
			BodyMesh,
			Cube,
			this,
			FVector(0.f, 0.f, Lift + Height * 0.5f),
			FRotator::ZeroRotator,
			FVector(Length, Width, Height),
			FVector(100.f, 100.f, 100.f),
			SolidStrongholdFurniture::BeigeColor(),
			TEXT("BedBody"));
		PlaceFurnishingPart(
			TopMesh,
			Cube,
			this,
			FVector(Length * 0.5f - PillowLength * 0.5f - 8.f, 0.f, Lift + Height + PillowHeight * 0.5f),
			FRotator::ZeroRotator,
			FVector(PillowLength, PillowWidth, PillowHeight),
			FVector(100.f, 100.f, 100.f),
			SolidStrongholdFurniture::PillowColor(),
			TEXT("BedPillow"));
		break;
	}
	case SolidStrongholdFurniture::EKind::Chest:
	default:
	{
		const float Length = SolidStrongholdFurniture::ChestLengthCm();
		const float Width = SolidStrongholdFurniture::ChestWidthCm();
		const float Height = SolidStrongholdFurniture::ChestBodyHeightCm();
		const float LidRadius = SolidStrongholdFurniture::ChestLidRadiusCm();
		PlaceFurnishingPart(
			BodyMesh,
			Cube,
			this,
			FVector(0.f, 0.f, Lift + Height * 0.5f),
			FRotator::ZeroRotator,
			FVector(Length, Width, Height),
			FVector(100.f, 100.f, 100.f),
			SolidStrongholdFurniture::BeigeColor(),
			TEXT("ChestBody"));
		// Engine cylinder axis is local Z. Turn that axis along the chest so the dome is the lid.
		const FRotator LidRotation = FRotationMatrix::MakeFromZ(FVector::ForwardVector).Rotator();
		PlaceFurnishingPart(
			TopMesh,
			Cylinder,
			this,
			FVector(0.f, 0.f, Lift + Height),
			LidRotation,
			FVector(LidRadius * 2.f, LidRadius * 2.f, Length),
			FVector(100.f, 100.f, 100.f),
			SolidStrongholdFurniture::BeigeColor(),
			TEXT("ChestLid"));
		break;
	}
	}
}
