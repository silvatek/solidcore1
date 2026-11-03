#include "SolidFireplace.h"

#include "SolidCore1.h"
#include "SolidFabManifest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	UStaticMesh* LoadCampfireMesh()
	{
		const FString Manifest = SolidFabManifest::LoadManifest();
		FString Recorded;
		if (SolidFabManifest::ReadJsonStringField(
			Manifest, TEXT("stylized-environment"), TEXT("Campfire.uasset"), Recorded))
		{
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(
				SolidFabManifest::TryLoadContentFile(Recorded, UStaticMesh::StaticClass())))
			{
				return Mesh;
			}
		}

		FString Folder;
		SolidFabManifest::ReadJsonStringField(
			Manifest, TEXT("stylized-environment"), TEXT("resolvedFolder"), Folder);
		if (Folder.IsEmpty())
		{
			Folder = TEXT("Content/StylizedEnvironment");
		}
		const FString Fallback = Folder / TEXT("Meshes") / TEXT("Campfire.uasset");
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(
			SolidFabManifest::TryLoadContentFile(Fallback, UStaticMesh::StaticClass())))
		{
			SolidFabManifest::RememberResolvedPath(TEXT("stylized-environment"), Mesh);
			return Mesh;
		}
		return nullptr;
	}
}

ASolidFireplace::ASolidFireplace()
{
	Hearth = MakePart(TEXT("Hearth"));
	Embers = MakePart(TEXT("Embers"));
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
	return FLinearColor(0.78f, 0.78f, 0.80f);
}

FLinearColor ASolidFireplace::EmberColor() const
{
	return FLinearColor(0.72f, 0.12f, 0.06f);
}

void ASolidFireplace::BuildVisuals()
{
	if (Embers)
	{
		Embers->SetStaticMesh(nullptr);
		Embers->SetVisibility(false);
		Embers->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (!Hearth)
	{
		return;
	}

	UStaticMesh* Mesh = LoadCampfireMesh();
	if (!Mesh)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidFireplace: campfire mesh missing."));
		return;
	}

	const FBox Box = Mesh->GetBounds().GetBox();
	Hearth->SetStaticMesh(Mesh);
	Hearth->SetRelativeScale3D(FVector::OneVector);
	Hearth->SetRelativeRotation(FRotator::ZeroRotator);
	Hearth->SetRelativeLocation(FVector(0.f, 0.f, FloorLiftCm() - Box.Min.Z));
	Hearth->EmptyOverrideMaterials();
	Hearth->SetVisibility(true);
	Hearth->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
