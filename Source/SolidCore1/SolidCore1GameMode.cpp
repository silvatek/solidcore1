#include "SolidCore1GameMode.h"
#include "SolidCore1BuildId.h"
#include "SolidCore1Character.h"
#include "SolidCore1HUD.h"
#include "SolidCore1PlayerController.h"
#include "SolidCore1.h"
#include "Companion/SolidCore1CompanionCharacter.h"
#include "Terrain/SolidCore1TerrainStreamer.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace SolidCore1GameModePrivate
{
	static UClass* TryLoadPawnClass(const TCHAR* Path)
	{
		if (UClass* Loaded = LoadClass<APawn>(nullptr, Path))
		{
			UE_LOG(LogSolidCore1, Warning, TEXT("SolidCore1GameMode using pawn class: %s"), *Loaded->GetPathName());
			return Loaded;
		}
		return nullptr;
	}
}

ASolidCore1GameMode::ASolidCore1GameMode()
{
	PlayerControllerClass = ASolidCore1PlayerController::StaticClass();
	HUDClass = ASolidCore1HUD::StaticClass();
	bAutoSpawnTerrainStreamer = true;
	bAutoSpawnCompanion = true;
	CompanionClass = ASolidCore1CompanionCharacter::StaticClass();

	UClass* PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
		TEXT("/Game/Characters/BP_SolidCore1Character.BP_SolidCore1Character_C"));
	if (!PawnClass)
	{
		PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/Blueprints/BP_SolidCore1Character.BP_SolidCore1Character_C"));
	}
	if (!PawnClass)
	{
		PawnClass = SolidCore1GameModePrivate::TryLoadPawnClass(
			TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C"));
	}

	if (PawnClass)
	{
		DefaultPawnClass = PawnClass;
	}
	else
	{
		DefaultPawnClass = ASolidCore1Character::StaticClass();
		UE_LOG(LogSolidCore1, Warning,
			TEXT("SolidCore1GameMode falling back to C++ SolidCore1Character. "
				 "Create /Game/Characters/BP_SolidCore1Character with SKM_Manny_Simple assigned, "
				 "or set Default Pawn via BP_SolidCore1GameMode in Project Settings."));
	}
}

void ASolidCore1GameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Build %s"), SOLIDCORE1_BUILD_ID);
	UE_LOG(LogSolidCore1, Warning, TEXT("Build %s"), SOLIDCORE1_BUILD_ID);
	EnsureTerrainStreamer();

	// Player pawn may not exist on the first frame of PIE — retry shortly.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CompanionSpawnTimer, this, &ASolidCore1GameMode::EnsureCompanion, 0.35f, false);
	}
	EnsureCompanion();
}

void ASolidCore1GameMode::EnsureTerrainStreamer()
{
	if (!bAutoSpawnTerrainStreamer)
	{
		return;
	}

	ASolidCore1TerrainStreamer::EnsureExists(GetWorld());
}

void ASolidCore1GameMode::EnsureCompanion()
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
			CompanionSpawnTimer, this, &ASolidCore1GameMode::EnsureCompanion, 0.25f, false);
		return;
	}

	UClass* ClassToSpawn = CompanionClass
		? CompanionClass.Get()
		: ASolidCore1CompanionCharacter::StaticClass();

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

	if (ASolidCore1TerrainStreamer* Streamer = ASolidCore1TerrainStreamer::EnsureExists(World))
	{
		const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
		SpawnLoc.Z = LandZ + CapsuleHalf + Streamer->SnapHeightPadding;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASolidCore1CompanionCharacter* Companion = World->SpawnActor<ASolidCore1CompanionCharacter>(
		ClassToSpawn, SpawnLoc, PlayerPawn->GetActorRotation(), SpawnParams);
	if (!Companion)
	{
		UE_LOG(LogTemp, Error, TEXT("[SolidCore1] Failed to spawn Quinn companion."));
		UE_LOG(LogSolidCore1, Error, TEXT("Failed to spawn Quinn companion."));
		return;
	}

	Companion->SetFollowTarget(PlayerPawn);
	SpawnedCompanion = Companion;

	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Spawned Quinn companion at %s following %s"),
		*SpawnLoc.ToCompactString(), *PlayerPawn->GetName());
	UE_LOG(LogSolidCore1, Warning, TEXT("Spawned Quinn companion following %s"), *PlayerPawn->GetName());
}
