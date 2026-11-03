#include "SolidTree.h"
#include "SolidCore1.h"
#include "SolidFabManifest.h"
#include "Terrain/SolidTerrainSeat.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	const TCHAR* TreeFileNames[ASolidTree::MeshCount] = {
		TEXT("Tree1.uasset"),
		TEXT("Tree2.uasset"),
		TEXT("Tree3.uasset"),
	};

	UStaticMesh* LoadStylizedTree(const int32 Index)
	{
		const int32 Chosen = FMath::Clamp(Index, 0, ASolidTree::MeshCount - 1);
		const FString Manifest = SolidFabManifest::LoadManifest();
		FString Recorded;
		if (SolidFabManifest::ReadJsonStringField(
			Manifest, TEXT("stylized-environment"), TreeFileNames[Chosen], Recorded))
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
		const FString Fallback = Folder / TEXT("Meshes") / TreeFileNames[Chosen];
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(
			SolidFabManifest::TryLoadContentFile(Fallback, UStaticMesh::StaticClass())))
		{
			SolidFabManifest::RememberResolvedPath(TEXT("stylized-environment"), Mesh);
			return Mesh;
		}
		return nullptr;
	}
}

ASolidTree::ASolidTree()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(SceneRoot);
	TrunkMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TrunkMesh->SetCollisionResponseToAllChannels(ECR_Block);
	TrunkMesh->SetCastShadow(true);

	CanopyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CanopyMesh"));
	CanopyMesh->SetupAttachment(SceneRoot);
	CanopyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CanopyMesh->SetVisibility(false);
	CanopyMesh->SetCastShadow(false);
}

void ASolidTree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidTree::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidTree::ApplyRandomVariation(FRandomStream& Rng)
{
	MeshIndex = Rng.RandRange(0, MeshCount - 1);
}

void ASolidTree::BuildVisuals()
{
	if (CanopyMesh)
	{
		CanopyMesh->SetStaticMesh(nullptr);
		CanopyMesh->SetVisibility(false);
		CanopyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (!TrunkMesh)
	{
		return;
	}

	UStaticMesh* Mesh = LoadStylizedTree(MeshIndex);
	if (!Mesh)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidTree: stylized tree mesh %d missing."), MeshIndex);
		return;
	}

	TrunkMesh->SetStaticMesh(Mesh);
	TrunkMesh->SetRelativeScale3D(FVector::OneVector);
	TrunkMesh->EmptyOverrideMaterials();

	const FBox Box = Mesh->GetBounds().GetBox();
	const float Radius = FMath::Max(
		FMath::Max(FMath::Abs(Box.Min.X), FMath::Abs(Box.Max.X)),
		FMath::Max(FMath::Abs(Box.Min.Y), FMath::Abs(Box.Max.Y)));
	SolidTerrainSeat::FFootprint Seat;
	Seat.bRound = true;
	Seat.HalfXCm = Radius;
	const float Drop = SolidTerrainSeat::DropCm(this, Seat);
	TrunkMesh->SetRelativeLocation(FVector(0.f, 0.f, -Drop - Box.Min.Z));

	static bool bLoggedSize = false;
	if (!bLoggedSize)
	{
		bLoggedSize = true;
		UE_LOG(LogSolid, Warning, TEXT("SolidTree: %s bounds Z %.0f to %.0f, radius %.0f"),
			*Mesh->GetName(), Box.Min.Z, Box.Max.Z, Radius);
	}
}
