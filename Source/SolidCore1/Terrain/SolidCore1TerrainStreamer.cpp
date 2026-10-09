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
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "MaterialEditingLibrary.h"
#endif

ASolidCore1TerrainStreamer::ASolidCore1TerrainStreamer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// FlatCol is the reliable lit solid-color fallback. Do NOT default to M_PrototypeGrid:
	// its grey T_GridChecker_A is hard-wired (no Texture parameter), so tint/noise binds stay grey.
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

UTexture2D* ASolidCore1TerrainStreamer::EnsureGrassNoiseTexture() const
{
	ASolidCore1TerrainStreamer* MutableThis = const_cast<ASolidCore1TerrainStreamer*>(this);
	if (MutableThis->GrassNoiseTexture)
	{
		return MutableThis->GrassNoiseTexture;
	}

	const int32 Size = FMath::Clamp(GrassNoiseTextureSize, 64, 1024);
	UTexture2D* NoiseTex = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!NoiseTex)
	{
		UE_LOG(LogSolidCore1, Error, TEXT("Failed to create grass noise texture."));
		return nullptr;
	}

	NoiseTex->SRGB = true;
	NoiseTex->Filter = TF_Bilinear;
	NoiseTex->AddressX = TA_Wrap;
	NoiseTex->AddressY = TA_Wrap;
	NoiseTex->CompressionSettings = TC_Default;
	NoiseTex->MipGenSettings = TMGS_NoMipmaps;
	NoiseTex->LODGroup = TEXTUREGROUP_World;
	NoiseTex->NeverStream = true;

	FTexturePlatformData* PlatformData = NoiseTex->GetPlatformData();
	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		UE_LOG(LogSolidCore1, Error, TEXT("Grass noise texture missing platform mip data."));
		return nullptr;
	}

	FTexture2DMipMap& Mip = PlatformData->Mips[0];
	void* RawMip = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (!RawMip)
	{
		UE_LOG(LogSolidCore1, Error, TEXT("Failed to lock grass noise texture mip."));
		return nullptr;
	}

	FColor* Pixels = static_cast<FColor*>(RawMip);
	// Match chunk UV scale (~0.0024): one texel ≈ world cm so speckles read at landscape scale.
	const float TexelWorldCm = 32.f;
	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			const float Tone = SolidCore1TerrainNoise::SampleGrassTone(
				static_cast<float>(X) * TexelWorldCm,
				static_cast<float>(Y) * TexelWorldCm,
				Seed + 9049);
			// Hard contrast so dark speckles read clearly on lit hills.
			const float Speckle = FMath::SmoothStep(0.35f, 0.65f, Tone);
			const FLinearColor Color = FLinearColor::LerpUsingHSV(GrassDarkColor, GrassColor, Speckle);
			Pixels[Y * Size + X] = Color.ToFColor(/*bSRGB=*/true);
		}
	}
	Mip.BulkData.Unlock();
	NoiseTex->UpdateResource();

	MutableThis->GrassNoiseTexture = NoiseTex;
	UE_LOG(LogSolidCore1, Warning, TEXT("Created grass noise texture %dx%d"), Size, Size);
	return NoiseTex;
}

