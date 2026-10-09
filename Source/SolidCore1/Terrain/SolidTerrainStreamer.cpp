#include "SolidTerrainStreamer.h"
#include "SolidTerrainChunk.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainNoise.h"
#include "SolidCore1.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

ASolidTerrainStreamer::ASolidTerrainStreamer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// TerrainMaterial left null: ResolveMaterial prefers Fab Mat_025_grass, then FlatCol.
	// Do NOT default to M_PrototypeGrid (hard-wired grey checker).
}

ASolidTerrainStreamer* ASolidTerrainStreamer::EnsureExists(UWorld* World)
{
	if (!World || World->bIsTearingDown)
	{
		return nullptr;
	}

	for (TActorIterator<ASolidTerrainStreamer> It(World); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASolidTerrainStreamer* Streamer = World->SpawnActor<ASolidTerrainStreamer>(
		ASolidTerrainStreamer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (Streamer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Spawned SolidTerrainStreamer (EnsureExists)."));
		UE_LOG(LogSolid, Warning, TEXT("Spawned SolidTerrainStreamer (EnsureExists)."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SolidCore1] Failed to spawn SolidTerrainStreamer (EnsureExists)."));
		UE_LOG(LogSolid, Error, TEXT("Failed to spawn SolidTerrainStreamer (EnsureExists)."));
	}

	return Streamer;
}

void ASolidTerrainStreamer::BeginPlay()
{
	Super::BeginPlay();
	EnsureTerrainMap();
	EnsureHeightFog();
	ClearExplorationFogAtFocus();
	TimeSinceUpdate = UpdateIntervalSeconds;
	UpdateStreaming();
	UpdateTerrainFog(0.f);
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

	ClearExplorationFogAtFocus();
	UpdateTerrainFog(DeltaSeconds);

	TimeSinceUpdate += DeltaSeconds;
	if (TimeSinceUpdate < UpdateIntervalSeconds)
	{
		return;
	}

	TimeSinceUpdate = 0.f;
	UpdateStreaming();
}

void ASolidTerrainStreamer::ClearExplorationFogAtFocus()
{
	if (!TerrainMap || !TerrainMap->IsBuilt())
	{
		return;
	}

	if (AActor* Focus = ResolveFocusActor())
	{
		const FVector Loc = Focus->GetActorLocation();
		TerrainMap->ClearFogAround(
			Loc.X, Loc.Y, SolidTerrainFog::MetersToCm(SolidTerrainFog::ClearRadiusMeters));
	}
}

void ASolidTerrainStreamer::EnsureHeightFog()
{
	if (!bRenderTerrainFog)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (HeightFogActor && IsValid(HeightFogActor))
	{
		return;
	}

	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		HeightFogActor = *It;
		break;
	}

	if (!HeightFogActor)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		HeightFogActor = World->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (HeightFogActor)
		{
			UE_LOG(LogSolid, Warning, TEXT("Spawned ExponentialHeightFog for TerrainPoint mist."));
		}
	}

	if (UExponentialHeightFogComponent* FogComp = HeightFogActor ? HeightFogActor->GetComponent() : nullptr)
	{
		FogComp->SetVisibility(true);
		FogComp->SetVolumetricFog(true);
		FogComp->VolumetricFogScatteringDistribution = 0.3f;
		FogComp->VolumetricFogExtinctionScale = 0.8f;
		FogComp->FogHeightFalloff = 0.12f;
		FogComp->SetFogInscatteringColor(FogMistColor);
		// 25% of the previous 50 km volumetric range (matches tighter fog bands).
		FogComp->SetVolumetricFogDistance(12500.f);
	}
}

float ASolidTerrainStreamer::SampleViewFogAmount() const
{
	if (!TerrainMap || !TerrainMap->IsBuilt())
	{
		return 0.f;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	// Drive mist from the same local TerrainPoint fog the HUD shows (pawn/focus),
	// not from forward probes. Probes kept mist~0.6 while standing in fog==0.
	FVector SampleOrigin = FVector::ZeroVector;
	bool bHaveSample = false;
	if (AActor* Focus = ResolveFocusActor())
	{
		SampleOrigin = Focus->GetActorLocation();
		bHaveSample = true;
	}
	else if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			SampleOrigin = Pawn->GetActorLocation();
			bHaveSample = true;
		}
	}

	if (!bHaveSample)
	{
		return 0.f;
	}

	return FMath::Clamp(GetTerrainPointAt(SampleOrigin).Fog, 0.f, 1.f);
}

