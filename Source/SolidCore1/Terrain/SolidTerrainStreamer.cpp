#include "SolidTerrainStreamer.h"
#include "SolidTerrainChunk.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainMaterials.h"
#include "SolidTerrainNoise.h"
#include "SolidWorldMap.h"
#include "SolidCore1.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Materials/MaterialInterface.h"

ASolidTerrainStreamer::ASolidTerrainStreamer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// TerrainMaterial left null: ResolveMaterial prefers Fab Mat_025_grass, then FlatCol.
	// Do NOT default to M_PrototypeGrid (hard-wired grey checker).
}

ASolidTerrainStreamer* ASolidTerrainStreamer::FindExisting(UWorld* World)
{
	if (!World || World->bIsTearingDown)
	{
		return nullptr;
	}

	for (TActorIterator<ASolidTerrainStreamer> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

ASolidTerrainStreamer* ASolidTerrainStreamer::EnsureExists(UWorld* World)
{
	if (ASolidTerrainStreamer* Existing = FindExisting(World))
	{
		return Existing;
	}

	if (!World || World->bIsTearingDown)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASolidTerrainStreamer* Streamer = World->SpawnActor<ASolidTerrainStreamer>(
		ASolidTerrainStreamer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (Streamer)
	{
		UE_LOG(LogSolid, Warning, TEXT("Spawned SolidTerrainStreamer (EnsureExists)."));
	}
	else
	{
		UE_LOG(LogSolid, Error, TEXT("Failed to spawn SolidTerrainStreamer (EnsureExists)."));
	}

	return Streamer;
}

void ASolidTerrainStreamer::BeginPlay()
{
	Super::BeginPlay();
	EnsureTerrainMap();
	TryRelocateFocusToStartTown();
	CenterExplorationFogOnFocus();
	if (bRenderExplorationFogMeshes)
	{
		EnsureExplorationFogMaterials();
	}
	EnsureHeightFog();
	bHasFogApplyLocation = false;
	ClearExplorationFogAtFocus();
	TimeSinceUpdate = UpdateIntervalSeconds;
	UpdateStreaming();
	UpdateTerrainFog(0.f);
	if (bRenderExplorationFogMeshes)
	{
		ProcessExplorationFogMeshRebuilds();
	}
}

void ASolidTerrainStreamer::EnsureTerrainMap()
{
	if (TerrainMap && TerrainMap->IsBuilt())
	{
		return;
	}

	if (!TerrainMap)
	{
		TerrainMap = NewObject<USolidTerrainMap>(this, TEXT("TerrainMap"));
	}

	// Align point spacing with chunk vertex step when possible so samples hit grid nodes.
	float Spacing = TerrainPointSpacing;
	if (QuadsPerSide > 0)
	{
		Spacing = ChunkWorldSize / static_cast<float>(QuadsPerSide);
	}

	TerrainMap->Build(
		Seed,
		FrequencyScale,
		Amplitude,
		BaseHeight,
		TerrainMapSize,
		TerrainMapSize,
		Spacing,
		/*bForceRebuild=*/false);
}

FSolidTerrainPoint ASolidTerrainStreamer::GetTerrainPointAt(const FVector& WorldLocation) const
{
	if (TerrainMap && TerrainMap->IsBuilt())
	{
		return TerrainMap->SamplePoint(WorldLocation.X, WorldLocation.Y);
	}

	FSolidTerrainPoint Fallback;
	Fallback.X = WorldLocation.X;
	Fallback.Y = WorldLocation.Y;
	Fallback.Height = SampleHeightAtWorld(WorldLocation);
	return Fallback;
}

void ASolidTerrainStreamer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateTerrainFog(DeltaSeconds);

	TimeSinceUpdate += DeltaSeconds;
	if (TimeSinceUpdate < UpdateIntervalSeconds)
	{
		return;
	}

	TimeSinceUpdate = 0.f;
	TryRelocateFocusToStartTown();
	CenterExplorationFogOnFocus();
	ClearExplorationFogAtFocus();
	UpdateStreaming();
	if (bRenderExplorationFogMeshes)
	{
		ProcessExplorationFogMeshRebuilds();
	}
}

int32 ASolidTerrainStreamer::ClearExplorationFogAtFocus()
{
	if (!TerrainMap || !TerrainMap->IsBuilt())
	{
		return 0;
	}

	AActor* Focus = ResolveFocusActor();
	if (!Focus)
	{
		return 0;
	}

	const FVector Loc = Focus->GetActorLocation();
	if (bHasFogApplyLocation)
	{
		const float MovedSq = FVector::DistSquared2D(Loc, LastFogApplyLocation);
		const float Threshold = FMath::Max(FogUpdateMoveThresholdCm, 50.f);
		if (MovedSq < Threshold * Threshold)
		{
			return 0;
		}
	}

	// Trail clear only. The initial bubble is CenterExplorationFogOnFocus.
	const int32 Changed = TerrainMap->ApplyExplorationFogAround(Loc.X, Loc.Y);
	LastFogApplyLocation = Loc;
	bHasFogApplyLocation = true;

	if (Changed > 0 && bRenderExplorationFogMeshes)
	{
		QueueExplorationFogMeshRefresh(
			Loc.X, Loc.Y,
			SolidTerrainFog::MetersToCm(SolidTerrainFog::FullFogStartMeters) + ChunkWorldSize * 0.5f);
	}
	return Changed;
}

void ASolidTerrainStreamer::EnsureExplorationFogMaterials()
{
	if (!ExplorationFogHalfMaterial)
	{
		ExplorationFogHalfMaterial = SolidTerrainFog::CreateHalfMaterial(this);
	}
	if (!ExplorationFogFullMaterial)
	{
		ExplorationFogFullMaterial = SolidTerrainFog::CreateFullMaterial(this);
	}
	if (!ExplorationFogOuterMaterial)
	{
		ExplorationFogOuterMaterial = SolidTerrainFog::CreateOuterMaterial(this);
	}
}

void ASolidTerrainStreamer::BuildChunkActor(ASolidTerrainChunk* Chunk, FIntPoint Coord)
{
	if (!Chunk)
	{
		return;
	}

	UMaterialInterface* FogHalf = nullptr;
	UMaterialInterface* FogFull = nullptr;
	UMaterialInterface* FogOuter = nullptr;
	if (bRenderExplorationFogMeshes)
	{
		EnsureExplorationFogMaterials();
		FogHalf = ExplorationFogHalfMaterial;
		FogFull = ExplorationFogFullMaterial;
		FogOuter = ExplorationFogOuterMaterial;
	}

	TMap<ESolidBiome, UMaterialInterface*> BiomeMaterials;
	CollectBiomeMaterials(BiomeMaterials);

	Chunk->BuildChunk(
		Coord,
		ChunkWorldSize,
		QuadsPerSide,
		Seed,
		FrequencyScale,
		Amplitude,
		BaseHeight,
		CollisionHeightBias,
		ResolveMaterial(),
		FogHalf,
		FogFull,
		FogOuter,
		TerrainMap,
		&BiomeMaterials,
		FogQuadsPerSide,
		FogVolumeHeightCm,
		FogVolumeHeightHalfCm);
}

void ASolidTerrainStreamer::QueueExplorationFogMeshRefresh(float WorldX, float WorldY, float RadiusCm)
{
	const float ChunkRadius = ChunkWorldSize * 0.75f;
	const float Reach = RadiusCm + ChunkRadius;
	const float ReachSq = Reach * Reach;
	for (const TPair<FIntPoint, TObjectPtr<ASolidTerrainChunk>>& Pair : LoadedChunks)
	{
		if (!Pair.Value)
		{
			continue;
		}
		const FVector ChunkCenter(
			(static_cast<float>(Pair.Key.X) + 0.5f) * ChunkWorldSize,
			(static_cast<float>(Pair.Key.Y) + 0.5f) * ChunkWorldSize,
			0.f);
		const float DX = ChunkCenter.X - WorldX;
		const float DY = ChunkCenter.Y - WorldY;
		if ((DX * DX + DY * DY) <= ReachSq)
		{
			DirtyFogChunkCoords.Add(Pair.Key);
		}
	}
}

void ASolidTerrainStreamer::ProcessExplorationFogMeshRebuilds()
{
	if (DirtyFogChunkCoords.Num() == 0)
	{
		return;
	}

	EnsureExplorationFogMaterials();
	const int32 Budget = FMath::Max(MaxFogChunkRebuildsPerUpdate, 1);
	int32 Rebuilt = 0;
	TArray<FIntPoint> StillDirty;
	StillDirty.Reserve(DirtyFogChunkCoords.Num());

	for (const FIntPoint& Coord : DirtyFogChunkCoords)
	{
		if (Rebuilt >= Budget)
		{
			StillDirty.Add(Coord);
			continue;
		}

		if (ASolidTerrainChunk* Chunk = LoadedChunks.FindRef(Coord))
		{
			Chunk->RebuildExplorationFog(
				ExplorationFogHalfMaterial,
				ExplorationFogFullMaterial,
				ExplorationFogOuterMaterial,
				TerrainMap,
				FogQuadsPerSide,
				FogVolumeHeightCm,
				FogVolumeHeightHalfCm);
			++Rebuilt;
		}
	}

	DirtyFogChunkCoords.Reset();
	for (const FIntPoint& Coord : StillDirty)
	{
		DirtyFogChunkCoords.Add(Coord);
	}
}

void ASolidTerrainStreamer::EnsureHeightFog()
{
	if (!bRenderTerrainFog)
	{
		return;
	}

	if (HeightFogActor && IsValid(HeightFogActor))
	{
		return;
	}

	HeightFogActor = SolidTerrainFog::EnsureHeightFog(GetWorld(), this, FogMistColor);
}

float ASolidTerrainStreamer::SampleViewFogAmount() const
{
	AActor* Focus = ResolveFocusActor();
	if (!Focus)
	{
		return 0.f;
	}
	return SolidTerrainFog::SampleMistAmountAround(TerrainMap, Focus->GetActorLocation());
}

void ASolidTerrainStreamer::UpdateTerrainFog(float DeltaSeconds)
{
	const float TargetFog = SampleViewFogAmount();
	if (DeltaSeconds <= 0.f)
	{
		RenderedFogAmount = TargetFog;
	}
	else
	{
		RenderedFogAmount = FMath::FInterpTo(RenderedFogAmount, TargetFog, DeltaSeconds, FogInterpSpeed);
	}

	if (!bRenderTerrainFog)
	{
		if (!bHeightFogSilenced)
		{
			SolidTerrainFog::SilenceHeightFog(GetWorld());
			bHeightFogSilenced = true;
		}
		return;
	}
	bHeightFogSilenced = false;

	EnsureHeightFog();

	SolidTerrainFog::FHeightFogStyle Style;
	Style.DensityAtHalf = FogDensityAtHalf;
	Style.DensityAtFull = FogDensityAtFull;
	Style.MaxOpacityAtHalf = FogMaxOpacityAtHalf;
	Style.MaxOpacityAtFull = FogMaxOpacityAtFull;
	Style.MistColor = FogMistColor;
	SolidTerrainFog::ApplyHeightFogAmount(HeightFogActor, RenderedFogAmount, Style);
}

FIntPoint ASolidTerrainStreamer::WorldToChunkCoord(const FVector& WorldLocation) const
{
	const float Size = FMath::Max(ChunkWorldSize, 100.f);
	return FIntPoint(
		FMath::FloorToInt(WorldLocation.X / Size),
		FMath::FloorToInt(WorldLocation.Y / Size));
}

AActor* ASolidTerrainStreamer::ResolveFocusActor() const
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

UMaterialInterface* ASolidTerrainStreamer::ResolveMaterial() const
{
	SolidTerrainMaterials::FResolveParams Params;
	Params.Outer = const_cast<ASolidTerrainStreamer*>(this);
	Params.OverrideMaterial = TerrainMaterial;
	Params.GrassColor = GrassColor;
	Params.GrassDarkColor = GrassDarkColor;
	Params.GrassRoughness = GrassRoughness;
	Params.GrassSpecular = GrassSpecular;

	return SolidTerrainMaterials::Resolve(
		Params, const_cast<ASolidTerrainStreamer*>(this)->ResolvedTerrainMaterial);
}

UMaterialInterface* ASolidTerrainStreamer::ResolveBiomeMaterial(const ESolidBiome Biome) const
{
	SolidTerrainMaterials::FResolveParams Params;
	Params.Outer = const_cast<ASolidTerrainStreamer*>(this);
	Params.OverrideMaterial = TerrainMaterial;
	Params.GrassColor = GrassColor;
	Params.GrassDarkColor = GrassDarkColor;
	Params.GrassRoughness = GrassRoughness;
	Params.GrassSpecular = GrassSpecular;

	const USolidWorldMap* WorldMap = TerrainMap ? TerrainMap->GetWorldMap() : nullptr;
	return SolidTerrainMaterials::ResolveForBiome(
		Biome,
		Params,
		WorldMap,
		const_cast<ASolidTerrainStreamer*>(this)->ResolvedTerrainMaterial,
		const_cast<ASolidTerrainStreamer*>(this)->ResolvedBiomeMaterials);
}

void ASolidTerrainStreamer::CollectBiomeMaterials(
	TMap<ESolidBiome, UMaterialInterface*>& OutMaterials) const
{
	static const ESolidBiome AllBiomes[] = {
		ESolidBiome::Grassland,
		ESolidBiome::Forest,
		ESolidBiome::Mountain,
		ESolidBiome::Town,
		ESolidBiome::Desert,
		ESolidBiome::Swamp,
		ESolidBiome::Sea,
		ESolidBiome::River,
	};

	OutMaterials.Reset();
	for (const ESolidBiome Biome : AllBiomes)
	{
		OutMaterials.Add(Biome, ResolveBiomeMaterial(Biome));
	}
}

void ASolidTerrainStreamer::RelocateCompanionsByDelta(const FVector& DeltaXY)
{
	UWorld* World = GetWorld();
	if (!World || DeltaXY.IsNearlyZero())
	{
		return;
	}

	int32 Moved = 0;
	for (TActorIterator<ASolidCompanionCharacter> It(World); It; ++It)
	{
		ASolidCompanionCharacter* Companion = *It;
		if (!IsValid(Companion))
		{
			continue;
		}

		const FVector OldLoc = Companion->GetActorLocation();
		const FVector Planned(OldLoc.X + DeltaXY.X, OldLoc.Y + DeltaXY.Y, OldLoc.Z);
		const float LandZ = SampleHeightAtWorld(Planned) + CollisionHeightBias;
		float CapsuleHalfHeight = 96.f;
		if (const UCapsuleComponent* Capsule = Companion->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}

		const FVector NewLoc(Planned.X, Planned.Y, LandZ + CapsuleHalfHeight + SnapHeightPadding);
		Companion->SetActorLocation(NewLoc);
		if (UCharacterMovementComponent* Move = Companion->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			Move->SetMovementMode(MOVE_Walking);
		}
		++Moved;
	}

	if (Moved > 0)
	{
		UE_LOG(LogSolid, Warning,
			TEXT("Relocated %d companion(s) with start-town delta (%.0f, %.0f)"),
			Moved, DeltaXY.X, DeltaXY.Y);
	}
}

void ASolidTerrainStreamer::CenterExplorationFogOnFocus()
{
	if (bDidCenterFogOnPlayer || !TerrainMap || !TerrainMap->IsBuilt() || !bDidRelocateToStartTown)
	{
		return;
	}

	AActor* Focus = ResolveFocusActor();
	if (!Focus)
	{
		return;
	}

	const FVector Loc = Focus->GetActorLocation();
	TerrainMap->CenterExplorationFogOn(Loc.X, Loc.Y);
	bDidCenterFogOnPlayer = true;
	bHasFogApplyLocation = false;

	if (bRenderExplorationFogMeshes)
	{
		QueueExplorationFogMeshRefresh(
			Loc.X, Loc.Y,
			SolidTerrainFog::MetersToCm(SolidTerrainFog::FullFogStartMeters) + ChunkWorldSize * 0.5f);
	}

	UE_LOG(LogSolid, Warning,
		TEXT("Centered exploration fog on player (%.0f, %.0f)"),
		Loc.X, Loc.Y);
}

void ASolidTerrainStreamer::TryRelocateFocusToStartTown()
{
	if (bDidRelocateToStartTown)
	{
		return;
	}

	EnsureTerrainMap();
	if (!TerrainMap || !TerrainMap->IsBuilt())
	{
		return;
	}

	FVector2D TownXY = FVector2D::ZeroVector;
	if (!TerrainMap->GetStartTownWorldXY(TownXY))
	{
		bDidRelocateToStartTown = true;
		return;
	}

	AActor* Focus = ResolveFocusActor();
	if (!Focus)
	{
		return;
	}

	// Stand in the town plaza, offset from the start-town centroid.
	const FVector2D PawnXY = TownXY + StartTownPawnOffsetXY;

	const FVector OldLoc = Focus->GetActorLocation();
	const float LandZ = SampleHeightAtWorld(FVector(PawnXY.X, PawnXY.Y, 0.f)) + CollisionHeightBias;
	float CapsuleHalfHeight = 96.f;
	if (const ACharacter* Character = Cast<ACharacter>(Focus))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const FVector NewLoc(PawnXY.X, PawnXY.Y, LandZ + CapsuleHalfHeight + SnapHeightPadding);
	const FVector Delta(NewLoc.X - OldLoc.X, NewLoc.Y - OldLoc.Y, 0.f);

	Focus->SetActorLocation(NewLoc);

	// Face the start-town centroid.
	const FVector2D ToTown = TownXY - PawnXY;
	if (!ToTown.IsNearlyZero())
	{
		const float YawDeg = FMath::RadiansToDegrees(FMath::Atan2(ToTown.Y, ToTown.X));
		Focus->SetActorRotation(FRotator(0.f, YawDeg, 0.f));
	}

	if (ACharacter* Character = Cast<ACharacter>(Focus))
	{
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			Move->SetMovementMode(MOVE_Walking);
		}
	}

	RelocateCompanionsByDelta(Delta);

	bDidRelocateToStartTown = true;
	bHasFogApplyLocation = false;
	UE_LOG(LogSolid, Warning,
		TEXT("Relocated focus to start town pawn (%.0f, %.0f) town=(%.0f, %.0f) Z=%.1f (from %.0f, %.0f)"),
		PawnXY.X, PawnXY.Y, TownXY.X, TownXY.Y, NewLoc.Z, OldLoc.X, OldLoc.Y);
}

float ASolidTerrainStreamer::SampleHeightAtWorld(const FVector& WorldLocation) const
{
	if (TerrainMap && TerrainMap->IsBuilt())
	{
		return TerrainMap->SampleHeight(WorldLocation.X, WorldLocation.Y);
	}

	return SolidTerrainNoise::SampleHeight(
		WorldLocation.X, WorldLocation.Y, Seed, FrequencyScale, Amplitude, BaseHeight);
}

void ASolidTerrainStreamer::DisableLandscapeActorsOnce()
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
		UE_LOG(LogSolid, Warning, TEXT("Disabled %d Landscape actor(s) for procedural terrain."), Count);
	}
}

