#include "SolidCore1TerrainStreamer.h"
#include "SolidCore1TerrainChunk.h"
#include "SolidCore1TerrainNoise.h"
#include "SolidCore1.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

ASolidCore1TerrainStreamer::ASolidCore1TerrainStreamer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Prefer FlatCol (reliable Base Color tint). PrototypeGrid is used when we can replace its Texture.
	static ConstructorHelpers::FObjectFinder<UMaterial> FlatColMat(
		TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (FlatColMat.Succeeded())
	{
		TerrainMaterial = FlatColMat.Object;
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
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Spawned SolidCore1TerrainStreamer (EnsureExists)."));
		UE_LOG(LogSolidCore1, Warning, TEXT("Spawned SolidCore1TerrainStreamer (EnsureExists)."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SolidCore1] Failed to spawn SolidCore1TerrainStreamer (EnsureExists)."));
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
	if (ResolvedTerrainMaterial)
	{
		return ResolvedTerrainMaterial;
	}

	ASolidCore1TerrainStreamer* MutableThis = const_cast<ASolidCore1TerrainStreamer*>(this);

	UMaterialInterface* Parent = TerrainMaterial;
	if (!Parent)
	{
		Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/LevelPrototyping/Materials/M_PrototypeGrid.M_PrototypeGrid"));
	}
	if (!Parent)
	{
		Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	}
	if (!Parent)
	{
		Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	}

	if (Parent)
	{
		if (UMaterialInstanceDynamic* GrassMID = UMaterialInstanceDynamic::Create(Parent, MutableThis))
		{
			// Dual-tone grass: lighter field + darker speckles/grid. Also set BaseColor for FlatCol fallback.
			GrassMID->SetVectorParameterValue(TEXT("BackgroundColor"), GrassColor);
			GrassMID->SetVectorParameterValue(TEXT("Background Color"), GrassColor);
			GrassMID->SetVectorParameterValue(TEXT("GridColor"), GrassDarkColor);
			GrassMID->SetVectorParameterValue(TEXT("SubGridColor"), GrassDarkColor * 0.75f);
			GrassMID->SetVectorParameterValue(TEXT("TopGridColor"), GrassColor * 0.85f);
			GrassMID->SetVectorParameterValue(TEXT("TopSubGridGridColor"), GrassDarkColor);
			GrassMID->SetVectorParameterValue(TEXT("BaseColor"), GrassColor);
			GrassMID->SetVectorParameterValue(TEXT("Base Color"), GrassColor);
			GrassMID->SetVectorParameterValue(TEXT("Color"), GrassColor);
			GrassMID->SetScalarParameterValue(TEXT("Grid Size"), GrassGridSize);
			GrassMID->SetScalarParameterValue(TEXT("GridSize"), GrassGridSize);
			GrassMID->SetScalarParameterValue(TEXT("Roughness"), 0.88f);
			MutableThis->ResolvedTerrainMaterial = GrassMID;
			UE_LOG(LogSolidCore1, Warning,
				TEXT("Terrain material: grassy noisy MID from %s"), *Parent->GetName());
			return ResolvedTerrainMaterial;
		}
	}

	MutableThis->ResolvedTerrainMaterial = Parent;
	return ResolvedTerrainMaterial;
}

float ASolidCore1TerrainStreamer::SampleHeightAtWorld(const FVector& WorldLocation) const
{
	return SolidCore1TerrainNoise::SampleHeight(
		WorldLocation.X, WorldLocation.Y, Seed, FrequencyScale, Amplitude, BaseHeight);
}

void ASolidCore1TerrainStreamer::DisableLandscapeActorsOnce()
{
	if (!bDisableLandscapeActors || bDidDisableLandscape)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// SC1-0015 Destroy+LoadClass did not clear horizon slivers — revert to one-shot hide.
	UClass* LandscapeClass = FindObject<UClass>(nullptr, TEXT("/Script/Landscape.Landscape"));
	UClass* LandscapeProxyClass = FindObject<UClass>(nullptr, TEXT("/Script/Landscape.LandscapeProxy"));
	UClass* StreamingProxyClass = FindObject<UClass>(nullptr, TEXT("/Script/Landscape.LandscapeStreamingProxy"));

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UClass* ActorClass = It->GetClass();
		const bool bIsLandscape =
			(LandscapeClass && ActorClass->IsChildOf(LandscapeClass)) ||
			(LandscapeProxyClass && ActorClass->IsChildOf(LandscapeProxyClass)) ||
			(StreamingProxyClass && ActorClass->IsChildOf(StreamingProxyClass));

		if (!bIsLandscape)
		{
			continue;
		}

		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
		++Count;
	}

	bDidDisableLandscape = true;
	if (Count > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Disabled %d Landscape actor(s) so pawn uses procedural terrain."), Count);
		UE_LOG(LogSolidCore1, Warning, TEXT("Disabled %d Landscape actor(s) for procedural terrain."), Count);
	}
}

void ASolidCore1TerrainStreamer::TrySnapFocusToTerrain(AActor* Focus)
{
	if (!bSnapFocusToTerrain || !Focus)
	{
		return;
	}

	const FVector Loc = Focus->GetActorLocation();
	const float LandZ = SampleHeightAtWorld(Loc) + CollisionHeightBias;

	float CapsuleHalfHeight = 96.f;
	if (const ACharacter* Character = Cast<ACharacter>(Focus))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const float TargetZ = LandZ + CapsuleHalfHeight + SnapHeightPadding;
	// Only correct when clearly below (or far above) the surface — avoids fighting normal walking.
	if (Loc.Z > TargetZ - 50.f && Loc.Z < TargetZ + 2000.f)
	{
		return;
	}

	Focus->SetActorLocation(FVector(Loc.X, Loc.Y, TargetZ));
	if (ACharacter* Character = Cast<ACharacter>(Focus))
	{
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			Move->SetMovementMode(MOVE_Walking);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Snapped focus onto terrain Z=%.1f at (%.0f, %.0f)"), LandZ, Loc.X, Loc.Y);
}

void ASolidCore1TerrainStreamer::UpdateStreaming()
{
	DisableLandscapeActorsOnce();

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

		const FVector SpawnLoc(
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			0.f);

		ASolidCore1TerrainChunk* Chunk = World->SpawnActor<ASolidCore1TerrainChunk>(
			ASolidCore1TerrainChunk::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
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
			CollisionHeightBias,
			Material);

		LoadedChunks.Add(Coord, Chunk);
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Built terrain chunk (%d, %d) origin=(%.0f, %.0f) loaded=%d"),
			Coord.X, Coord.Y,
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			LoadedChunks.Num());
		UE_LOG(LogSolidCore1, Warning, TEXT("Built terrain chunk (%d, %d) at origin (%.0f, %.0f). Loaded=%d"),
			Coord.X, Coord.Y,
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			LoadedChunks.Num());
	}

	TrySnapFocusToTerrain(Focus);
}
