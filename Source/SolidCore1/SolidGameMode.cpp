#include "SolidGameMode.h"
#include "SolidBuildId.h"
#include "SolidCharacter.h"
#include "SolidHUD.h"
#include "SolidPlayerController.h"
#include "SolidCore1.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Vegetation/SolidMonolith.h"
#include "Vegetation/SolidTree.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace SolidGameModePrivate
{
	static UClass* TryLoadPawnClass(const TCHAR* Path)
	{
		if (UClass* Loaded = LoadClass<APawn>(nullptr, Path))
		{
			UE_LOG(LogSolid, Warning, TEXT("SolidGameMode using pawn class: %s"), *Loaded->GetPathName());
			return Loaded;
		}
		return nullptr;
	}
}

ASolidGameMode::ASolidGameMode()
{
	PlayerControllerClass = ASolidPlayerController::StaticClass();
	HUDClass = ASolidHUD::StaticClass();
	bAutoSpawnTerrainStreamer = true;
	bAutoSpawnCompanion = true;
	CompanionClass = ASolidCompanionCharacter::StaticClass();

	// Prefer Solid* BP names; keep legacy BP_SolidCore1* as fallback.
	UClass* PawnClass = SolidGameModePrivate::TryLoadPawnClass(
		TEXT("/Game/Characters/BP_SolidCharacter.BP_SolidCharacter_C"));
	if (!PawnClass)
	{
		PawnClass = SolidGameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Characters/BP_SolidCore1Character.BP_SolidCore1Character_C"));
	}
	if (!PawnClass)
	{
		PawnClass = SolidGameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Blueprints/BP_SolidCharacter.BP_SolidCharacter_C"));
	}
	if (!PawnClass)
	{
		PawnClass = SolidGameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Blueprints/BP_SolidCore1Character.BP_SolidCore1Character_C"));
	}
	if (!PawnClass)
	{
		PawnClass = SolidGameModePrivate::TryLoadPawnClass(
			TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C"));
	}

	if (PawnClass)
	{
		DefaultPawnClass = PawnClass;
	}
	else
	{
		DefaultPawnClass = ASolidCharacter::StaticClass();
		UE_LOG(LogSolid, Warning,
			TEXT("SolidGameMode falling back to C++ SolidCharacter. "
				 "Create /Game/Characters/BP_SolidCharacter (parent SolidCharacter), "
				 "or set Default Pawn via BP_SolidGameMode in Project Settings."));
	}
}

void ASolidGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Build %s"), SOLID_BUILD_ID);
	UE_LOG(LogSolid, Warning, TEXT("Build %s"), SOLID_BUILD_ID);
	EnsureTerrainStreamer();

	// Player pawn / terrain may not be ready on the first PIE frame — retry shortly.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CompanionSpawnTimer, this, &ASolidGameMode::EnsureCompanion, 0.35f, false);
		World->GetTimerManager().SetTimer(
			StarterTreeSpawnTimer, this, &ASolidGameMode::EnsureStarterTrees, 0.5f, false);
	}
	EnsureCompanion();
	EnsureStarterTrees();
}

void ASolidGameMode::EnsureTerrainStreamer()
{
	if (!bAutoSpawnTerrainStreamer)
	{
		return;
	}

	ASolidTerrainStreamer::EnsureExists(GetWorld());
}