void ASolidTerrainStreamer::TrySnapFocusToTerrain(AActor* Focus)
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

	UE_LOG(LogSolid, Warning, TEXT("Snapped focus onto terrain Z=%.1f at (%.0f, %.0f)"), LandZ, Loc.X, Loc.Y);
}

void ASolidTerrainStreamer::UpdateStreaming()
{
	EnsureTerrainMap();
	TryRelocateFocusToStartTown();
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
	for (const TPair<FIntPoint, TObjectPtr<ASolidTerrainChunk>>& Pair : LoadedChunks)
	{
		if (!Desired.Contains(Pair.Key))
		{
			ToRemove.Add(Pair.Key);
		}
	}

	for (const FIntPoint& Key : ToRemove)
	{
		if (ASolidTerrainChunk* Chunk = LoadedChunks.FindRef(Key))
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

	EnsureExplorationFogMaterials();

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

		ASolidTerrainChunk* Chunk = World->SpawnActor<ASolidTerrainChunk>(
			ASolidTerrainChunk::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		if (!Chunk)
		{
			UE_LOG(LogSolid, Error, TEXT("Failed to spawn terrain chunk at %d,%d"), Coord.X, Coord.Y);
			continue;
		}

		BuildChunkActor(Chunk, Coord);

		LoadedChunks.Add(Coord, Chunk);
		UE_LOG(LogSolid, Warning, TEXT("Built terrain chunk (%d, %d) at origin (%.0f, %.0f). Loaded=%d"),
			Coord.X, Coord.Y,
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			LoadedChunks.Num());
	}

	TrySnapFocusToTerrain(Focus);
}