UMaterialInterface* ASolidCore1TerrainStreamer::CreateVertexColorGrassMaterial() const
{
#if WITH_EDITOR
	ASolidCore1TerrainStreamer* MutableThis = const_cast<ASolidCore1TerrainStreamer*>(this);
	UMaterial* GrassMat = NewObject<UMaterial>(MutableThis, FName(TEXT("M_SC1_GrassVertex")), RF_Transient);
	if (!GrassMat)
	{
		return nullptr;
	}

	GrassMat->MaterialDomain = MD_Surface;
	GrassMat->BlendMode = BLEND_Opaque;
	GrassMat->SetShadingModel(MSM_DefaultLit);
	GrassMat->TwoSided = false;
	GrassMat->bUsedWithStaticMeshes = true;

	// Chunks already bake darker/lighter grass into vertex colors.
	UMaterialExpression* VertColorExp = UMaterialEditingLibrary::CreateMaterialExpression(
		GrassMat, UMaterialExpressionVertexColor::StaticClass(), -320, 0);
	if (!VertColorExp)
	{
		UE_LOG(LogSolidCore1, Error, TEXT("Failed to create VertexColor expression for grass."));
		return nullptr;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(GrassMat, VertColorExp, MP_BaseColor);

	UMaterialExpression* RoughExp = UMaterialEditingLibrary::CreateMaterialExpression(
		GrassMat, UMaterialExpressionConstant::StaticClass(), -320, 140);
	if (UMaterialExpressionConstant* Rough = Cast<UMaterialExpressionConstant>(RoughExp))
	{
		Rough->R = 0.9f;
		UMaterialEditingLibrary::ConnectMaterialProperty(GrassMat, Rough, MP_Roughness);
	}

	UMaterialEditingLibrary::RecompileMaterial(GrassMat);

	UE_LOG(LogSolidCore1, Warning,
		TEXT("Terrain material: M_SC1_GrassVertex (chunk vertex-color speckles)"));
	return GrassMat;
#else
	return nullptr;
#endif
}

UMaterialInterface* ASolidCore1TerrainStreamer::CreateGrassNoiseMaterial(UTexture2D* NoiseTex) const
{
#if WITH_EDITOR
	if (!NoiseTex)
	{
		return nullptr;
	}

	ASolidCore1TerrainStreamer* MutableThis = const_cast<ASolidCore1TerrainStreamer*>(this);
	UMaterial* GrassMat = NewObject<UMaterial>(MutableThis, FName(TEXT("M_SC1_GrassNoise")), RF_Transient);
	if (!GrassMat)
	{
		return nullptr;
	}

	GrassMat->MaterialDomain = MD_Surface;
	GrassMat->BlendMode = BLEND_Opaque;
	GrassMat->SetShadingModel(MSM_DefaultLit);
	GrassMat->TwoSided = false;
	GrassMat->bUsedWithStaticMeshes = true;

	UMaterialExpression* TexExp = UMaterialEditingLibrary::CreateMaterialExpression(
		GrassMat, UMaterialExpressionTextureSample::StaticClass(), -400, 0);
	UMaterialExpressionTextureSample* TexSample = Cast<UMaterialExpressionTextureSample>(TexExp);
	if (!TexSample)
	{
		UE_LOG(LogSolidCore1, Error, TEXT("Failed to create TextureSample expression for grass."));
		return nullptr;
	}
	TexSample->Texture = NoiseTex;
	TexSample->SamplerType = SAMPLERTYPE_Color;
	TexSample->ConstCoordinate = 0;
	UMaterialEditingLibrary::ConnectMaterialProperty(GrassMat, TexSample, MP_BaseColor);

	UMaterialExpression* RoughExp = UMaterialEditingLibrary::CreateMaterialExpression(
		GrassMat, UMaterialExpressionConstant::StaticClass(), -400, 160);
	if (UMaterialExpressionConstant* Rough = Cast<UMaterialExpressionConstant>(RoughExp))
	{
		Rough->R = 0.9f;
		UMaterialEditingLibrary::ConnectMaterialProperty(GrassMat, Rough, MP_Roughness);
	}

	UMaterialEditingLibrary::RecompileMaterial(GrassMat);

	UE_LOG(LogSolidCore1, Warning,
		TEXT("Terrain material: M_SC1_GrassNoise (green noise texture on mesh UVs)"));
	return GrassMat;
#else
	(void)NoiseTex;
	return nullptr;
#endif
}

UMaterialInterface* ASolidCore1TerrainStreamer::CreateFlatColGrassMaterial() const
{
	ASolidCore1TerrainStreamer* MutableThis = const_cast<ASolidCore1TerrainStreamer*>(this);

	// FlatCol solid green fallback. Never parent from PrototypeGrid (hard-wired grey checker).
	UMaterialInterface* Parent = nullptr;
	if (TerrainMaterial)
	{
		const FString MatName = TerrainMaterial->GetName();
		if (!MatName.Contains(TEXT("PrototypeGrid"), ESearchCase::IgnoreCase))
		{
			Parent = TerrainMaterial.Get();
		}
		else
		{
			UE_LOG(LogSolidCore1, Warning,
				TEXT("Ignoring TerrainMaterial '%s' (PrototypeGrid cannot be tinted green)."),
				*MatName);
		}
	}
	if (!Parent)
	{
		Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	}
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

	UE_LOG(LogSolidCore1, Warning,
		TEXT("Terrain material: %s solid green fallback (monotone)"), *Parent->GetName());
	return GrassMID;
}

UMaterialInterface* ASolidCore1TerrainStreamer::ResolveMaterial() const
{
	if (ResolvedTerrainMaterial)
	{
		return ResolvedTerrainMaterial;
	}

	ASolidCore1TerrainStreamer* MutableThis = const_cast<ASolidCore1TerrainStreamer*>(this);

	// Never use M_PrototypeGrid (hard-wired grey checker).
	//
	// TODO(shipping grass): Author /Game/SolidCore1/Materials/M_SC1_Grass (vertex color or
	// GrassNoise texture param) and load it here for packaged builds. Paths 1–2 are editor-only
	// (MaterialEditingLibrary / UnrealEd); packaged falls through to FlatCol monotone green.

	// 1) Vertex colors already hold darker/lighter grass per vert — show them.
	if (UMaterialInterface* VertGrass = CreateVertexColorGrassMaterial())
	{
		MutableThis->ResolvedTerrainMaterial = VertGrass;
		return ResolvedTerrainMaterial;
	}

	// 2) Runtime lit material sampling green noise (chunk UVs already world-tiled).
	if (UTexture2D* NoiseTex = EnsureGrassNoiseTexture())
	{
		if (UMaterialInterface* GrassMat = CreateGrassNoiseMaterial(NoiseTex))
		{
			MutableThis->ResolvedTerrainMaterial = GrassMat;
			return ResolvedTerrainMaterial;
		}
	}

	// 3) FlatCol Base Color — solid green last resort (also the packaged-build path today).
	if (UMaterialInterface* FlatGrass = CreateFlatColGrassMaterial())
	{
		MutableThis->ResolvedTerrainMaterial = FlatGrass;
		return ResolvedTerrainMaterial;
	}

	UE_LOG(LogSolidCore1, Error, TEXT("Terrain material: no grass material could be created."));
	return nullptr;
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