void ASolidGameMode::EnsureCompanion()
{
	if (!bAutoSpawnCompanion)
	{
		return;
	}

	if (SpawnedCompanion.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		// Keep trying until the pawn exists.
		World->GetTimerManager().SetTimer(
			CompanionSpawnTimer, this, &ASolidGameMode::EnsureCompanion, 0.25f, false);
		return;
	}

	UClass* ClassToSpawn = CompanionClass
		? CompanionClass.Get()
		: ASolidCompanionCharacter::StaticClass();

	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const FVector PlayerFwd = PlayerPawn->GetActorForwardVector();
	const FVector PlayerRight = PlayerPawn->GetActorRightVector();
	FVector SpawnLoc = PlayerLoc - PlayerFwd * 280.f + PlayerRight * 80.f;

	float CapsuleHalf = 96.f;
	if (const ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn))
	{
		if (const UCapsuleComponent* Capsule = PlayerCharacter->GetCapsuleComponent())
		{
			CapsuleHalf = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	if (ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::EnsureExists(World))
	{
		const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
		SpawnLoc.Z = LandZ + CapsuleHalf + Streamer->SnapHeightPadding;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASolidCompanionCharacter* Companion = World->SpawnActor<ASolidCompanionCharacter>(
		ClassToSpawn, SpawnLoc, PlayerPawn->GetActorRotation(), SpawnParams);
	if (!Companion)
	{
		UE_LOG(LogTemp, Error, TEXT("[SolidCore1] Failed to spawn companion."));
		UE_LOG(LogSolid, Error, TEXT("Failed to spawn companion."));
		return;
	}

	Companion->SetFollowTarget(PlayerPawn);
	SpawnedCompanion = Companion;

	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Spawned companion at %s following %s"),
		*SpawnLoc.ToCompactString(), *PlayerPawn->GetName());
	UE_LOG(LogSolid, Warning, TEXT("Spawned companion following %s"), *PlayerPawn->GetName());
}

void ASolidGameMode::EnsureStarterTrees()
{
	if (!bAutoSpawnStarterTrees)
	{
		return;
	}

	if (SpawnedMonolith.IsValid() || SpawnedStarterTrees.Num() > 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Already placed in the level (PIE restart / hand-placed).
	for (TActorIterator<ASolidMonolith> It(World); It; ++It)
	{
		SpawnedMonolith = *It;
	}
	for (TActorIterator<ASolidTree> It(World); It; ++It)
	{
		SpawnedStarterTrees.Add(*It);
	}
	if (SpawnedMonolith.IsValid() || SpawnedStarterTrees.Num() > 0)
	{
		return;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::EnsureExists(World);
	if (!Streamer || !Streamer->GetTerrainMap())
	{
		World->GetTimerManager().SetTimer(
			StarterTreeSpawnTimer, this, &ASolidGameMode::EnsureStarterTrees, 0.25f, false);
		return;
	}

	FVector2D Dir = StarterTreeLineDirection;
	if (!Dir.Normalize())
	{
		Dir = FVector2D(1.f, 0.f);
	}
	const FVector2D Side(-Dir.Y, Dir.X);

	FRandomStream Rng(StarterTreeSeed != 0 ? StarterTreeSeed : 1337);
	const int32 Count = FMath::Clamp(StarterTreeCount, 1, 64);
	const float Spacing = FMath::Max(StarterTreeSpacingCm, 200.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Monolith at the former first-tree spot (start of the clear-zone landmark).
	{
		FVector MonoLoc(StarterTreeOffsetXY.X, StarterTreeOffsetXY.Y, 0.f);
		const float LandZ = Streamer->GetHeightAt(MonoLoc) + Streamer->CollisionHeightBias;
		MonoLoc.Z = LandZ + Streamer->SnapHeightPadding;

		ASolidMonolith* Monolith = World->SpawnActor<ASolidMonolith>(
			ASolidMonolith::StaticClass(), MonoLoc, FRotator(0.f, 25.f, 0.f), SpawnParams);
		if (Monolith)
		{
			Monolith->BuildVisuals();
			SpawnedMonolith = Monolith;
			UE_LOG(LogSolid, Warning, TEXT("Spawned starter monolith at %s"), *MonoLoc.ToCompactString());
		}
		else
		{
			UE_LOG(LogSolid, Error, TEXT("Failed to spawn starter SolidMonolith."));
		}
	}

	// Trees begin one spacing past the monolith and stretch into the fog.
	int32 Spawned = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Along =
			Spacing * (1.f + static_cast<float>(Index)) + Rng.FRandRange(-Spacing * 0.15f, Spacing * 0.15f);
		const float Lateral = Rng.FRandRange(-220.f, 220.f);
		const FVector2D XY = StarterTreeOffsetXY + Dir * Along + Side * Lateral;

		FVector SpawnLoc(XY.X, XY.Y, 0.f);
		const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
		SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

		const float Yaw = Rng.FRandRange(0.f, 360.f);
		ASolidTree* Tree = World->SpawnActor<ASolidTree>(
			ASolidTree::StaticClass(), SpawnLoc, FRotator(0.f, Yaw, 0.f), SpawnParams);
		if (!Tree)
		{
			continue;
		}

		Tree->ApplyRandomVariation(Rng);
		Tree->BuildVisuals();
		SpawnedStarterTrees.Add(Tree);
		++Spawned;
	}

	UE_LOG(LogSolid, Warning,
		TEXT("Spawned monolith + %d trees along dir=(%.2f,%.2f) spacing=%.0fcm"),
		Spawned, Dir.X, Dir.Y, Spacing);
}
