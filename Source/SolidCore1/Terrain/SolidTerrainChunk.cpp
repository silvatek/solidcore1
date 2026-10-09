#include "SolidTerrainChunk.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainNoise.h"
#include "SolidTerrainTypes.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "Components/StaticMeshComponent.h"

namespace SolidTerrainChunkPrivate
{
	static void ConfigureCollision(UStaticMeshComponent* Mesh)
	{
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Mesh->SetCollisionObjectType(ECC_WorldStatic);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
		Mesh->SetGenerateOverlapEvents(false);
	}

	static void ConfigureFogOverlay(UStaticMeshComponent* Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCastShadow(false);
		Mesh->SetVisibility(true);
		Mesh->SetHiddenInGame(false);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->bUseAsOccluder = false;
		Mesh->SetCanEverAffectNavigation(false);
	}

	static UStaticMesh* BuildRuntimeMesh(
		UObject* Outer,
		const TArray<FVector>& Positions,
		const TArray<FVector>& Normals,
		const TArray<FVector>& Tangents,
		const TArray<FVector2D>& UVs,
		const TArray<FLinearColor>& Colors,
		const TArray<int32>& Triangles,
		const TArray<UMaterialInterface*>& Materials,
		const TArray<int32>& TriMaterialIndices,
		bool bBuildCollision)
	{
		if (Positions.Num() == 0 || Triangles.Num() < 3 || Materials.Num() == 0)
		{
			return nullptr;
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

		TArray<FPolygonGroupID> PolygonGroups;
		PolygonGroups.Reserve(Materials.Num());
		for (int32 MatIndex = 0; MatIndex < Materials.Num(); ++MatIndex)
		{
			const FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
			PolygonGroupNames[PolygonGroupID] = FName(*FString::Printf(TEXT("Slot%d"), MatIndex));
			PolygonGroups.Add(PolygonGroupID);
		}

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
			const int32 I0 = Triangles[TriIndex * 3 + 0];
			const int32 I1 = Triangles[TriIndex * 3 + 1];
			const int32 I2 = Triangles[TriIndex * 3 + 2];
			const int32 MatIndex = TriMaterialIndices.IsValidIndex(TriIndex)
				? FMath::Clamp(TriMaterialIndices[TriIndex], 0, PolygonGroups.Num() - 1)
				: 0;

			TArray<FVertexInstanceID, TInlineAllocator<3>> InstanceIDs;
			const int32 CornerIndices[3] = { I0, I1, I2 };
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 VertIndex = CornerIndices[Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[VertIndex]);
				InstanceNormals[InstanceID] = FVector3f(Normals[VertIndex]);
				InstanceTangents[InstanceID] = FVector3f(Tangents[VertIndex]);
				InstanceBinormalSigns[InstanceID] = 1.f;
				InstanceUVs.Set(InstanceID, 0, FVector2f(UVs.IsValidIndex(VertIndex) ? UVs[VertIndex] : FVector2D::ZeroVector));
				InstanceColors[InstanceID] = FVector4f(
					Colors.IsValidIndex(VertIndex) ? Colors[VertIndex] : FLinearColor::White);
				InstanceIDs.Add(InstanceID);
			}

			MeshDescription.CreatePolygon(PolygonGroups[MatIndex], InstanceIDs);
		}

		UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer, NAME_None, RF_Transient);
		Mesh->bAllowCPUAccess = true;
		Mesh->NeverStream = true;
		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		TArray<FStaticMaterial> StaticMaterials;
		StaticMaterials.Reserve(Materials.Num());
		for (int32 MatIndex = 0; MatIndex < Materials.Num(); ++MatIndex)
		{
			const FName SlotName(*FString::Printf(TEXT("Slot%d"), MatIndex));
			StaticMaterials.Add(FStaticMaterial(Materials[MatIndex], SlotName, SlotName));
		}
		Mesh->SetStaticMaterials(StaticMaterials);

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		// Fog overlays skip collision — fast build avoids the expensive path.
		BuildParams.bFastBuild = !bBuildCollision;
		BuildParams.bAllowCpuAccess = bBuildCollision;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = false;

		const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(Descriptions, BuildParams))
		{
			return nullptr;
		}

		{
			FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
			NaniteSettings.bEnabled = false;
			Mesh->SetNaniteSettings(NaniteSettings);
		}

		if (bBuildCollision)
		{
			if (!Mesh->GetBodySetup())
			{
				Mesh->CreateBodySetup();
			}
			if (UBodySetup* BodySetup = Mesh->GetBodySetup())
			{
				BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
				BodySetup->bDoubleSidedGeometry = true;
				BodySetup->InvalidatePhysicsData();
				BodySetup->CreatePhysicsMeshes();
			}
		}

		return Mesh;
	}
}

