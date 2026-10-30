#include "SolidTerrainStreamer.h"
#include "SolidGameMode.h"
#include "Menus/SolidSquadBuilder.h"
#include "SolidHUD.h"
#include "Lairs/SolidLair.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "SolidGameInstance.h"
#include "Company/SolidCompany.h"
#include "SolidStronghold.h"
#include "SolidTerrainChunk.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainMaterials.h"
#include "SolidTerrainNoise.h"
#include "SolidWorldChart.h"
#include "SolidCore1.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Player/SolidCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Materials/MaterialInterface.h"

namespace
{
	bool FocusWantsEagleMap(const AActor* Focus)
	{
		const ASolidCharacter* Character = Cast<ASolidCharacter>(Focus);
		return Character && Character->IsEagleSight();
	}
}

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
	if (bChartView)
	{
		return;
	}

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
	if (bChartView)
	{
		return;
	}

	Super::Tick(DeltaSeconds);

	UpdateTerrainFog(DeltaSeconds);

	const bool bEagleNow = FocusWantsEagleMap(ResolveFocusActor());
	if (bEagleNow != bEagleMapStream)
	{
		TimeSinceUpdate = UpdateIntervalSeconds;
	}

	TimeSinceUpdate += DeltaSeconds;
	const bool bInterval = TimeSinceUpdate >= UpdateIntervalSeconds;
	if (!bInterval && !bEagleNow && !bStreamWorkLeft)
	{
		return;
	}

	if (bInterval)
	{
		TimeSinceUpdate = 0.f;
		TryRelocateFocusToStartTown();
		CenterExplorationFogOnFocus();
		ClearExplorationFogAtFocus();
	}

	UpdateStreaming();
	if (bInterval && bRenderExplorationFogMeshes)
	{
		ProcessExplorationFogMeshRebuilds();
	}
}

void ASolidTerrainStreamer::ClearAllExplorationFog()
{
	if (!TerrainMap)
	{
		return;
	}

	TerrainMap->ClearAllExplorationFog();
	if (UWorld* World = GetWorld())
	{
		if (USolidGameInstance* Instance = World->GetGameInstance<USolidGameInstance>())
		{
			TArray<float> Fog;
			TerrainMap->CopyExplorationFog(Fog);
			Instance->StoreExplorationFog(Fog);
		}
	}

	for (const TPair<FIntPoint, TObjectPtr<ASolidTerrainChunk>>& Pair : LoadedChunks)
	{
		if (Pair.Value)
		{
			DirtyFogChunkCoords.Add(Pair.Key);
		}
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
	TArray<FVector2D> ChangedXY;
	const int32 Changed = TerrainMap->ApplyExplorationFogAround(
		Loc.X,
		Loc.Y,
		bHasFogApplyLocation,
		LastFogApplyLocation.X,
		LastFogApplyLocation.Y,
		&ChangedXY);
	LastFogApplyLocation = Loc;
	bHasFogApplyLocation = true;
	if (ASolidGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<ASolidGameMode>() : nullptr)
	{
		Mode->SeedForagePiles();
		Mode->SeedMonsters();
	}

	if (Changed > 0 && bRenderExplorationFogMeshes)
	{
		for (const FVector2D& Point : ChangedXY)
		{
			MarkFogChunksAt(Point.X, Point.Y);
		}
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

void ASolidTerrainStreamer::ChunkSpan(
	const FVector2D& WorldMin,
	const FVector2D& WorldMax,
	float ChunkSize,
	FIntPoint& OutMin,
	FIntPoint& OutMax)
{
	const float Size = FMath::Max(ChunkSize, 100.f);
	OutMin = FIntPoint(
		FMath::FloorToInt(WorldMin.X / Size),
		FMath::FloorToInt(WorldMin.Y / Size));
	OutMax = FIntPoint(
		FMath::FloorToInt((WorldMax.X - 1.f) / Size),
		FMath::FloorToInt((WorldMax.Y - 1.f) / Size));
}

int32 ASolidTerrainStreamer::EagleStreamRadius(
	float HeightAboveGroundCm,
	float ChunkSizeCm,
	int32 NearbyRadius,
	int32 MapRadius)
{
	const float Size = FMath::Max(ChunkSizeCm, 1.f);
	const int32 FromHeight = FMath::CeilToInt(FMath::Max(HeightAboveGroundCm, 0.f) / Size);
	const int32 Upper = FMath::Max(MapRadius, NearbyRadius);
	return FMath::Clamp(FMath::Max(FromHeight, NearbyRadius), 0, Upper);
}

void ASolidTerrainStreamer::BuildChunkActor(
	ASolidTerrainChunk* Chunk, FIntPoint Coord, int32 Quads, int32 FogQuads)
{
	if (!Chunk)
	{
		return;
	}

	const int32 TerrainQuads = Quads > 0 ? Quads : QuadsPerSide;
	const int32 FogCells = FogQuads > 0 ? FogQuads : FogQuadsPerSide;

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
		TerrainQuads,
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
		FogCells,
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

void ASolidTerrainStreamer::MarkFogChunksAt(float WorldX, float WorldY)
{
	const float Size = FMath::Max(ChunkWorldSize, 100.f);
	const FIntPoint Coord = WorldToChunkCoord(FVector(WorldX, WorldY, 0.f));
	DirtyFogChunkCoords.Add(Coord);
	const float LocalX = WorldX - static_cast<float>(Coord.X) * Size;
	const float LocalY = WorldY - static_cast<float>(Coord.Y) * Size;
	const float Edge = FMath::Max(TerrainPointSpacing, 50.f);
	if (LocalX <= Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X - 1, Coord.Y));
	}
	if (LocalX >= Size - Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X + 1, Coord.Y));
	}
	if (LocalY <= Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X, Coord.Y - 1));
	}
	if (LocalY >= Size - Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X, Coord.Y + 1));
	}
	if (LocalX <= Edge && LocalY <= Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X - 1, Coord.Y - 1));
	}
	if (LocalX >= Size - Edge && LocalY <= Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X + 1, Coord.Y - 1));
	}
	if (LocalX <= Edge && LocalY >= Size - Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X - 1, Coord.Y + 1));
	}
	if (LocalX >= Size - Edge && LocalY >= Size - Edge)
	{
		DirtyFogChunkCoords.Add(FIntPoint(Coord.X + 1, Coord.Y + 1));
	}
}

