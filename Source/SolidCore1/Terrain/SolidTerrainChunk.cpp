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
		// Fog must never block pawn/camera traces (spring arm ProbeChannel = ECC_Camera).
		Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Mesh->SetCollisionObjectType(ECC_WorldDynamic);
		Mesh->BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->BodyInstance.SetResponseToAllChannels(ECR_Ignore);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetNotifyRigidBodyCollision(false);
		Mesh->CanCharacterStepUpOn = ECB_No;
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
	int32 InFogQuadsPerSide,
	float InFogVolumeHeightCm,
	float InFogVolumeHeightHalfCm)
{
	ChunkCoord = InChunkCoord;
	InQuadsPerSide = FMath::Clamp(InQuadsPerSide, 1, 256);
	InChunkWorldSize = FMath::Max(InChunkWorldSize, 100.f);
	InCollisionHeightBias = FMath::Max(InCollisionHeightBias, 0.f);
	CachedChunkWorldSize = InChunkWorldSize;
	CachedCollisionHeightBias = InCollisionHeightBias;
	CachedFogQuadsPerSide = FMath::Clamp(InFogQuadsPerSide, 1, 64);
	CachedFogVolumeHeightCm = FMath::Max(InFogVolumeHeightCm, 200.f);
	CachedFogVolumeHeightHalfCm = FMath::Max(InFogVolumeHeightHalfCm, 200.f);

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

	RebuildExplorationFog(
		FogHalfMaterial,
		FogFullMaterial,
		TerrainMap,
		CachedFogQuadsPerSide,
		CachedFogVolumeHeightCm,
		CachedFogVolumeHeightHalfCm);

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
	int32 InFogQuadsPerSide,
	float InFogVolumeHeightCm,
	float InFogVolumeHeightHalfCm)
{
	CachedFogQuadsPerSide = FMath::Clamp(
		InFogQuadsPerSide > 0 ? InFogQuadsPerSide : CachedFogQuadsPerSide, 1, 64);
	CachedFogVolumeHeightCm = FMath::Max(
		InFogVolumeHeightCm > 0.f ? InFogVolumeHeightCm : CachedFogVolumeHeightCm, 200.f);
	CachedFogVolumeHeightHalfCm = FMath::Max(
		InFogVolumeHeightHalfCm > 0.f ? InFogVolumeHeightHalfCm : CachedFogVolumeHeightHalfCm, 200.f);

	const float ChunkSize = FMath::Max(CachedChunkWorldSize, 100.f);
	const int32 FogQuads = CachedFogQuadsPerSide;
	const int32 FogVerts = FogQuads + 1;
	const float Step = ChunkSize / static_cast<float>(FogQuads);
	const float OriginX = static_cast<float>(ChunkCoord.X) * ChunkSize;
	const float OriginY = static_cast<float>(ChunkCoord.Y) * ChunkSize;

	// Sample surface corners for each low-res fog cell.
	TArray<float> SurfaceZ;
	TArray<float> FogAmounts;
	SurfaceZ.SetNumUninitialized(FogVerts * FogVerts);
	FogAmounts.SetNumUninitialized(FogVerts * FogVerts);
	for (int32 Y = 0; Y < FogVerts; ++Y)
	{
		for (int32 X = 0; X < FogVerts; ++X)
		{
			const float WorldX = OriginX + static_cast<float>(X) * Step;
			const float WorldY = OriginY + static_cast<float>(Y) * Step;
			const float Height = (TerrainMap && TerrainMap->IsBuilt())
				? TerrainMap->SampleHeight(WorldX, WorldY)
				: 0.f;
			SurfaceZ[Y * FogVerts + X] = Height + CachedCollisionHeightBias;
			FogAmounts[Y * FogVerts + X] = (TerrainMap && TerrainMap->IsBuilt())
				? FMath::Clamp(TerrainMap->SamplePoint(WorldX, WorldY).Fog, 0.f, 1.f)
				: 1.f;
		}
	}

	TArray<FVector> FogPositions;
	TArray<FVector> FogNormals;
	TArray<FVector> FogTangents;
	TArray<FVector2D> FogUVs;
	TArray<FLinearColor> FogColors;
	TArray<int32> FogTriangles;
	TArray<int32> FogTriMaterials;

	// Each fogged cell becomes a prism (top + 4 walls) so fog reads as air banks, not snow.
	FogPositions.Reserve(FogQuads * FogQuads * 8);
	FogNormals.Reserve(FogQuads * FogQuads * 8);
	FogTangents.Reserve(FogQuads * FogQuads * 8);
	FogUVs.Reserve(FogQuads * FogQuads * 8);
	FogColors.Reserve(FogQuads * FogQuads * 8);
	FogTriangles.Reserve(FogQuads * FogQuads * 30);
	FogTriMaterials.Reserve(FogQuads * FogQuads * 10);

	auto AppendVert = [&](const FVector& Pos, const FVector& Normal) -> int32
	{
		const int32 Index = FogPositions.Num();
		FogPositions.Add(Pos);
		FogNormals.Add(Normal);
		FVector Tangent = FVector::CrossProduct(FVector::UpVector, Normal).GetSafeNormal();
		if (Tangent.IsNearlyZero())
		{
			Tangent = FVector::RightVector;
		}
		FogTangents.Add(Tangent);
		FogUVs.Add(FVector2D(Pos.X * 0.01f, Pos.Y * 0.01f));
		FogColors.Add(FLinearColor::White);
		return Index;
	};

	auto AppendQuad = [&](int32 I0, int32 I1, int32 I2, int32 I3, int32 Slot)
	{
		// Both windings — FlatCol is one-sided; avoids black backs on fog banks.
		FogTriangles.Add(I0);
		FogTriangles.Add(I1);
		FogTriangles.Add(I2);
		FogTriangles.Add(I0);
		FogTriangles.Add(I2);
		FogTriangles.Add(I3);
		FogTriangles.Add(I0);
		FogTriangles.Add(I2);
		FogTriangles.Add(I1);
		FogTriangles.Add(I0);
		FogTriangles.Add(I3);
		FogTriangles.Add(I2);
		FogTriMaterials.Add(Slot);
		FogTriMaterials.Add(Slot);
		FogTriMaterials.Add(Slot);
		FogTriMaterials.Add(Slot);
	};

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

				const float F00 = FogAmounts[I00];
				const float F10 = FogAmounts[I10];
				const float F01 = FogAmounts[I01];
				const float F11 = FogAmounts[I11];
				const float AvgFog = 0.25f * (F00 + F10 + F01 + F11);
				const float MinFog = FMath::Min(FMath::Min(F00, F10), FMath::Min(F01, F11));

				// Require the whole cell to be fogged. Using MaxFog let pillars straddle into
				// the clear disk and sit on top of the camera / spring-arm path.
				if (MinFog <= 0.05f)
				{
					continue;
				}

				const bool bFull = AvgFog >= 0.75f;
				int32 Slot = 0;
				if (bHaveHalf && bHaveFull)
				{
					Slot = bFull ? 1 : 0;
				}

				// Half band: checkerboard pillars so ~50% of cells stay empty (see-through).
				if (!bFull && (((X + Y) & 1) == 0))
				{
					continue;
				}

				const float VolumeHeight = bFull ? CachedFogVolumeHeightCm : CachedFogVolumeHeightHalfCm;
				const float X0 = static_cast<float>(X) * Step;
				const float X1 = static_cast<float>(X + 1) * Step;
				const float Y0 = static_cast<float>(Y) * Step;
				const float Y1 = static_cast<float>(Y + 1) * Step;

				const float Z00 = SurfaceZ[I00];
				const float Z10 = SurfaceZ[I10];
				const float Z01 = SurfaceZ[I01];
				const float Z11 = SurfaceZ[I11];

				constexpr float SkirtCm = 20.f;
				FVector B00(X0, Y0, Z00 + SkirtCm);
				FVector B10(X1, Y0, Z10 + SkirtCm);
				FVector B01(X0, Y1, Z01 + SkirtCm);
				FVector B11(X1, Y1, Z11 + SkirtCm);

				if (!bFull)
				{
					// Shrink pillar to the cell center so gaps between half-fog cells stay open.
					const float Inset = Step * 0.22f;
					B00 = FVector(X0 + Inset, Y0 + Inset, Z00 + SkirtCm);
					B10 = FVector(X1 - Inset, Y0 + Inset, Z10 + SkirtCm);
					B01 = FVector(X0 + Inset, Y1 - Inset, Z01 + SkirtCm);
					B11 = FVector(X1 - Inset, Y1 - Inset, Z11 + SkirtCm);
				}

				const FVector T00(B00.X, B00.Y, Z00 + VolumeHeight);
				const FVector T10(B10.X, B10.Y, Z10 + VolumeHeight);
				const FVector T01(B01.X, B01.Y, Z01 + VolumeHeight);
				const FVector T11(B11.X, B11.Y, Z11 + VolumeHeight);

				auto AppendWall = [&](const FVector& BottomA, const FVector& BottomB,
					const FVector& TopB, const FVector& TopA, const FVector& Normal)
				{
					const int32 V0 = AppendVert(BottomA, Normal);
					const int32 V1 = AppendVert(BottomB, Normal);
					const int32 V2 = AppendVert(TopB, Normal);
					const int32 V3 = AppendVert(TopA, Normal);
					AppendQuad(V0, V1, V2, V3, Slot);
				};

				// Closed prism (top + 4 walls). Half uses checkerboard+inset; full fills every cell.
				{
					const int32 V0 = AppendVert(T00, FVector::UpVector);
					const int32 V1 = AppendVert(T10, FVector::UpVector);
					const int32 V2 = AppendVert(T11, FVector::UpVector);
					const int32 V3 = AppendVert(T01, FVector::UpVector);
					AppendQuad(V0, V1, V2, V3, Slot);
				}
				AppendWall(B00, B10, T10, T00, FVector(0.f, -1.f, 0.f));
				AppendWall(B11, B01, T01, T11, FVector(0.f, 1.f, 0.f));
				AppendWall(B01, B00, T00, T01, FVector(-1.f, 0.f, 0.f));
				AppendWall(B10, B11, T11, T10, FVector(1.f, 0.f, 0.f));
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
		// SetStaticMesh can restore default collision — force fog non-blocking again.
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