ASolidTerrainChunk::ASolidTerrainChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	FogMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FogMeshComponent"));
	FogMeshComponent->SetupAttachment(MeshComponent);

	SolidTerrainChunkPrivate::ConfigureCollision(MeshComponent);
	SolidTerrainChunkPrivate::ConfigureFogOverlay(FogMeshComponent);
	MeshComponent->SetCastShadow(true);
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->bUseAsOccluder = false;
	MeshComponent->bTreatAsBackgroundForOcclusion = true;
	MeshComponent->LDMaxDrawDistance = 0.f;
	MeshComponent->bAllowCullDistanceVolume = false;
	MeshComponent->SetCanEverAffectNavigation(false);
}

void ASolidTerrainChunk::BuildChunk(
	FIntPoint InChunkCoord,
	float InChunkWorldSize,
	int32 InQuadsPerSide,
	int32 InSeed,
	float InFrequencyScale,
	float InAmplitude,
	float InBaseHeight,
	float InCollisionHeightBias,
	UMaterialInterface* Material,
	UMaterialInterface* FogHalfMaterial,
	UMaterialInterface* FogFullMaterial,
	const USolidTerrainMap* TerrainMap,
	int32 InFogQuadsPerSide)
{
	ChunkCoord = InChunkCoord;
	InQuadsPerSide = FMath::Clamp(InQuadsPerSide, 1, 256);
	InChunkWorldSize = FMath::Max(InChunkWorldSize, 100.f);
	InCollisionHeightBias = FMath::Max(InCollisionHeightBias, 0.f);
	CachedChunkWorldSize = InChunkWorldSize;
	CachedCollisionHeightBias = InCollisionHeightBias;
	CachedFogQuadsPerSide = FMath::Clamp(InFogQuadsPerSide, 1, 64);

	const int32 VertsPerSide = InQuadsPerSide + 1;
	const float Step = InChunkWorldSize / static_cast<float>(InQuadsPerSide);
	const float OriginX = static_cast<float>(InChunkCoord.X) * InChunkWorldSize;
	const float OriginY = static_cast<float>(InChunkCoord.Y) * InChunkWorldSize;

	SetActorLocation(FVector(OriginX, OriginY, 0.f));

	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<FVector> Tangents;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<int32> Triangles;

	Positions.Reserve(VertsPerSide * VertsPerSide);
	Normals.Reserve(VertsPerSide * VertsPerSide);
	Tangents.Reserve(VertsPerSide * VertsPerSide);
	UVs.Reserve(VertsPerSide * VertsPerSide);
	Colors.Reserve(VertsPerSide * VertsPerSide);
	Triangles.Reserve(InQuadsPerSide * InQuadsPerSide * 6);

	TArray<float> Heights;
	Heights.SetNumUninitialized(VertsPerSide * VertsPerSide);

	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();

	for (int32 Y = 0; Y < VertsPerSide; ++Y)
	{
		for (int32 X = 0; X < VertsPerSide; ++X)
		{
			const float WorldX = OriginX + static_cast<float>(X) * Step;
			const float WorldY = OriginY + static_cast<float>(Y) * Step;
			const float Height = (TerrainMap && TerrainMap->IsBuilt())
				? TerrainMap->SampleHeight(WorldX, WorldY)
				: SolidTerrainNoise::SampleHeight(
					WorldX, WorldY, InSeed, InFrequencyScale, InAmplitude, InBaseHeight);
			Heights[Y * VertsPerSide + X] = Height;

			const float SurfaceZ = Height + InCollisionHeightBias;
			MinZ = FMath::Min(MinZ, SurfaceZ);
			MaxZ = FMath::Max(MaxZ, SurfaceZ);

			Positions.Add(FVector(static_cast<float>(X) * Step, static_cast<float>(Y) * Step, SurfaceZ));

			const float GrassTone = SolidTerrainNoise::SampleGrassTone(WorldX, WorldY, InSeed);
			const float UVScale = 0.01f;
			UVs.Add(FVector2D(WorldX * UVScale, WorldY * UVScale));

			const float HeightT = FMath::Clamp((Height - InBaseHeight) / FMath::Max(InAmplitude, 1.f), 0.f, 1.f);
			const FLinearColor DarkGrass(0.04f, 0.10f, 0.02f);
			const FLinearColor MidGrass(0.30f, 0.52f, 0.12f);
			const FLinearColor DryGrass(0.36f, 0.38f, 0.12f);
			const float Speckle = FMath::SmoothStep(0.30f, 0.70f, GrassTone);
			FLinearColor Grass = FLinearColor::LerpUsingHSV(DarkGrass, MidGrass, Speckle);
			Grass = FLinearColor::LerpUsingHSV(Grass, DryGrass, HeightT * 0.30f);
			Colors.Add(Grass);
		}
	}

	auto SampleSurfaceAt = [&](int32 X, int32 Y) -> float
	{
		X = FMath::Clamp(X, 0, InQuadsPerSide);
		Y = FMath::Clamp(Y, 0, InQuadsPerSide);
		return Heights[Y * VertsPerSide + X] + InCollisionHeightBias;
	};

	for (int32 Y = 0; Y < VertsPerSide; ++Y)
	{
		for (int32 X = 0; X < VertsPerSide; ++X)
		{
			const float HLft = SampleSurfaceAt(X - 1, Y);
			const float HRgt = SampleSurfaceAt(X + 1, Y);
			const float HDn = SampleSurfaceAt(X, Y - 1);
			const float HUp = SampleSurfaceAt(X, Y + 1);

			const FVector Normal = FVector(HLft - HRgt, HDn - HUp, Step * 2.f).GetSafeNormal();
			Normals.Add(Normal);

			FVector Tangent = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
			if (Tangent.IsNearlyZero())
			{
				Tangent = FVector::RightVector;
			}
			Tangents.Add(Tangent);
		}
	}

	for (int32 Y = 0; Y < InQuadsPerSide; ++Y)
	{
		for (int32 X = 0; X < InQuadsPerSide; ++X)
		{
			const int32 I00 = Y * VertsPerSide + X;
			const int32 I10 = I00 + 1;
			const int32 I01 = I00 + VertsPerSide;
			const int32 I11 = I01 + 1;

			Triangles.Add(I00);
			Triangles.Add(I11);
			Triangles.Add(I10);
			Triangles.Add(I00);
			Triangles.Add(I01);
			Triangles.Add(I11);
		}
	}

	TArray<int32> GrassMatIndices;
	GrassMatIndices.Init(0, Triangles.Num() / 3);
	TArray<UMaterialInterface*> GrassMaterials;
	GrassMaterials.Add(Material);

	RuntimeStaticMesh = SolidTerrainChunkPrivate::BuildRuntimeMesh(
		this, Positions, Normals, Tangents, UVs, Colors, Triangles, GrassMaterials, GrassMatIndices,
		/*bBuildCollision=*/true);

	MeshComponent->SetStaticMesh(nullptr);
	MeshComponent->SetStaticMesh(RuntimeStaticMesh);
	if (Material)
	{
		MeshComponent->SetMaterial(0, Material);
	}
	SolidTerrainChunkPrivate::ConfigureCollision(MeshComponent);
	MeshComponent->BodyInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	MeshComponent->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->bUseAsOccluder = false;
	MeshComponent->UpdateBounds();
	MeshComponent->MarkRenderStateDirty();
	MeshComponent->RecreatePhysicsState();

	RebuildExplorationFog(FogHalfMaterial, FogFullMaterial, TerrainMap, CachedFogQuadsPerSide);

	const int32 RenderTris = RuntimeStaticMesh ? RuntimeStaticMesh->GetNumTriangles(0) : 0;
	UE_LOG(LogTemp, Warning,
		TEXT("[SolidCore1] StaticMesh chunk (%d,%d) renderTris=%d actor=(%.0f,%.0f) Z=[%.0f,%.0f] material=%s"),
		InChunkCoord.X, InChunkCoord.Y, RenderTris, OriginX, OriginY, MinZ, MaxZ,
		Material ? *Material->GetName() : TEXT("<null>"));
}

