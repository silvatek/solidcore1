#include "SolidTerrainStreamer.h"
#include "SolidTerrainChunk.h"
#include "SolidTerrainFog.h"
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
#include "Components/CapsuleComponent.h"
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

UMaterialInterface* ASolidTerrainStreamer::CreateSolidColorMaterial(
	const FLinearColor& Color,
	const TCHAR* DebugName) const
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
		UE_LOG(LogSolid, Error, TEXT("CreateSolidColorMaterial(%s): no FlatCol parent."), DebugName);
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, MutableThis);
	if (!MID)
	{
		return Parent;
	}

	MID->SetVectorParameterValue(TEXT("Base Color"), Color);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
	MID->SetScalarParameterValue(TEXT("Roughness"), 1.f);
	UE_LOG(LogSolid, Warning, TEXT("Created solid material %s from %s"), DebugName, *Parent->GetName());
	return MID;
}

UMaterialInterface* ASolidTerrainStreamer::CreateFlatColGrassMaterial() const
{
	const FLinearColor MidGrass = FLinearColor::LerpUsingHSV(GrassDarkColor, GrassColor, 0.55f);
	UMaterialInterface* GrassMID = CreateSolidColorMaterial(MidGrass, TEXT("FlatColGrass"));
	if (GrassMID)
	{
		UE_LOG(LogSolid, Warning, TEXT("Terrain material: solid green (FlatCol fallback)"));
	}
	return GrassMID;
}

UMaterialInterface* ASolidTerrainStreamer::MakeMatteGrassInstance(UMaterialInterface* Parent) const
{
	if (!Parent)
	{
		return nullptr;
	}

	ASolidTerrainStreamer* MutableThis = const_cast<ASolidTerrainStreamer*>(this);
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, MutableThis);
	if (!MID)
	{
		return Parent;
	}

	const float Roughness = FMath::Clamp(GrassRoughness, 0.f, 1.f);
	const float Specular = FMath::Clamp(GrassSpecular, 0.f, 1.f);

	// Blanket sets for common Fab / Quixel / FlatCol names.
	static const TCHAR* RoughnessNames[] = {
		TEXT("Roughness"), TEXT("roughness"), TEXT("RoughnessAmount"), TEXT("RoughnessIntensity"),
		TEXT("Roughness Min"), TEXT("RoughnessMax"), TEXT("Roughness Max"), TEXT("RoughnessMultiply"),
		TEXT("Roughness Scale"), TEXT("RoughnessScale"), TEXT("ORM Roughness"),
	};
	static const TCHAR* SpecularNames[] = {
		TEXT("Specular"), TEXT("specular"), TEXT("SpecularAmount"), TEXT("SpecularIntensity"),
		TEXT("Spec"), TEXT("SpecularScale"),
	};
	static const TCHAR* MetallicNames[] = {
		TEXT("Metallic"), TEXT("metallic"), TEXT("MetallicAmount"), TEXT("Metalness"),
	};

	for (const TCHAR* Name : RoughnessNames)
	{
		MID->SetScalarParameterValue(Name, Roughness);
	}
	for (const TCHAR* Name : SpecularNames)
	{
		MID->SetScalarParameterValue(Name, Specular);
	}
	for (const TCHAR* Name : MetallicNames)
	{
		MID->SetScalarParameterValue(Name, 0.f);
	}

	// Also drive any scalar the parent actually exposes whose name looks relevant.
	TArray<FMaterialParameterInfo> ScalarInfos;
	TArray<FGuid> ScalarIds;
	Parent->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);
	for (const FMaterialParameterInfo& Info : ScalarInfos)
	{
		const FString Name = Info.Name.ToString();
		if (Name.Contains(TEXT("Rough"), ESearchCase::IgnoreCase))
		{
			MID->SetScalarParameterValue(Info.Name, Roughness);
		}
		else if (Name.Contains(TEXT("Spec"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Gloss"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Shine"), ESearchCase::IgnoreCase))
		{
			// Gloss/shine often inverted vs roughness — keep low for less shine.
			const bool bLooksLikeGloss = Name.Contains(TEXT("Gloss"), ESearchCase::IgnoreCase)
				|| Name.Contains(TEXT("Shine"), ESearchCase::IgnoreCase);
			MID->SetScalarParameterValue(Info.Name, bLooksLikeGloss ? (1.f - Roughness) : Specular);
		}
		else if (Name.Contains(TEXT("Metal"), ESearchCase::IgnoreCase))
		{
			MID->SetScalarParameterValue(Info.Name, 0.f);
		}
	}

	UE_LOG(LogSolid, Warning,
		TEXT("Terrain material: matte MID on %s (roughness=%.2f specular=%.2f, %d scalar params scanned)"),
		*Parent->GetName(), Roughness, Specular, ScalarInfos.Num());
	return MID;
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
			MutableThis->ResolvedTerrainMaterial = MakeMatteGrassInstance(TerrainMaterial);
			UE_LOG(LogSolid, Warning, TEXT("Terrain material: override %s (matte)"), *MatName);
			return ResolvedTerrainMaterial;
		}
		UE_LOG(LogSolid, Warning,
			TEXT("Ignoring TerrainMaterial '%s' (PrototypeGrid cannot be tinted)."), *MatName);
	}

	// 2) Fab seamless grass (Content/Fab/.../Mat_025_grass), forced matte.
	if (UMaterialInterface* FabGrass = FindFabGrassMaterial())
	{
		MutableThis->ResolvedTerrainMaterial = MakeMatteGrassInstance(FabGrass);
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