void ASolidTerrainStreamer::UpdateTerrainFog(float DeltaSeconds)
{
	if (!bRenderTerrainFog)
	{
		return;
	}

	EnsureHeightFog();
	if (!HeightFogActor)
	{
		return;
	}

	UExponentialHeightFogComponent* FogComp = HeightFogActor->GetComponent();
	if (!FogComp)
	{
		return;
	}

	const float TargetFog = SampleViewFogAmount();
	if (DeltaSeconds <= 0.f || TargetFog <= KINDA_SMALL_NUMBER)
	{
		// Snap clear so fog==0 never leaves residual mist while interpolating down.
		RenderedFogAmount = TargetFog;
	}
	else
	{
		RenderedFogAmount = FMath::FInterpTo(RenderedFogAmount, TargetFog, DeltaSeconds, FogInterpSpeed);
	}

	const float Amount = FMath::Clamp(RenderedFogAmount, 0.f, 1.f);

	float Density = 0.f;
	float MaxOpacity = 0.f;
	float StartDistance = 0.f;
	float ExtinctionScale = 0.f;

	// Piecewise mist response keyed to TerrainPoint fog levels:
	// fog==0 → perfect clear (true zeros, no residual haze)
	// fog==0.5 → hard to see through
	// fog==1 → essentially opaque
	if (Amount <= KINDA_SMALL_NUMBER)
	{
		Density = 0.f;
		MaxOpacity = 0.f;
		StartDistance = 0.f;
		ExtinctionScale = 0.f;
	}
	else if (Amount <= 0.5f)
	{
		const float T = Amount / 0.5f;
		Density = FMath::Lerp(0.f, FogDensityAtHalf, T);
		MaxOpacity = FMath::Lerp(0.f, FogMaxOpacityAtHalf, T);
		StartDistance = FMath::Lerp(2500.f, 60.f, T);
		ExtinctionScale = FMath::Lerp(0.f, 2.4f, T);
	}
	else
	{
		const float T = (Amount - 0.5f) / 0.5f;
		Density = FMath::Lerp(FogDensityAtHalf, FogDensityAtFull, T);
		MaxOpacity = FMath::Lerp(FogMaxOpacityAtHalf, FogMaxOpacityAtFull, T);
		StartDistance = FMath::Lerp(60.f, 0.f, T);
		ExtinctionScale = FMath::Lerp(2.4f, 4.5f, T);
	}

	FogComp->SetFogDensity(Density);
	FogComp->SetFogMaxOpacity(MaxOpacity);
	FogComp->SetFogInscatteringColor(FogMistColor);
	FogComp->SetStartDistance(StartDistance);
	FogComp->SetVolumetricFog(Amount > KINDA_SMALL_NUMBER);
	FogComp->VolumetricFogExtinctionScale = ExtinctionScale;
	FogComp->MarkRenderStateDirty();
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

UMaterialInterface* ASolidTerrainStreamer::FindFabGrassMaterial() const
{
	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/Fab")));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(UMaterialInstance::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	for (const FAssetData& Asset : Assets)
	{
		const FString Name = Asset.AssetName.ToString();
		if (!Name.Equals(TEXT("Mat_025_grass"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		// Skip the StaticMeshes package that shares the asset name.
		if (Asset.PackageName.ToString().Contains(TEXT("/StaticMeshes/"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		if (UMaterialInterface* Grass = Cast<UMaterialInterface>(Asset.GetAsset()))
		{
			UE_LOG(LogSolid, Warning,
				TEXT("Terrain material: Fab grass %s"), *Asset.GetObjectPathString());
			return Grass;
		}
	}

	UE_LOG(LogSolid, Warning, TEXT("Fab Mat_025_grass not found under /Game/Fab."));
	return nullptr;
}

UMaterialInterface* ASolidTerrainStreamer::CreateFlatColGrassMaterial() const
{
	ASolidTerrainStreamer* MutableThis = const_cast<ASolidTerrainStreamer*>(this);

	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (!Parent)
	{
		Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_DefaultColorway.MI_DefaultColorway"));
	}
	if (!Parent)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* GrassMID = UMaterialInstanceDynamic::Create(Parent, MutableThis);
	if (!GrassMID)
	{
		return Parent;
	}

	const FLinearColor MidGrass = FLinearColor::LerpUsingHSV(GrassDarkColor, GrassColor, 0.55f);
	GrassMID->SetVectorParameterValue(TEXT("Base Color"), MidGrass);
	GrassMID->SetVectorParameterValue(TEXT("BaseColor"), MidGrass);
	GrassMID->SetScalarParameterValue(TEXT("Roughness"), 0.9f);

	UE_LOG(LogSolid, Warning,
		TEXT("Terrain material: %s solid green (FlatCol fallback)"), *Parent->GetName());
	return GrassMID;
}

UMaterialInterface* ASolidTerrainStreamer::ResolveMaterial() const
{
	if (ResolvedTerrainMaterial)
	{
		return ResolvedTerrainMaterial;
	}

	ASolidTerrainStreamer* MutableThis = const_cast<ASolidTerrainStreamer*>(this);

	// Never use M_PrototypeGrid (hard-wired grey checker).

	// 1) Explicit override (skip PrototypeGrid if someone set it).
	if (TerrainMaterial)
	{
		const FString MatName = TerrainMaterial->GetName();
		if (!MatName.Contains(TEXT("PrototypeGrid"), ESearchCase::IgnoreCase))
		{
			MutableThis->ResolvedTerrainMaterial = TerrainMaterial;
			UE_LOG(LogSolid, Warning, TEXT("Terrain material: override %s"), *MatName);
			return ResolvedTerrainMaterial;
		}
		UE_LOG(LogSolid, Warning,
			TEXT("Ignoring TerrainMaterial '%s' (PrototypeGrid cannot be tinted)."), *MatName);
	}

	// 2) Fab seamless grass (Content/Fab/.../Mat_025_grass).
	if (UMaterialInterface* FabGrass = FindFabGrassMaterial())
	{
		MutableThis->ResolvedTerrainMaterial = FabGrass;
		return ResolvedTerrainMaterial;
	}

	// 3) FlatCol solid green fallback.
	if (UMaterialInterface* FlatGrass = CreateFlatColGrassMaterial())
	{
		MutableThis->ResolvedTerrainMaterial = FlatGrass;
		return ResolvedTerrainMaterial;
	}

	UE_LOG(LogSolid, Error, TEXT("Terrain material: no grass material could be created."));
	return nullptr;
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
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Disabled %d Landscape actor(s) so pawn uses procedural terrain."), Count);
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

	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Snapped focus onto terrain Z=%.1f at (%.0f, %.0f)"), LandZ, Loc.X, Loc.Y);
}

void ASolidTerrainStreamer::UpdateStreaming()
{
	EnsureTerrainMap();
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

		ASolidTerrainChunk* Chunk = World->SpawnActor<ASolidTerrainChunk>(
			ASolidTerrainChunk::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		if (!Chunk)
		{
			UE_LOG(LogSolid, Error, TEXT("Failed to spawn terrain chunk at %d,%d"), Coord.X, Coord.Y);
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
			Material,
			TerrainMap);

		LoadedChunks.Add(Coord, Chunk);
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Built terrain chunk (%d, %d) origin=(%.0f, %.0f) loaded=%d"),
			Coord.X, Coord.Y,
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			LoadedChunks.Num());
		UE_LOG(LogSolid, Warning, TEXT("Built terrain chunk (%d, %d) at origin (%.0f, %.0f). Loaded=%d"),
			Coord.X, Coord.Y,
			static_cast<float>(Coord.X) * ChunkWorldSize,
			static_cast<float>(Coord.Y) * ChunkWorldSize,
			LoadedChunks.Num());
	}

	TrySnapFocusToTerrain(Focus);
}
