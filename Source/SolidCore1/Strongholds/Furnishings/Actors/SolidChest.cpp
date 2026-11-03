#include "SolidChest.h"

#include "SolidCore1.h"
#include "SolidFabManifest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	UStaticMesh* LoadChestMesh()
	{
		const FString Manifest = SolidFabManifest::LoadManifest();
		FString Recorded;
		if (SolidFabManifest::ReadJsonStringField(
			Manifest, TEXT("stylized-environment"), TEXT("Chest_Static.uasset"), Recorded))
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
		const FString Fallback = Folder / TEXT("Meshes") / TEXT("Chest_Static.uasset");
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(
			SolidFabManifest::TryLoadContentFile(Fallback, UStaticMesh::StaticClass())))
		{
			SolidFabManifest::RememberResolvedPath(TEXT("stylized-environment"), Mesh);
			return Mesh;
		}
		return nullptr;
	}
}

ASolidChest::ASolidChest()
{
	ChestMesh = MakePart(TEXT("Chest"));
	Lid = MakePart(TEXT("Lid"));
}

float ASolidChest::LengthCm() const
{
	return 76.f;
}

float ASolidChest::WidthCm() const
{
	return 48.f;
}

float ASolidChest::BodyHeightCm() const
{
	return 36.f;
}

float ASolidChest::LidRadiusCm() const
{
	return WidthCm() * 0.5f;
}

void ASolidChest::BuildVisuals()
{
	if (Lid)
	{
		Lid->SetStaticMesh(nullptr);
		Lid->SetVisibility(false);
		Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (!ChestMesh)
	{
		return;
	}

	UStaticMesh* Mesh = LoadChestMesh();
	if (!Mesh)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidChest: chest mesh missing."));
		return;
	}

	const FBox Box = Mesh->GetBounds().GetBox();
	ChestMesh->SetStaticMesh(Mesh);
	ChestMesh->SetRelativeScale3D(FVector::OneVector);
	ChestMesh->SetRelativeRotation(FRotator::ZeroRotator);
	ChestMesh->SetRelativeLocation(FVector(0.f, 0.f, FloorLiftCm() - Box.Min.Z));
	ChestMesh->EmptyOverrideMaterials();
	ChestMesh->SetVisibility(true);
	ChestMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
