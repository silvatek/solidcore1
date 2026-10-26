#include "SolidFireplace.h"

#include "SolidCore1.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

namespace
{
	/** Pointy-top prism. Flat-to-flat 100, height 100, bottom on Z = 0. */
	UStaticMesh* BuildHearthPrism(UObject* Outer)
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
			UE_LOG(LogSolid, Error, TEXT("SolidFireplace: failed to build the hearth."));
			return nullptr;
		}
		return Mesh;
	}
}

ASolidFireplace::ASolidFireplace()
{
	Hearth = MakePart(TEXT("Hearth"));
	Embers = MakePart(TEXT("Embers"));
}

SolidStrongholdFurniture::EKind ASolidFireplace::GetKind() const
{
	return SolidStrongholdFurniture::EKind::Fireplace;
}

float ASolidFireplace::AcrossCm() const
{
	return SolidStrongholdGrid::FlatToFlatCm * 0.90f;
}

float ASolidFireplace::HearthCm() const
{
	return 30.f;
}

float ASolidFireplace::EmberRadiusCm() const
{
	return AcrossCm() * 0.40f;
}

float ASolidFireplace::EmberHeightCm() const
{
	// Was 20 cm. About 60 percent of that height is gone.
	return 8.f;
}

FLinearColor ASolidFireplace::StoneColor() const
{
	return FLinearColor(0.45f, 0.45f, 0.47f);
}

FLinearColor ASolidFireplace::EmberColor() const
{
	return FLinearColor(0.72f, 0.12f, 0.06f);
}

void ASolidFireplace::BuildVisuals()
{
	const float Lift = FloorLiftCm();
	PlacePart(
		Hearth,
		BuildHearthPrism(this),
		FVector(0.f, 0.f, Lift),
		FRotator::ZeroRotator,
		FVector(AcrossCm(), AcrossCm(), HearthCm()),
		FVector(100.f, 100.f, 100.f),
		StoneColor(),
		TEXT("FireplaceHearth"));
	PlacePart(
		Embers,
		BasicCylinder(),
		FVector(0.f, 0.f, Lift + HearthCm() + EmberHeightCm() * 0.5f),
		FRotator::ZeroRotator,
		FVector(EmberRadiusCm() * 2.f, EmberRadiusCm() * 2.f, EmberHeightCm()),
		FVector(100.f, 100.f, 100.f),
		EmberColor(),
		TEXT("FireplaceEmber"));
}
