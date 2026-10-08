#include "SolidCore1TerrainChunk.h"
#include "SolidCore1TerrainNoise.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

ASolidCore1TerrainChunk::ASolidCore1TerrainChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	SetRootComponent(ProceduralMesh);

	ProceduralMesh->bUseAsyncCooking = false;
	ProceduralMesh->bUseComplexAsSimpleCollision = true;
	ProceduralMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	ProceduralMesh->SetCollisionObjectType(ECC_WorldStatic);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	// Block Camera so the spring-arm probe stays above the surface (Ignore lets it clip under and backfaces vanish).
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	ProceduralMesh->SetGenerateOverlapEvents(false);
	ProceduralMesh->SetCastShadow(true);
	ProceduralMesh->SetVisibility(true);
	ProceduralMesh->SetHiddenInGame(false);
	ProceduralMesh->SetMobility(EComponentMobility::Movable);
	ProceduralMesh->bUseAsOccluder = false;
	ProceduralMesh->bTreatAsBackgroundForOcclusion = true;
	ProceduralMesh->SetCullDistance(0.f);
	ProceduralMesh->bAllowCullDistanceVolume = false;
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

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	TArray<int32> Triangles;

	Vertices.Reserve(VertsPerSide * VertsPerSide);
	Normals.Reserve(VertsPerSide * VertsPerSide);
	UVs.Reserve(VertsPerSide * VertsPerSide);
	Colors.Reserve(VertsPerSide * VertsPerSide);
	Tangents.Reserve(VertsPerSide * VertsPerSide);
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

			const float LocalX = static_cast<float>(X) * Step;
			const float LocalY = static_cast<float>(Y) * Step;
			Vertices.Add(FVector(LocalX, LocalY, SurfaceZ));
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
			Tangents.Add(FProcMeshTangent(Tangent, false));
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

			// Top face (CCW from +Z).
			Triangles.Add(I00);
			Triangles.Add(I10);
			Triangles.Add(I11);
			Triangles.Add(I00);
			Triangles.Add(I11);
			Triangles.Add(I01);
		}
	}

	// Underside on a separate section, biased down so it never z-fights the top.
	// Needed when the camera dips under the surface (one-sided materials go invisible).
	constexpr float UndersideBiasCm = 3.f;
	TArray<FVector> BottomVertices;
	TArray<FVector> BottomNormals;
	TArray<int32> BottomTriangles;
	BottomVertices.Reserve(Vertices.Num());
	BottomNormals.Reserve(Normals.Num());
	BottomTriangles.Reserve(Triangles.Num());
	for (const FVector& V : Vertices)
	{
		BottomVertices.Add(FVector(V.X, V.Y, V.Z - UndersideBiasCm));
	}
	for (const FVector& N : Normals)
	{
		BottomNormals.Add(-N);
	}
	for (int32 Y = 0; Y < InQuadsPerSide; ++Y)
	{
		for (int32 X = 0; X < InQuadsPerSide; ++X)
		{
			const int32 I00 = Y * VertsPerSide + X;
			const int32 I10 = I00 + 1;
			const int32 I01 = I00 + VertsPerSide;
			const int32 I11 = I01 + 1;

			BottomTriangles.Add(I00);
			BottomTriangles.Add(I11);
			BottomTriangles.Add(I10);
			BottomTriangles.Add(I00);
			BottomTriangles.Add(I01);
			BottomTriangles.Add(I11);
		}
	}

	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->bUseComplexAsSimpleCollision = true;
	ProceduralMesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UVs, Colors, Tangents, /*bCreateCollision=*/true);
	ProceduralMesh->CreateMeshSection_LinearColor(
		1, BottomVertices, BottomTriangles, BottomNormals, UVs, Colors, Tangents, /*bCreateCollision=*/false);

	if (Material)
	{
		ProceduralMesh->SetMaterial(0, Material);
		ProceduralMesh->SetMaterial(1, Material);
	}

	// Pad local section boxes, then UpdateBounds() → correct world frustum bounds.
	const FBox PaddedLocalBox(
		FVector(-100.f, -100.f, MinZ - UndersideBiasCm - 500.f),
		FVector(InChunkWorldSize + 100.f, InChunkWorldSize + 100.f, MaxZ + 500.f));
	for (int32 SectionIndex = 0; SectionIndex <= 1; ++SectionIndex)
	{
		if (FProcMeshSection* Section = ProceduralMesh->GetProcMeshSection(SectionIndex))
		{
			Section->SectionLocalBox = PaddedLocalBox;
		}
	}
	ProceduralMesh->SetBoundsScale(1.25f);
	ProceduralMesh->UpdateBounds();
	ProceduralMesh->MarkRenderStateDirty();
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	ProceduralMesh->SetVisibility(true);
	ProceduralMesh->SetHiddenInGame(false);
	ProceduralMesh->bUseAsOccluder = false;
	ProceduralMesh->bTreatAsBackgroundForOcclusion = true;
	ProceduralMesh->RecreatePhysicsState();

	UE_LOG(LogTemp, Warning,
		TEXT("[SolidCore1] Chunk (%d,%d) actor=(%.0f,%.0f) verts=%d tris=%d Z=[%.0f,%.0f] worldBounds=%s material=%s"),
		InChunkCoord.X, InChunkCoord.Y, OriginX, OriginY, Vertices.Num(),
		(Triangles.Num() + BottomTriangles.Num()) / 3, MinZ, MaxZ,
		*ProceduralMesh->Bounds.ToString(),
		Material ? *Material->GetName() : TEXT("<null>"));
}
