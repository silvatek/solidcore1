#include "SolidCore1TerrainChunk.h"
#include "SolidCore1TerrainNoise.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "Components/StaticMeshComponent.h"

namespace SolidCore1TerrainChunkPrivate
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
}

ASolidCore1TerrainChunk::ASolidCore1TerrainChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	SolidCore1TerrainChunkPrivate::ConfigureCollision(MeshComponent);
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

void ASolidCore1TerrainChunk::BuildChunk(
	FIntPoint InChunkCoord,
	float InChunkWorldSize,
	int32 InQuadsPerSide,
	int32 InSeed,
	float InFrequencyScale,
	float InAmplitude,
	float InBaseHeight,
	float InCollisionHeightBias,
	UMaterialInterface* Material)
{
	ChunkCoord = InChunkCoord;
	InQuadsPerSide = FMath::Clamp(InQuadsPerSide, 1, 256);
	InChunkWorldSize = FMath::Max(InChunkWorldSize, 100.f);
	InCollisionHeightBias = FMath::Max(InCollisionHeightBias, 0.f);

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
			const float Height = SolidCore1TerrainNoise::SampleHeight(
				WorldX, WorldY, InSeed, InFrequencyScale, InAmplitude, InBaseHeight);
			Heights[Y * VertsPerSide + X] = Height;

			const float SurfaceZ = Height + InCollisionHeightBias;
			MinZ = FMath::Min(MinZ, SurfaceZ);
			MaxZ = FMath::Max(MaxZ, SurfaceZ);

			Positions.Add(FVector(static_cast<float>(X) * Step, static_cast<float>(Y) * Step, SurfaceZ));
			UVs.Add(FVector2D(static_cast<float>(X) / InQuadsPerSide, static_cast<float>(Y) / InQuadsPerSide));

			const float T = FMath::Clamp((Height - InBaseHeight) / FMath::Max(InAmplitude, 1.f), 0.f, 1.f);
			Colors.Add(FLinearColor::LerpUsingHSV(FLinearColor(0.2f, 0.55f, 0.15f), FLinearColor(0.55f, 0.5f, 0.35f), T));
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

			// SC1-0010: flip winding vs prior builds. Collision worked while looking down showed
			// only blue fog + horizon ribbons — classic one-sided backface cull from reversed winding.
			Triangles.Add(I00);
			Triangles.Add(I11);
			Triangles.Add(I10);
			Triangles.Add(I00);
			Triangles.Add(I01);
			Triangles.Add(I11);
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

	const FPolygonGroupID PolygonGroupID = MeshDescription.CreatePolygonGroup();
	PolygonGroupNames[PolygonGroupID] = FName(TEXT("Terrain"));

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

		TArray<FVertexInstanceID, TInlineAllocator<3>> InstanceIDs;
		const int32 CornerIndices[3] = { I0, I1, I2 };
		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			const int32 VertIndex = CornerIndices[Corner];
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

	RuntimeStaticMesh = NewObject<UStaticMesh>(this, NAME_None, RF_Transient);
	RuntimeStaticMesh->bAllowCPUAccess = true;
	RuntimeStaticMesh->NeverStream = true;
	// Open World projects default Nanite on; runtime meshes with Nanite enabled but no valid
	// Nanite build often collide yet draw as invisible (blue fog). Force classic raster path.
	RuntimeStaticMesh->NaniteSettings.bEnabled = false;

	FStaticMaterial StaticMaterial(Material, FName(TEXT("Terrain")), FName(TEXT("Terrain")));
	RuntimeStaticMesh->SetStaticMaterials({ StaticMaterial });

	// Full build + CPU access for complex-as-simple (bFastBuild skipped collision in SC1-0007).
	UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
	BuildParams.bBuildSimpleCollision = false;
	BuildParams.bFastBuild = false;
	BuildParams.bAllowCpuAccess = true;
	BuildParams.bCommitMeshDescription = true;
	BuildParams.bMarkPackageDirty = false;

	const TArray<const FMeshDescription*> Descriptions = { &MeshDescription };
	const bool bBuilt = RuntimeStaticMesh->BuildFromMeshDescriptions(Descriptions, BuildParams);
	if (!bBuilt)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[SolidCore1] BuildFromMeshDescriptions FAILED for chunk (%d,%d)"),
			InChunkCoord.X, InChunkCoord.Y);
	}

	// Build can re-enable Nanite from project defaults — keep it off for runtime meshes.
	RuntimeStaticMesh->NaniteSettings.bEnabled = false;

	if (!RuntimeStaticMesh->GetBodySetup())
	{
		RuntimeStaticMesh->CreateBodySetup();
	}
	if (UBodySetup* BodySetup = RuntimeStaticMesh->GetBodySetup())
	{
		BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		BodySetup->bDoubleSidedGeometry = true;
		BodySetup->InvalidatePhysicsData();
		BodySetup->CreatePhysicsMeshes();
	}

	MeshComponent->SetStaticMesh(nullptr);
	MeshComponent->SetStaticMesh(RuntimeStaticMesh);
	if (Material)
	{
		MeshComponent->SetMaterial(0, Material);
	}

	SolidCore1TerrainChunkPrivate::ConfigureCollision(MeshComponent);
	MeshComponent->BodyInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	MeshComponent->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
	MeshComponent->bUseAsOccluder = false;
	MeshComponent->bTreatAsBackgroundForOcclusion = true;
	MeshComponent->UpdateBounds();
	MeshComponent->MarkRenderStateDirty();
	MeshComponent->RecreatePhysicsState();

	const int32 RenderTris = RuntimeStaticMesh->GetNumTriangles(0);
	int32 SimpleCollisionElems = 0;
	ECollisionTraceFlag TraceFlag = CTF_UseDefault;
	if (const UBodySetup* BodySetup = RuntimeStaticMesh->GetBodySetup())
	{
		SimpleCollisionElems = BodySetup->AggGeom.GetElementCount();
		TraceFlag = BodySetup->CollisionTraceFlag;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[SolidCore1] StaticMesh chunk (%d,%d) built=%d renderTris=%d simpleCols=%d traceFlag=%d actor=(%.0f,%.0f) Z=[%.0f,%.0f] worldBounds=%s material=%s"),
		InChunkCoord.X, InChunkCoord.Y, bBuilt ? 1 : 0, RenderTris, SimpleCollisionElems,
		static_cast<int32>(TraceFlag), OriginX, OriginY, MinZ, MaxZ,
		*MeshComponent->Bounds.ToString(),
		Material ? *Material->GetName() : TEXT("<null>"));
}
