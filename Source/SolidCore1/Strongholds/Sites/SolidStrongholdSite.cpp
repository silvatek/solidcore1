#include "SolidStrongholdSite.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Terrain/SolidTerrainSeat.h"
#include "Strongholds/SolidStronghold.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldChart.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"

namespace SolidStrongholdSitePrivate
{
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
	 * Square truncated pyramid. Z is 0 at the wide foot and Height at the smaller top.
	 * Sides are wound clockwise from the outside, the same way the terrain is.
	 * BurialCm drops a vertical skirt under the foot. The taper above the foot stays.
	 */
	UStaticMesh* BuildPyramid(
		UObject* Outer,
		const float BottomEdge,
		const float TopEdge,
		const float Height,
		const float BurialCm)
	{
		if (!Outer
			|| Height <= KINDA_SMALL_NUMBER
			|| BottomEdge <= KINDA_SMALL_NUMBER
			|| TopEdge <= KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		const float BottomHalf = BottomEdge * 0.5f;
		const float TopHalf = TopEdge * 0.5f;
		const FVector2D Corner[4] = {
			FVector2D(1.f, 1.f),
			FVector2D(-1.f, 1.f),
			FVector2D(-1.f, -1.f),
			FVector2D(1.f, -1.f),
		};
		const float Slope = (TopHalf - BottomHalf) / Height;
		const float NormalZ = -Slope;
		const float NormalLength = FMath::Sqrt(1.f + NormalZ * NormalZ);

		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<int32> Triangles;
		Positions.Reserve(48);
		Normals.Reserve(48);
		Triangles.Reserve(48);

		for (int32 Side = 0; Side < 4; ++Side)
		{
			const int32 Next = (Side + 1) % 4;
			const FVector2D Dir = (Corner[Side] + Corner[Next]).GetSafeNormal();
			const FVector Bottom0(Corner[Side].X * BottomHalf, Corner[Side].Y * BottomHalf, 0.f);
			const FVector Bottom1(Corner[Next].X * BottomHalf, Corner[Next].Y * BottomHalf, 0.f);
			const FVector Top0(Corner[Side].X * TopHalf, Corner[Side].Y * TopHalf, Height);
			const FVector Top1(Corner[Next].X * TopHalf, Corner[Next].Y * TopHalf, Height);
			const FVector SideNormal(Dir.X / NormalLength, Dir.Y / NormalLength, NormalZ / NormalLength);

			AddCorner(Positions, Normals, Triangles, Bottom1, Bottom0, Top0, SideNormal);
			AddCorner(Positions, Normals, Triangles, Bottom1, Top0, Top1, SideNormal);

			if (BurialCm > 0.f)
			{
				const FVector Buried0(Bottom0.X, Bottom0.Y, -BurialCm);
				const FVector Buried1(Bottom1.X, Bottom1.Y, -BurialCm);
				const FVector SkirtNormal(Dir.X, Dir.Y, 0.f);
				AddCorner(Positions, Normals, Triangles, Buried1, Buried0, Bottom0, SkirtNormal);
				AddCorner(Positions, Normals, Triangles, Buried1, Bottom0, Bottom1, SkirtNormal);
			}
		}

		const float FootZ = BurialCm > 0.f ? -BurialCm : 0.f;
		const FVector Down(0.f, 0.f, -1.f);
		const FVector FootCenter(0.f, 0.f, FootZ);
		const FVector TopCenter(0.f, 0.f, Height);
		const FVector Up(0.f, 0.f, 1.f);
		for (int32 Side = 0; Side < 4; ++Side)
		{
			const int32 Next = (Side + 1) % 4;
			const FVector Rim0(Corner[Side].X * BottomHalf, Corner[Side].Y * BottomHalf, FootZ);
			const FVector Rim1(Corner[Next].X * BottomHalf, Corner[Next].Y * BottomHalf, FootZ);
			AddCorner(Positions, Normals, Triangles, FootCenter, Rim0, Rim1, Down);

			const FVector Cap0(Corner[Side].X * TopHalf, Corner[Side].Y * TopHalf, Height);
			const FVector Cap1(Corner[Next].X * TopHalf, Corner[Next].Y * TopHalf, Height);
			AddCorner(Positions, Normals, Triangles, TopCenter, Cap1, Cap0, Up);
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
			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				const int32 Vert = Triangles[Tri + CornerIndex];
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
			UE_LOG(LogSolid, Error, TEXT("SolidStrongholdSite: failed to build the pyramid."));
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

ASolidStrongholdSite::ASolidStrongholdSite()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Block"));
	BlockMesh->SetupAttachment(SceneRoot);
	BlockMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ASolidStrongholdSite::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidStrongholdSite::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidStrongholdSite::SetSite(const FIntPoint InTerrainPoint, const ESolidStrongholdSize InSize)
{
	TerrainPoint = InTerrainPoint;
	Size = InSize;
}

FString ASolidStrongholdSite::Hover() const
{
	return FString::Printf(TEXT("Build %s stronghold"), SolidStronghold::SizeName(Size));
}

void ASolidStrongholdSite::BuildVisuals()
{
	if (!BlockMesh)
	{
		return;
	}

	SolidTerrainSeat::FFootprint Seat;
	Seat.HalfXCm = BlockCm * 0.5f;
	Seat.HalfYCm = BlockCm * 0.5f;
	const float Drop = SolidTerrainSeat::DropCm(this, Seat);
	UStaticMesh* Pyramid = SolidStrongholdSitePrivate::BuildPyramid(
		this,
		BlockCm,
		BlockCm * TopScale,
		BlockCm,
		Drop);
	if (!Pyramid)
	{
		return;
	}

	BlockMesh->SetStaticMesh(Pyramid);
	BlockMesh->SetRelativeLocation(FVector::ZeroVector);
	BlockMesh->SetRelativeRotation(FRotator::ZeroRotator);
	BlockMesh->SetRelativeScale3D(FVector::OneVector);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
		this, SolidStrongholds::RockColor(), TEXT("StrongholdSite"), 0.94f))
	{
		BlockMesh->SetMaterial(0, Material);
	}
}

void ASolidStrongholdSite::Spawn(
	UWorld* World,
	ASolidTerrainStreamer* Streamer,
	const TArray<const FSolidStronghold*>& Company,
	const FActorSpawnParameters& SpawnParams,
	TArray<TObjectPtr<ASolidStrongholdSite>>& Sites,
	bool& bSitesSpawned)
{
	if (!World || !Streamer || !Streamer->GetTerrainMap() || !Streamer->GetTerrainMap()->IsBuilt())
	{
		return;
	}

	Sites.RemoveAll([](const TObjectPtr<ASolidStrongholdSite>& Site)
	{
		return !IsValid(Site);
	});

	USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
	USolidWorldChart* Map = Terrain->GetWorldMap();
	if (bSitesSpawned || !Map || !Map->IsLoaded())
	{
		return;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	bSitesSpawned = true;
	int32 SpawnedSites = 0;
	for (uint8 SizeValue = static_cast<uint8>(ESolidStrongholdSize::Meagre);
		SizeValue <= static_cast<uint8>(ESolidStrongholdSize::Imperious);
		++SizeValue)
	{
		const ESolidStrongholdSize SiteSize = static_cast<ESolidStrongholdSize>(SizeValue);
		FSolidWorldChartMark Mark;
		if (!Map->FindStrongholdSiteMarker(SiteSize, Mark))
		{
			continue;
		}

		FVector2D SiteXY = FVector2D::ZeroVector;
		if (!Map->TerrainPointToWorld(Mark.TerrainPoint, WorldMin, WorldMax, SiteXY))
		{
			continue;
		}

			const FSolidStronghold* Owned = nullptr;
			for (const FSolidStronghold* Stronghold : Company)
			{
				if (Stronghold && Stronghold->Cell == Mark.TerrainPoint)
				{
					Owned = Stronghold;
					break;
				}
			}
			if (Owned && Owned->ExteriorClass())
			{
				continue;
			}

			FVector SpawnLoc(SiteXY.X, SiteXY.Y, 0.f);
			const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
			SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;
			ASolidStrongholdSite* Site = World->SpawnActor<ASolidStrongholdSite>(
				ASolidStrongholdSite::StaticClass(),
				SpawnLoc,
				FRotator::ZeroRotator,
				SpawnParams);
			if (!Site)
			{
				continue;
			}
			Site->SetSite(Mark.TerrainPoint, SiteSize);
			Site->BuildVisuals();
			Sites.Add(Site);
			++SpawnedSites;
	}
	UE_LOG(LogSolid, Warning, TEXT("Spawned %d stronghold sites"), SpawnedSites);
}
