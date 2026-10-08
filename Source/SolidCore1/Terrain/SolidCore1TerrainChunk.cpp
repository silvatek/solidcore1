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

	// Sync cooking so collision exists before the pawn lands (async often causes fall-through).
	ProceduralMesh->bUseAsyncCooking = false;
	ProceduralMesh->bUseComplexAsSimpleCollision = true;
	ProceduralMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	ProceduralMesh->SetCollisionObjectType(ECC_WorldStatic);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ProceduralMesh->SetGenerateOverlapEvents(false);
	ProceduralMesh->SetCastShadow(true);
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
	TArray<FVector> CollisionVertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	TArray<int32> Triangles;

	Vertices.Reserve(VertsPerSide * VertsPerSide);
	CollisionVertices.Reserve(VertsPerSide * VertsPerSide);
	Normals.Reserve(VertsPerSide * VertsPerSide);
	UVs.Reserve(VertsPerSide * VertsPerSide);
	Colors.Reserve(VertsPerSide * VertsPerSide);
	Tangents.Reserve(VertsPerSide * VertsPerSide);
	Triangles.Reserve(InQuadsPerSide * InQuadsPerSide * 6);

	TArray<float> Heights;
	Heights.SetNumUninitialized(VertsPerSide * VertsPerSide);

	for (int32 Y = 0; Y < VertsPerSide; ++Y)
	{
		for (int32 X = 0; X < VertsPerSide; ++X)
		{
			const float WorldX = OriginX + static_cast<float>(X) * Step;
			const float WorldY = OriginY + static_cast<float>(Y) * Step;
			const float Height = SolidCore1TerrainNoise::SampleHeight(
				WorldX, WorldY, InSeed, InFrequencyScale, InAmplitude, InBaseHeight);
			Heights[Y * VertsPerSide + X] = Height;

			const float LocalX = static_cast<float>(X) * Step;
			const float LocalY = static_cast<float>(Y) * Step;
			Vertices.Add(FVector(LocalX, LocalY, Height));
			CollisionVertices.Add(FVector(LocalX, LocalY, Height + InCollisionHeightBias));
			UVs.Add(FVector2D(static_cast<float>(X) / InQuadsPerSide, static_cast<float>(Y) / InQuadsPerSide));

			const float T = FMath::Clamp((Height - InBaseHeight) / FMath::Max(InAmplitude, 1.f), 0.f, 1.f);
			Colors.Add(FLinearColor::LerpUsingHSV(FLinearColor(0.15f, 0.35f, 0.12f), FLinearColor(0.45f, 0.42f, 0.32f), T));
		}
	}

	auto SampleHeightAt = [&](int32 X, int32 Y) -> float
	{
		X = FMath::Clamp(X, 0, InQuadsPerSide);
		Y = FMath::Clamp(Y, 0, InQuadsPerSide);
		return Heights[Y * VertsPerSide + X];
	};

	for (int32 Y = 0; Y < VertsPerSide; ++Y)
	{
		for (int32 X = 0; X < VertsPerSide; ++X)
		{
			const float HLft = SampleHeightAt(X - 1, Y);
			const float HRgt = SampleHeightAt(X + 1, Y);
			const float HDn = SampleHeightAt(X, Y - 1);
			const float HUp = SampleHeightAt(X, Y + 1);

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

			Triangles.Add(I00);
			Triangles.Add(I10);
			Triangles.Add(I11);

			Triangles.Add(I00);
			Triangles.Add(I11);
			Triangles.Add(I01);
		}
	}

	ProceduralMesh->ClearAllMeshSections();
	ProceduralMesh->bUseComplexAsSimpleCollision = true;

	// Section 0: visible terrain (no collision).
	ProceduralMesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UVs, Colors, Tangents, /*bCreateCollision=*/false);
	if (Material)
	{
		ProceduralMesh->SetMaterial(0, Material);
	}

	// Section 1: invisible collision surface raised to counter capsule sink into complex mesh.
	ProceduralMesh->CreateMeshSection_LinearColor(
		1, CollisionVertices, Triangles, Normals, UVs, Colors, Tangents, /*bCreateCollision=*/true);
	ProceduralMesh->SetMeshSectionVisible(1, false);

	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ProceduralMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ProceduralMesh->RecreatePhysicsState();

	// Static after positioning: avoids Movable "movement base" paths (GetMovementBase deprecation spam).
	ProceduralMesh->SetMobility(EComponentMobility::Static);
}