void ASolidTerrainStreamer::ProcessExplorationFogMeshRebuilds()
{
	if (DirtyFogChunkCoords.Num() == 0)
	{
		return;
	}

	EnsureExplorationFogMaterials();
	for (const FIntPoint& Coord : DirtyFogChunkCoords)
	{
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
		}
	}
	DirtyFogChunkCoords.Reset();
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

	const USolidWorldChart* WorldMap = TerrainMap ? TerrainMap->GetWorldMap() : nullptr;
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
		ESolidBiome::Road,
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
		if (!IsValid(Companion) || Companion->IsStagePreview())
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
	bool bRestoredTrail = false;
	UWorld* World = GetWorld();
	ASolidGameMode* Mode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (Mode && !Mode->IsInClosedVisit())
	{
		if (const USolidGameInstance* Instance = World->GetGameInstance<USolidGameInstance>())
		{
			if (Instance->HasExplorationFog())
			{
				bRestoredTrail = TerrainMap->ApplyExplorationFog(Instance->GetExplorationFog());
			}
		}
	}
	if (!bRestoredTrail)
	{
		TerrainMap->CenterExplorationFogOn(Loc.X, Loc.Y);
	}
	bDidCenterFogOnPlayer = true;
	if (Mode)
	{
		Mode->SeedForagePiles();
		Mode->SeedMonsters();
	}
	bHasFogApplyLocation = false;

	if (bRenderExplorationFogMeshes)
	{
		QueueExplorationFogMeshRefresh(
			Loc.X, Loc.Y,
			SolidTerrainFog::MetersToCm(SolidTerrainFog::FullFogStartMeters) + ChunkWorldSize * 0.5f);
	}

	UE_LOG(LogSolid, Warning,
		TEXT("%s exploration fog at player (%.0f, %.0f)"),
		bRestoredTrail ? TEXT("Restored") : TEXT("Centered"),
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

	AActor* Focus = ResolveFocusActor();
	if (!Focus)
	{
		return;
	}

	FVector2D PawnXY = FVector2D::ZeroVector;
	float FacingYawDeg = 0.f;
	float FacingPitchDeg = 0.f;
	bool bHaveYaw = false;
	bool bCaveMouth = false;
	bool bStrongholdLeave = false;

	if (UWorld* World = GetWorld())
	{
		if (USolidGameInstance* Instance = World->GetGameInstance<USolidGameInstance>())
		{
			if (Instance->WantsStrongholdExit())
			{
				const USolidCompany* Company = Instance->GetCompany();
				const FSolidStronghold* Stronghold = Company ? Company->GetStronghold() : nullptr;
				if (Stronghold)
				{
					const SolidStronghold::FExitStand Stand = SolidStronghold::ExitStand(*Stronghold);
					PawnXY = Stand.XY;
					FacingYawDeg = Stand.YawDeg;
					FacingPitchDeg = Stand.PitchDeg;
					bHaveYaw = true;
					bCaveMouth = true;
					if (const APawn* Pawn = Cast<APawn>(Focus))
					{
						if (!Pawn->Controller)
						{
							return;
						}
					}
					bStrongholdLeave = true;
					Instance->ClearStrongholdExit();
				}
				else
				{
					Instance->ClearStrongholdExit();
				}
			}
			else if (Instance->WantsLairMouthSpawn() && TerrainMap->GetWorldMap())
			{
				TArray<SolidLair::FEntrance> Entrances;
				SolidLair::CollectEntrances(
					TerrainMap->GetWorldMap(),
					TerrainMap->GetWorldMinXY(),
					TerrainMap->GetWorldMaxXY(),
					Entrances);
				const TCHAR End = Instance->GetSession().LairEnd;
				const SolidLair::FEntrance* Chosen = nullptr;
				for (const SolidLair::FEntrance& Entrance : Entrances)
				{
					if (Entrance.EndMarker == End)
					{
						Chosen = &Entrance;
						break;
					}
				}
				if (Chosen)
				{
					PawnXY = Chosen->WorldXY;
					FacingYawDeg = Chosen->FacingYawDeg;
					FacingPitchDeg = 0.f;
					bHaveYaw = true;
					bCaveMouth = true;
					if (const APawn* Pawn = Cast<APawn>(Focus))
					{
						if (!Pawn->Controller)
						{
							return;
						}
					}
					// A lair is not a stronghold leave. Build squad stays closed.
					Instance->ClearLairMouthSpawn();
				}
			}
		}
	}

	if (!bCaveMouth)
	{
		FVector2D TownXY = FVector2D::ZeroVector;
		if (!TerrainMap->GetStartTownWorldXY(TownXY))
		{
			bDidRelocateToStartTown = true;
			return;
		}
		// Stand in the town plaza, offset from the start-town centroid.
		PawnXY = TownXY + StartTownPawnOffsetXY;
		const FVector2D ToTown = TownXY - PawnXY;
		if (!ToTown.IsNearlyZero())
		{
			FacingYawDeg = FMath::RadiansToDegrees(FMath::Atan2(ToTown.Y, ToTown.X));
			bHaveYaw = true;
		}
	}

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

	if (bHaveYaw)
	{
		Focus->SetActorRotation(FRotator(0.f, FacingYawDeg, 0.f));
		if (APawn* Pawn = Cast<APawn>(Focus))
		{
			if (Pawn->Controller)
			{
				Pawn->Controller->SetControlRotation(FRotator(FacingPitchDeg, FacingYawDeg, 0.f));
			}
		}
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
	if (bStrongholdLeave && bHaveYaw)
	{
		if (UWorld* PartyWorld = GetWorld())
		{
			for (TActorIterator<ASolidCompanionCharacter> It(PartyWorld); It; ++It)
			{
				ASolidCompanionCharacter* Companion = *It;
				if (!IsValid(Companion) || Companion->IsStagePreview())
				{
					continue;
				}
				Companion->SetActorRotation(FRotator(0.f, FacingYawDeg, 0.f));
			}
		}
	}

	bDidRelocateToStartTown = true;
	bHasFogApplyLocation = false;
	if (bStrongholdLeave)
	{
		ASolidHUD* HUD = nullptr;
		const USolidSquad* Squad = nullptr;
		int32 MaxSquad = 0;
		if (UWorld* World = GetWorld())
		{
			if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
			{
				Squad = GameMode->GetSquad();
				MaxSquad = GameMode->GetMaxSquadSize();
			}
			if (const APawn* Pawn = Cast<APawn>(Focus))
			{
				if (APlayerController* Controller = Cast<APlayerController>(Pawn->GetController()))
				{
					HUD = Cast<ASolidHUD>(Controller->GetHUD());
				}
			}
		}
		SolidSquadBuilder::OpenAfterStrongholdLeave(HUD, Squad, MaxSquad);
	}
	UE_LOG(LogSolid, Warning,
		TEXT("Relocated focus to %s (%.0f, %.0f) Z=%.1f (from %.0f, %.0f)"),
		bStrongholdLeave ? TEXT("stronghold entrance") : (bCaveMouth ? TEXT("cave mouth") : TEXT("start town")),
		PawnXY.X, PawnXY.Y, NewLoc.Z, OldLoc.X, OldLoc.Y);
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
	if (bChartView)
	{
		return;
	}

	EnsureTerrainMap();
	TryRelocateFocusToStartTown();
	DisableLandscapeActorsOnce();

	AActor* Focus = ResolveFocusActor();
	if (!Focus)
	{
		return;
	}

	const bool bEagle = FocusWantsEagleMap(Focus);
	bEagleMapStream = bEagle;

	FVector ViewLocation = Focus->GetActorLocation();
	if (const APawn* Pawn = Cast<APawn>(Focus))
	{
		if (const APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				ViewLocation = PC->PlayerCameraManager->GetCameraLocation();
			}
		}
	}

	TSet<FIntPoint> Desired;
	FIntPoint StreamCenter = WorldToChunkCoord(bEagle ? ViewLocation : Focus->GetActorLocation());
	if (bEagleMapStream && TerrainMap && TerrainMap->IsBuilt())
	{
		FIntPoint MinChunk;
		FIntPoint MaxChunk;
		ChunkSpan(TerrainMap->GetWorldMinXY(), TerrainMap->GetWorldMaxXY(), ChunkWorldSize, MinChunk, MaxChunk);
		const int32 MapRadius = FMath::Max(
			FMath::Max(FMath::Abs(StreamCenter.X - MinChunk.X), FMath::Abs(StreamCenter.X - MaxChunk.X)),
			FMath::Max(FMath::Abs(StreamCenter.Y - MinChunk.Y), FMath::Abs(StreamCenter.Y - MaxChunk.Y)));
		const float Height = ViewLocation.Z - GetHeightAt(ViewLocation);
		const int32 Radius = EagleStreamRadius(Height, ChunkWorldSize, ViewRadiusChunks, MapRadius);
		for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; ++Y)
		{
			for (int32 X = MinChunk.X; X <= MaxChunk.X; ++X)
			{
				const int32 Chebyshev = FMath::Max(FMath::Abs(X - StreamCenter.X), FMath::Abs(Y - StreamCenter.Y));
				if (Chebyshev <= Radius)
				{
					Desired.Add(FIntPoint(X, Y));
				}
			}
		}
	}
	else
	{
		Desired.Reserve((ViewRadiusChunks * 2 + 1) * (ViewRadiusChunks * 2 + 1));
		for (int32 DY = -ViewRadiusChunks; DY <= ViewRadiusChunks; ++DY)
		{
			for (int32 DX = -ViewRadiusChunks; DX <= ViewRadiusChunks; ++DX)
			{
				Desired.Add(FIntPoint(StreamCenter.X + DX, StreamCenter.Y + DY));
			}
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

	int32 Removed = 0;
	for (const FIntPoint& Key : ToRemove)
	{
		if (bEagleMapStream || Removed >= EagleChunksPerUpdate)
		{
			break;
		}
		if (ASolidTerrainChunk* Chunk = LoadedChunks.FindRef(Key))
		{
			Chunk->Destroy();
		}
		LoadedChunks.Remove(Key);
		++Removed;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		bStreamWorkLeft = Removed < ToRemove.Num();
		return;
	}

	EnsureExplorationFogMaterials();

	TArray<FIntPoint> ToBuild;
	for (const FIntPoint& Coord : Desired)
	{
		if (!LoadedChunks.Contains(Coord))
		{
			ToBuild.Add(Coord);
		}
	}
	if (bEagleMapStream)
	{
		ToBuild.Sort([StreamCenter](const FIntPoint& A, const FIntPoint& B)
		{
			const int32 DA = FMath::Max(FMath::Abs(A.X - StreamCenter.X), FMath::Abs(A.Y - StreamCenter.Y));
			const int32 DB = FMath::Max(FMath::Abs(B.X - StreamCenter.X), FMath::Abs(B.Y - StreamCenter.Y));
			if (DA != DB)
			{
				return DA < DB;
			}
			if (A.Y != B.Y)
			{
				return A.Y < B.Y;
			}
			return A.X < B.X;
		});
	}

	int32 Built = 0;
	for (const FIntPoint& Coord : ToBuild)
	{
		if (bEagleMapStream && Built >= EagleChunksPerUpdate)
		{
			break;
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

		if (bEagleMapStream)
		{
			BuildChunkActor(Chunk, Coord, EagleQuadsPerSide, EagleFogQuadsPerSide);
		}
		else
		{
			BuildChunkActor(Chunk, Coord);
		}

		LoadedChunks.Add(Coord, Chunk);
		++Built;
		UE_LOG(LogSolid, Warning, TEXT("Built terrain chunk (%d, %d) at origin (%.0f, %.0f). Loaded=%d"),
			Coord.X, Coord.Y,
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			LoadedChunks.Num());
	}

	bStreamWorkLeft = Removed < ToRemove.Num() || Built < ToBuild.Num();
	TrySnapFocusToTerrain(Focus);
}

namespace
{
	void KeepLevelClean(UPackage* Package, const bool bWasDirty)
	{
#if WITH_EDITOR
		if (Package && !bWasDirty)
		{
			Package->SetDirtyFlag(false);
		}
#else
		(void)Package;
		(void)bWasDirty;
#endif
	}
}

void ASolidTerrainStreamer::ClearChartView()
{
#if WITH_EDITOR
	RestoreChartLandscapes();
#endif
	for (const TPair<FIntPoint, TObjectPtr<ASolidTerrainChunk>>& Pair : LoadedChunks)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	LoadedChunks.Empty();
	bChartView = false;
}

void ASolidTerrainStreamer::BuildChartView(const USolidWorldChart* Chart)
{
	bChartView = true;
	bRenderExplorationFogMeshes = false;

	UPackage* LevelPackage = nullptr;
	bool bWasDirty = false;
#if WITH_EDITOR
	if (ULevel* Level = GetLevel())
	{
		LevelPackage = Level->GetOutermost();
		bWasDirty = LevelPackage && LevelPackage->IsDirty();
	}
#endif

	for (const TPair<FIntPoint, TObjectPtr<ASolidTerrainChunk>>& Pair : LoadedChunks)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	LoadedChunks.Empty();

	UWorld* World = GetWorld();
	if (!World || !Chart)
	{
		KeepLevelClean(LevelPackage, bWasDirty);
		return;
	}

	USolidWorldChart* Map = NewObject<USolidWorldChart>(this);
	if (!Map->LoadFromChart(Chart))
	{
		KeepLevelClean(LevelPackage, bWasDirty);
		return;
	}

	if (!TerrainMap)
	{
		TerrainMap = NewObject<USolidTerrainMap>(this, TEXT("TerrainMap"));
	}

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
		true,
		Map);

#if WITH_EDITOR
	HideChartLandscapes();
#endif

	const FVector2D WorldMin = TerrainMap->GetWorldMinXY();
	const FVector2D WorldMax = TerrainMap->GetWorldMaxXY();
	const FIntPoint MinChunk = WorldToChunkCoord(FVector(WorldMin.X, WorldMin.Y, 0.f));
	const FIntPoint MaxChunk = WorldToChunkCoord(FVector(WorldMax.X - 1.f, WorldMax.Y - 1.f, 0.f));
	for (int32 Y = MinChunk.Y; Y <= MaxChunk.Y; ++Y)
	{
		for (int32 X = MinChunk.X; X <= MaxChunk.X; ++X)
		{
			const FIntPoint Coord(X, Y);
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
#if WITH_EDITOR
			SpawnParams.bHideFromSceneOutliner = true;
			SpawnParams.ObjectFlags = RF_Transient;
#endif
			const FVector SpawnLoc(
				static_cast<float>(Coord.X) * ChunkWorldSize,
				static_cast<float>(Coord.Y) * ChunkWorldSize,
				0.f);
			ASolidTerrainChunk* Chunk = World->SpawnActor<ASolidTerrainChunk>(
				ASolidTerrainChunk::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
			if (!Chunk)
			{
				continue;
			}
#if WITH_EDITOR
			Chunk->bIsEditorOnlyActor = true;
#endif
			BuildChunkActor(Chunk, Coord);
			LoadedChunks.Add(Coord, Chunk);
		}
	}

	KeepLevelClean(LevelPackage, bWasDirty);
	UE_LOG(LogSolid, Warning, TEXT("World chart terrain: %d chunks."), LoadedChunks.Num());
}

#if WITH_EDITOR
void ASolidTerrainStreamer::HideChartLandscapes()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* LandscapeClass = FindObject<UClass>(nullptr, TEXT("/Script/Landscape.Landscape"));
	UClass* LandscapeProxyClass = FindObject<UClass>(nullptr, TEXT("/Script/Landscape.LandscapeProxy"));
	UClass* StreamingProxyClass = FindObject<UClass>(nullptr, TEXT("/Script/Landscape.LandscapeStreamingProxy"));
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UClass* ActorClass = It->GetClass();
		const bool bIsLandscape =
			(LandscapeClass && ActorClass->IsChildOf(LandscapeClass))
			|| (LandscapeProxyClass && ActorClass->IsChildOf(LandscapeProxyClass))
			|| (StreamingProxyClass && ActorClass->IsChildOf(StreamingProxyClass));
		if (!bIsLandscape || It->IsTemporarilyHiddenInEditor())
		{
			continue;
		}
		It->SetIsTemporarilyHiddenInEditor(true);
		ChartHiddenLandscapes.Add(*It);
	}
}

