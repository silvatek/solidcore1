#include "SolidGameMode.h"
#include "SolidBuildId.h"
#include "SolidCharacter.h"
#include "SolidContentPaths.h"
#include "SolidHUD.h"
#include "SolidPlayerController.h"
#include "SolidCore1.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Party/SolidBattlePlan.h"
#include "Party/SolidCompany.h"
#include "Party/SolidParty.h"
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

ASolidGameMode::ASolidGameMode()
{
	PlayerControllerClass = ASolidPlayerController::StaticClass();
	HUDClass = ASolidHUD::StaticClass();
	bAutoSpawnTerrainStreamer = true;
	bAutoSpawnCompanion = true;
	CompanionClass = ASolidCompanionCharacter::StaticClass();

	if (UClass* PawnClass = SolidContentPaths::LoadFirstClass<APawn>(SolidContentPaths::PawnBlueprintClasses()))
	{
		DefaultPawnClass = PawnClass;
		UE_LOG(LogSolid, Warning, TEXT("SolidGameMode using pawn class: %s"), *PawnClass->GetPathName());
	}
	else
	{
		DefaultPawnClass = ASolidCharacter::StaticClass();
		UE_LOG(LogSolid, Warning,
			TEXT("SolidGameMode falling back to C++ SolidCharacter. "
				 "Expected /Game/Characters/BP_SolidCharacter (parent SolidCharacter)."));
	}

	EnsureCompanyAndParty();
}

void ASolidGameMode::EnsureCompanyAndParty()
{
	if (!Company)
	{
		Company = NewObject<USolidCompany>(this, TEXT("Company"));
	}
	Company->InitializeDefaultBattlePlans();

	if (!Party)
	{
		Party = NewObject<USolidParty>(this, TEXT("Party"));
	}
	Party->InitializeFromCompany(Company);
}

void ASolidGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogSolid, Warning, TEXT("Build %s"), SOLID_BUILD_ID);
	EnsureCompanyAndParty();
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

bool ASolidGameMode::SelectBattlePlanSlot(const int32 SlotIndex)
{
	EnsureCompanyAndParty();
	if (!Party || !Party->SelectAssignedSlot(SlotIndex))
	{
		return false;
	}

	UE_LOG(LogSolid, Warning, TEXT("%s"), *Party->GetActiveBattlePlanDebugString());
	return true;
}

void ASolidGameMode::EnsureTerrainStreamer()
{
	if (!bAutoSpawnTerrainStreamer)
	{
		return;
	}

	ASolidTerrainStreamer::EnsureExists(GetWorld());
}

ASolidCompanionCharacter* ASolidGameMode::GetCompanion() const
{
	return SpawnedCompanions.Num() > 0 ? SpawnedCompanions[0].Get() : nullptr;
}

ASolidCompanionCharacter* ASolidGameMode::SpawnCompanion(
	UWorld* World,
	APawn* PlayerPawn,
	UClass* ClassToSpawn,
	const FString& DisplayName,
	const int32 PartySlotIndex)
{
	if (!World || !PlayerPawn || !ClassToSpawn)
	{
		return nullptr;
	}

	EnsureCompanyAndParty();
	const ESolidBattleFormation Formation = Party
		? Party->GetActiveFormation()
		: ESolidBattleFormation::Line;
	const ESolidBattleSpacing Spacing = Party
		? Party->GetActiveSpacing()
		: ESolidBattleSpacing::Standard;
	const FVector2D Slot = SolidBattleFormationSlots::SlotOffset(
		Formation, PartySlotIndex, DefaultCompanionCount, Spacing);

	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const FVector PlayerFwd = PlayerPawn->GetActorForwardVector();
	const FVector PlayerRight = PlayerPawn->GetActorRightVector();
	FVector SpawnLoc = PlayerLoc + PlayerFwd * Slot.X + PlayerRight * Slot.Y;

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
		UE_LOG(LogSolid, Error, TEXT("Failed to spawn companion %s."), *DisplayName);
		return nullptr;
	}

	const float Behind = FMath::Abs(Slot.X);
	Companion->FollowDistance = FMath::Max(Behind, 120.f);
	Companion->SideOffset = Slot.Y;
	Companion->CatchUpDistance = FMath::Max(700.f, Behind + 400.f);
	Companion->SetPartySlotIndex(PartySlotIndex);
	Companion->SetCharacterDisplayName(DisplayName);
	Companion->SetFollowTarget(PlayerPawn);

	UE_LOG(LogSolid, Warning,
		TEXT("Spawned companion %s slot=%d at %s (fwd=%.0f right=%.0f) following %s"),
		*DisplayName, PartySlotIndex, *SpawnLoc.ToCompactString(), Slot.X, Slot.Y, *PlayerPawn->GetName());
	return Companion;
}

void ASolidGameMode::EnsureCompanion()
{
	if (!bAutoSpawnCompanion)
	{
		return;
	}

	EnsureCompanyAndParty();

	// Drop stale entries (PIE teardown / destroyed actors).
	SpawnedCompanions.RemoveAll([](const TObjectPtr<ASolidCompanionCharacter>& Companion)
	{
		return !IsValid(Companion);
	});

	if (SpawnedCompanions.Num() >= DefaultCompanionCount)
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

	auto HasNamed = [this](const TCHAR* Name) -> bool
	{
		for (const TObjectPtr<ASolidCompanionCharacter>& Companion : SpawnedCompanions)
		{
			if (Companion && Companion->GetCharacterDisplayName() == Name)
			{
				return true;
			}
		}
		return false;
	};

	// Slot 0 = Sam, slot 1 = Alex (formation offsets come from the active Battle Plan).
	if (!HasNamed(TEXT("Sam")))
	{
		if (ASolidCompanionCharacter* Sam = SpawnCompanion(
			World, PlayerPawn, ClassToSpawn, TEXT("Sam"), /*PartySlotIndex=*/0))
		{
			SpawnedCompanions.Add(Sam);
		}
	}

	if (!HasNamed(TEXT("Alex")))
	{
		if (ASolidCompanionCharacter* Alex = SpawnCompanion(
			World, PlayerPawn, ClassToSpawn, TEXT("Alex"), /*PartySlotIndex=*/1))
		{
			SpawnedCompanions.Add(Alex);
		}
	}

	if (SpawnedCompanions.Num() < DefaultCompanionCount)
	{
		World->GetTimerManager().SetTimer(
			CompanionSpawnTimer, this, &ASolidGameMode::EnsureCompanion, 0.25f, false);
	}
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