void ASolidTerrainChunk::RebuildExplorationFog(
	UMaterialInterface* FogHalfMaterial,
	UMaterialInterface* FogFullMaterial,
	const USolidTerrainMap* TerrainMap,
	int32 InFogQuadsPerSide)
{
	CachedFogQuadsPerSide = FMath::Clamp(
		InFogQuadsPerSide > 0 ? InFogQuadsPerSide : CachedFogQuadsPerSide, 1, 64);
	const float ChunkSize = FMath::Max(CachedChunkWorldSize, 100.f);
	const int32 FogQuads = CachedFogQuadsPerSide;
	const int32 FogVerts = FogQuads + 1;
	const float Step = ChunkSize / static_cast<float>(FogQuads);
	const float OriginX = static_cast<float>(ChunkCoord.X) * ChunkSize;
	const float OriginY = static_cast<float>(ChunkCoord.Y) * ChunkSize;
	constexpr float FogHeightOffsetCm = 12.f;

	TArray<FVector> FogPositions;
	TArray<FVector> FogNormals;
	TArray<FVector> FogTangents;
	TArray<FVector2D> FogUVs;
	TArray<FLinearColor> FogColors;
	TArray<float> FogAmounts;
	FogPositions.Reserve(FogVerts * FogVerts);
	FogNormals.Reserve(FogVerts * FogVerts);
	FogTangents.Reserve(FogVerts * FogVerts);
	FogUVs.Reserve(FogVerts * FogVerts);
	FogColors.Reserve(FogVerts * FogVerts);
	FogAmounts.Reserve(FogVerts * FogVerts);

	for (int32 Y = 0; Y < FogVerts; ++Y)
	{
		for (int32 X = 0; X < FogVerts; ++X)
		{
			const float LocalX = static_cast<float>(X) * Step;
			const float LocalY = static_cast<float>(Y) * Step;
			const float WorldX = OriginX + LocalX;
			const float WorldY = OriginY + LocalY;
			const float Height = (TerrainMap && TerrainMap->IsBuilt())
				? TerrainMap->SampleHeight(WorldX, WorldY)
				: 0.f;
			const float SurfaceZ = Height + CachedCollisionHeightBias + FogHeightOffsetCm;
			FogPositions.Add(FVector(LocalX, LocalY, SurfaceZ));
			FogNormals.Add(FVector::UpVector);
			FogTangents.Add(FVector::RightVector);
			FogUVs.Add(FVector2D(WorldX * 0.01f, WorldY * 0.01f));
			FogColors.Add(FLinearColor::White);

			const float Fog = (TerrainMap && TerrainMap->IsBuilt())
				? TerrainMap->SamplePoint(WorldX, WorldY).Fog
				: 1.f;
			FogAmounts.Add(FMath::Clamp(Fog, 0.f, 1.f));
		}
	}

	TArray<int32> FogTriangles;
	TArray<int32> FogTriMaterials;
	FogTriangles.Reserve(FogQuads * FogQuads * 6);
	FogTriMaterials.Reserve(FogQuads * FogQuads * 2);

	const bool bHaveHalf = FogHalfMaterial != nullptr;
	const bool bHaveFull = FogFullMaterial != nullptr;
	if (bHaveHalf || bHaveFull)
	{
		for (int32 Y = 0; Y < FogQuads; ++Y)
		{
			for (int32 X = 0; X < FogQuads; ++X)
			{
				const int32 I00 = Y * FogVerts + X;
				const int32 I10 = I00 + 1;
				const int32 I01 = I00 + FogVerts;
				const int32 I11 = I01 + 1;

				const float AvgFog = 0.25f * (
					FogAmounts[I00] + FogAmounts[I10] + FogAmounts[I01] + FogAmounts[I11]);
				const float MaxFog = FMath::Max(
					FMath::Max(FogAmounts[I00], FogAmounts[I10]),
					FMath::Max(FogAmounts[I01], FogAmounts[I11]));

				if (MaxFog <= 0.05f)
				{
					continue;
				}

				// Distinct bands: <0.75 average uses half material (fog~=0.5 zone).
				const bool bFull = AvgFog >= 0.75f;
				int32 Slot = 0;
				if (bHaveHalf && bHaveFull)
				{
					Slot = bFull ? 1 : 0;
				}
				else if (bHaveFull)
				{
					Slot = 0;
				}
				else
				{
					Slot = 0;
				}

				FogTriangles.Add(I00);
				FogTriangles.Add(I11);
				FogTriangles.Add(I10);
				FogTriangles.Add(I00);
				FogTriangles.Add(I01);
				FogTriangles.Add(I11);
				FogTriMaterials.Add(Slot);
				FogTriMaterials.Add(Slot);
			}
		}
	}

	TArray<UMaterialInterface*> FogMaterials;
	if (bHaveHalf && bHaveFull)
	{
		FogMaterials.Add(FogHalfMaterial);
		FogMaterials.Add(FogFullMaterial);
	}
	else if (bHaveFull)
	{
		FogMaterials.Add(FogFullMaterial);
	}
	else if (bHaveHalf)
	{
		FogMaterials.Add(FogHalfMaterial);
	}

	RuntimeFogStaticMesh = nullptr;
	FogMeshComponent->SetStaticMesh(nullptr);
	if (FogTriangles.Num() > 0 && FogMaterials.Num() > 0)
	{
		RuntimeFogStaticMesh = SolidTerrainChunkPrivate::BuildRuntimeMesh(
			this, FogPositions, FogNormals, FogTangents, FogUVs, FogColors, FogTriangles,
			FogMaterials, FogTriMaterials, /*bBuildCollision=*/false);

		FogMeshComponent->SetStaticMesh(RuntimeFogStaticMesh);
		for (int32 MatIndex = 0; MatIndex < FogMaterials.Num(); ++MatIndex)
		{
			FogMeshComponent->SetMaterial(MatIndex, FogMaterials[MatIndex]);
		}
		SolidTerrainChunkPrivate::ConfigureFogOverlay(FogMeshComponent);
		FogMeshComponent->SetVisibility(true);
		FogMeshComponent->SetHiddenInGame(false);
		FogMeshComponent->UpdateBounds();
		FogMeshComponent->MarkRenderStateDirty();
	}
	else
	{
		FogMeshComponent->SetVisibility(false);
	}
}
