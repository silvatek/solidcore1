#include "SolidGameMode.h"
#include "SolidBuildId.h"
#include "SolidCharacter.h"
#include "SolidHUD.h"
#include "SolidPlayerController.h"
#include "SolidCore1.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
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

	UClass* PawnClass = SolidGameModePrivate::TryLoadPawnClass(
		TEXT("/Game/Characters/BP_SolidCore1Character.BP_SolidCore1Character_C"));
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
				 "Create /Game/Characters/BP_SolidCore1Character with SKM_Manny_Simple assigned, "
				 "or set Default Pawn via BP_SolidCore1GameMode in Project Settings."));
	}
}

void ASolidGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Build %s"), SOLID_BUILD_ID);
	UE_LOG(LogSolid, Warning, TEXT("Build %s"), SOLID_BUILD_ID);
	EnsureTerrainStreamer();

	// Player pawn may not exist on the first frame of PIE — retry shortly.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CompanionSpawnTimer, this, &ASolidGameMode::EnsureCompanion, 0.35f, false);
	}
	EnsureCompanion();
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
		UE_LOG(LogTemp, Error, TEXT("[SolidCore1] Failed to spawn Quinn companion."));
		UE_LOG(LogSolid, Error, TEXT("Failed to spawn Quinn companion."));
		return;
	}

	Companion->SetFollowTarget(PlayerPawn);
	SpawnedCompanion = Companion;

	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Spawned Quinn companion at %s following %s"),
		*SpawnLoc.ToCompactString(), *PlayerPawn->GetName());
	UE_LOG(LogSolid, Warning, TEXT("Spawned Quinn companion following %s"), *PlayerPawn->GetName());
}
