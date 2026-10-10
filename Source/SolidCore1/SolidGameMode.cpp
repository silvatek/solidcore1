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
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Vegetation/SolidBuilding.h"
#include "Vegetation/SolidForestTrees.h"
#include "Vegetation/SolidTownBuildings.h"
#include "Vegetation/SolidTownSign.h"
#include "Vegetation/SolidTownSigns.h"
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
			VegetationSpawnTimer, this, &ASolidGameMode::EnsureVegetation, 0.5f, false);
	}
	EnsureCompanion();
	EnsureVegetation();
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

	// Wait for the streamer to finish the one-shot location-0 relocate so companions
	// spawn next to the Captain at the starting town, not at the map PlayerStart.
	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::EnsureExists(World);
	if (Streamer && !Streamer->HasAttemptedStartTownRelocate())
	{
		World->GetTimerManager().SetTimer(
			CompanionSpawnTimer, this, &ASolidGameMode::EnsureCompanion, 0.1f, false);
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

void ASolidGameMode::EnsureVegetation()
{
	if (!bAutoSpawnVegetation)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SpawnedForestTrees.RemoveAll([](const TObjectPtr<ASolidTree>& Tree)
	{
		return !IsValid(Tree);
	});
	SpawnedTownBuildings.RemoveAll([](const TObjectPtr<ASolidBuilding>& Building)
	{
		return !IsValid(Building);
	});
	SpawnedTownSigns.RemoveAll([](const TObjectPtr<ASolidTownSign>& Sign)
	{
		return !IsValid(Sign);
	});

	if (SpawnedForestTrees.Num() == 0)
	{
		for (TActorIterator<ASolidTree> It(World); It; ++It)
		{
			SpawnedForestTrees.Add(*It);
		}
	}

	if (SpawnedTownBuildings.Num() == 0)
	{
		for (TActorIterator<ASolidBuilding> It(World); It; ++It)
		{
			SpawnedTownBuildings.Add(*It);
		}
	}

	if (SpawnedTownSigns.Num() == 0)
	{
		for (TActorIterator<ASolidTownSign> It(World); It; ++It)
		{
			SpawnedTownSigns.Add(*It);
		}
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::EnsureExists(World);
	if (!Streamer || !Streamer->GetTerrainMap() || !Streamer->GetTerrainMap()->IsBuilt())
	{
		World->GetTimerManager().SetTimer(
			VegetationSpawnTimer, this, &ASolidGameMode::EnsureVegetation, 0.25f, false);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (SpawnedForestTrees.Num() == 0)
	{
		SolidForestTrees::FScatterParams Scatter;
		Scatter.Density = ForestTreeDensity;
		Scatter.MaxTrees = MaxForestTrees;
		Scatter.JitterCm = ForestTreeJitterCm;

		TArray<FVector2D> Positions;
		SolidForestTrees::CollectSpawnPositions(
			Streamer->GetTerrainMap(), ForestTreeSeed, Scatter, Positions);

		FRandomStream Rng(ForestTreeSeed != 0 ? ForestTreeSeed : 1337);
		int32 Spawned = 0;
		for (const FVector2D& XY : Positions)
		{
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
			SpawnedForestTrees.Add(Tree);
			++Spawned;
		}

		UE_LOG(LogSolid, Warning,
			TEXT("Spawned %d forest trees (density=%.2f max=%d)"),
			Spawned, ForestTreeDensity, MaxForestTrees);
	}

	TArray<SolidTownSigns::FPlacement> SignPlacements;
	TArray<FVector2D> SignClearCenters;
	if (const USolidWorldMap* WorldMap = Streamer->GetTerrainMap()->GetWorldMap())
	{
		SolidTownSigns::CollectPlacements(
			WorldMap,
			Streamer->GetTerrainMap()->GetWorldMinXY(),
			Streamer->GetTerrainMap()->GetWorldMaxXY(),
			SignPlacements);
		SignClearCenters.Reserve(SignPlacements.Num());
		for (const SolidTownSigns::FPlacement& Placement : SignPlacements)
		{
			SignClearCenters.Add(Placement.WorldXY);
		}
	}

	if (SpawnedTownBuildings.Num() == 0)
	{
		SolidTownBuildings::FScatterParams BuildingParams;
		BuildingParams.Density = TownBuildingDensity;
		BuildingParams.MaxBuildings = MaxTownBuildings;
		BuildingParams.MinSeparationCm = TownBuildingMinSeparationCm;
		BuildingParams.ClearRadiusAroundTownCenterCm = TownBuildingClearRadiusCm;

		TArray<SolidTownBuildings::FPlacement> Placements;
		SolidTownBuildings::CollectPlacements(
			Streamer->GetTerrainMap(), TownBuildingSeed, BuildingParams, Placements, SignClearCenters);

		int32 Spawned = 0;
		for (const SolidTownBuildings::FPlacement& Placement : Placements)
		{
			FVector SpawnLoc(Placement.CenterXY.X, Placement.CenterXY.Y, 0.f);
			const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
			SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

			ASolidBuilding* Building = World->SpawnActor<ASolidBuilding>(
				ASolidBuilding::StaticClass(),
				SpawnLoc,
				FRotator(0.f, Placement.YawDeg, 0.f),
				SpawnParams);
			if (!Building)
			{
				continue;
			}

			Building->FootprintXCm = Placement.FootprintXCm;
			Building->FootprintYCm = Placement.FootprintYCm;
			Building->BodyHeightCm = Placement.BodyHeightCm;
			Building->RoofHeightCm = Placement.RoofHeightCm;
			Building->BuildVisuals();
			SpawnedTownBuildings.Add(Building);
			++Spawned;
		}

		UE_LOG(LogSolid, Warning,
			TEXT("Spawned %d town buildings (density=%.2f max=%d)"),
			Spawned, TownBuildingDensity, MaxTownBuildings);
	}

	if (SpawnedTownSigns.Num() == 0)
	{
		int32 Spawned = 0;
		for (const SolidTownSigns::FPlacement& Placement : SignPlacements)
		{
			FVector SpawnLoc(Placement.WorldXY.X, Placement.WorldXY.Y, 0.f);
			const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
			SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

			ASolidTownSign* Sign = World->SpawnActor<ASolidTownSign>(
				ASolidTownSign::StaticClass(),
				SpawnLoc,
				FRotator(0.f, SolidTownSigns::FacingYawDeg, 0.f),
				SpawnParams);
			if (!Sign)
			{
				continue;
			}

			Sign->SetTownName(Placement.Name);
			Sign->BuildVisuals();
			SpawnedTownSigns.Add(Sign);
			++Spawned;
		}

		UE_LOG(LogSolid, Warning, TEXT("Spawned %d town signs"), Spawned);
	}
}
