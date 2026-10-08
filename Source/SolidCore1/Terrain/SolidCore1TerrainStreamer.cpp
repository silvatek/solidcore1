#include "Terrain/SolidCore1TerrainStreamer.h"
#include "Terrain/SolidCore1TerrainChunk.h"
#include "Terrain/SolidCore1TerrainNoise.h"
#include "SolidCore1.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASolidCore1TerrainStreamer::ASolidCore1TerrainStreamer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (DefaultMat.Succeeded())
	{
		TerrainMaterial = DefaultMat.Object;
	}
}

ASolidCore1TerrainStreamer* ASolidCore1TerrainStreamer::EnsureExists(UWorld* World)
{
	if (!World || World->bIsTearingDown)
	{
		return nullptr;
	}

	for (TActorIterator<ASolidCore1TerrainStreamer> It(World); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASolidCore1TerrainStreamer* Streamer = World->SpawnActor<ASolidCore1TerrainStreamer>(
		ASolidCore1TerrainStreamer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (Streamer)
	{
		UE_LOG(LogSolidCore1, Warning, TEXT("Spawned SolidCore1TerrainStreamer (EnsureExists)."));
	}
	else
	{
		UE_LOG(LogSolidCore1, Error, TEXT("Failed to spawn SolidCore1TerrainStreamer (EnsureExists)."));
	}

	return Streamer;
}

void ASolidCore1TerrainStreamer::BeginPlay()
{
	Super::BeginPlay();
	TimeSinceUpdate = UpdateIntervalSeconds;
	UpdateStreaming();
}

void ASolidCore1TerrainStreamer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceUpdate += DeltaSeconds;
	if (TimeSinceUpdate < UpdateIntervalSeconds)
	{
		return;
	}

	TimeSinceUpdate = 0.f;
	UpdateStreaming();
}

FIntPoint ASolidCore1TerrainStreamer::WorldToChunkCoord(const FVector& WorldLocation) const
{
	const float Size = FMath::Max(ChunkWorldSize, 100.f);
	return FIntPoint(
		FMath::FloorToInt(WorldLocation.X / Size),
		FMath::FloorToInt(WorldLocation.Y / Size));
}

AActor* ASolidCore1TerrainStreamer::ResolveFocusActor() const
{
	if (AActor* Focus = FocusActor.Get())
	{
		return Focus;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				return Pawn;
			}
		}
	}

	return nullptr;
}

UMaterialInterface* ASolidCore1TerrainStreamer::ResolveMaterial() const
{
	return TerrainMaterial;
}

float ASolidCore1TerrainStreamer::SampleHeightAtWorld(const FVector& WorldLocation) const
{
	return SolidCore1TerrainNoise::SampleHeight(
		WorldLocation.X, WorldLocation.Y, Seed, FrequencyScale, Amplitude, BaseHeight);
}

void ASolidCore1TerrainStreamer::TrySnapFocusToTerrain(AActor* Focus)
{
	if (!bSnapFocusToTerrainOnce || bDidSnapFocus || !Focus)
	{
		return;
	}

	const FVector Loc = Focus->GetActorLocation();
	const float TerrainZ = SampleHeightAtWorld(Loc);
	Focus->SetActorLocation(FVector(Loc.X, Loc.Y, TerrainZ + 120.f));
	bDidSnapFocus = true;

	UE_LOG(LogSolidCore1, Warning,
		TEXT("Snapped focus to procedural terrain height %.1f at (%.0f, %.0f)."), TerrainZ, Loc.X, Loc.Y);
}

void ASolidCore1TerrainStreamer::UpdateStreaming()
{
	AActor* Focus = ResolveFocusActor();
	if (!Focus)
	{
		return;
	}

	const FIntPoint Center = WorldToChunkCoord(Focus->GetActorLocation());

	TSet<FIntPoint> Desired;
	Desired.Reserve((ViewRadiusChunks * 2 + 1) * (ViewRadiusChunks * 2 + 1));
	for (int32 DY = -ViewRadiusChunks; DY <= ViewRadiusChunks; ++DY)
	{
		for (int32 DX = -ViewRadiusChunks; DX <= ViewRadiusChunks; ++DX)
		{
			Desired.Add(FIntPoint(Center.X + DX, Center.Y + DY));
		}
	}

	TArray<FIntPoint> ToRemove;
	for (const TPair<FIntPoint, TObjectPtr<ASolidCore1TerrainChunk>>& Pair : LoadedChunks)
	{
		if (!Desired.Contains(Pair.Key))
		{
			ToRemove.Add(Pair.Key);
		}
	}

	for (const FIntPoint& Key : ToRemove)
	{
		if (ASolidCore1TerrainChunk* Chunk = LoadedChunks.FindRef(Key))
		{
			Chunk->Destroy();
		}
		LoadedChunks.Remove(Key);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMaterialInterface* Material = ResolveMaterial();

	for (const FIntPoint& Coord : Desired)
	{
		if (LoadedChunks.Contains(Coord))
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ASolidCore1TerrainChunk* Chunk = World->SpawnActor<ASolidCore1TerrainChunk>(
			ASolidCore1TerrainChunk::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (!Chunk)
		{
			UE_LOG(LogSolidCore1, Error, TEXT("Failed to spawn terrain chunk at %d,%d"), Coord.X, Coord.Y);
			continue;
		}

		Chunk->BuildChunk(
			Coord,
			ChunkWorldSize,
			QuadsPerSide,
			Seed,
			FrequencyScale,
			Amplitude,
			BaseHeight,
			Material);

		LoadedChunks.Add(Coord, Chunk);
		UE_LOG(LogSolidCore1, Warning, TEXT("Built terrain chunk (%d, %d) at origin (%.0f, %.0f). Loaded=%d"),
			Coord.X, Coord.Y,
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			LoadedChunks.Num());
	}

	TrySnapFocusToTerrain(Focus);
}