void ASolidTerrainStreamer::RestoreChartLandscapes()
{
	for (const TWeakObjectPtr<AActor>& Actor : ChartHiddenLandscapes)
	{
		if (Actor.IsValid())
		{
			Actor->SetIsTemporarilyHiddenInEditor(false);
		}
	}
	ChartHiddenLandscapes.Reset();
}
#endif

void ASolidTerrainStreamer::RebuildChartView(const USolidWorldChart* Chart, const TArray<FIntPoint>& Cells)
{
	RebuildChartGround(Chart, Cells, TArray<FVector2D>());
}

void ASolidTerrainStreamer::RebuildChartPoints(const USolidWorldChart* Chart, const TArray<FVector2D>& Positions)
{
	RebuildChartGround(Chart, TArray<FIntPoint>(), Positions);
}

void ASolidTerrainStreamer::RebuildChartGround(
	const USolidWorldChart* Chart,
	const TArray<FIntPoint>& Cells,
	const TArray<FVector2D>& Positions)
{
	if (!bChartView || !TerrainMap || !Chart)
	{
		BuildChartView(Chart);
		return;
	}

	UPackage* LevelPackage = nullptr;
	bool bWasDirty = false;
#if WITH_EDITOR
	if (ULevel* Level = GetLevel())
	{
		LevelPackage = Level->GetOutermost();
		bWasDirty = LevelPackage && LevelPackage->IsDirty();
	}
#endif

	USolidWorldChart* Map = NewObject<USolidWorldChart>(this);
	if (!Map->LoadFromChart(Chart))
	{
		KeepLevelClean(LevelPackage, bWasDirty);
		return;
	}

	float Spacing = TerrainPointSpacing;
	if (QuadsPerSide > 0)
	{
		Spacing = ChunkWorldSize / static_cast<float>(QuadsPerSide);
	}
	bRenderExplorationFogMeshes = false;
	TerrainMap->Build(
		Seed,
		FrequencyScale,
		Amplitude,
		BaseHeight,
		TerrainMapSize,
		TerrainMapSize,
		Spacing,
		true,
		Map);

	UWorld* World = GetWorld();
	const USolidWorldChart* WorldMap = TerrainMap->GetWorldMap();
	TSet<FIntPoint> Chunks;
	if (WorldMap)
	{
		for (const FIntPoint& Cell : Cells)
		{
			FVector2D Centre = FVector2D::ZeroVector;
			if (!WorldMap->CellToWorldXY(Cell, TerrainMap->GetWorldMinXY(), TerrainMap->GetWorldMaxXY(), Centre))
			{
				continue;
			}
			Chunks.Add(WorldToChunkCoord(FVector(Centre.X, Centre.Y, 0.f)));
		}
	}
	for (const FVector2D& Position : Positions)
	{
		Chunks.Add(WorldToChunkCoord(FVector(Position.X, Position.Y, 0.f)));
	}

	for (const FIntPoint& Coord : Chunks)
	{
		ASolidTerrainChunk* Chunk = LoadedChunks.FindRef(Coord);
		if (!Chunk && World)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
#if WITH_EDITOR
			SpawnParams.bHideFromSceneOutliner = true;
			SpawnParams.ObjectFlags = RF_Transient;
#endif
			const FVector SpawnLoc(
				static_cast<float>(Coord.X) * ChunkWorldSize,
				static_cast<float>(Coord.Y) * ChunkWorldSize,
				0.f);
			Chunk = World->SpawnActor<ASolidTerrainChunk>(
				ASolidTerrainChunk::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
			if (Chunk)
			{
#if WITH_EDITOR
				Chunk->bIsEditorOnlyActor = true;
#endif
				LoadedChunks.Add(Coord, Chunk);
			}
		}
		if (Chunk)
		{
			BuildChunkActor(Chunk, Coord);
		}
	}

	KeepLevelClean(LevelPackage, bWasDirty);
}
