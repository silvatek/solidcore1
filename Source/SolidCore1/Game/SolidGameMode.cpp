#include "SolidGameMode.h"
#include "SolidBuildId.h"
#include "SolidCharacter.h"
#include "SolidContentLookup.h"
#include "SolidContentPaths.h"
#include "UObject/SoftObjectPath.h"
#include "HUD/SolidHUD.h"
#include "SolidPlayerController.h"
#include "SolidCore1.h"
#include "Companion/SolidCompanionCharacter.h"
#include "SolidBodyMesh.h"
#include "SolidBodyParams.h"
#include "Party/SolidBattlePlan.h"
#include "Party/SolidCompany.h"
#include "Party/SolidParty.h"
#include "Terrain/SolidBridge.h"
#include "SolidGroundMove.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Towns/SolidTowns.h"
#include "Menus/SolidPortal.h"
#include "Towns/SolidBuilding.h"
#include "Towns/SolidWallgate.h"
#include "Towns/SolidTownBuildings.h"
#include "Towns/SolidTownSign.h"
#include "Towns/SolidTownSigns.h"
#include "Lairs/SolidLair.h"
#include "Lairs/SolidLairInterior.h"
#include "Strongholds/SolidDungeonEntrance.h"
#include "Strongholds/SolidDungeonInterior.h"
#include "Strongholds/SolidDungeonLevel.h"
#include "Strongholds/SolidStrongholds.h"
#include "Strongholds/SolidTowerInterior.h"
#include "Strongholds/SolidStrongholdStone.h"
#include "Strongholds/SolidStrongholdTransition.h"
#include "Strongholds/SolidTowerShell.h"
#include "Strongholds/SolidTowerStronghold.h"
#include "SolidGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Vegetation/SolidForage.h"
#include "Vegetation/SolidForagePile.h"
#include "Vegetation/SolidForestTrees.h"
#include "Vegetation/SolidTree.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	TSoftObjectPtr<USkeletalMesh> ResolveFieldedMesh(USolidCompany* Company, const FSolidCompanion& Companion)
	{
		TSoftObjectPtr<USkeletalMesh> Mesh = Company
			? Company->GetMeshForCompanion(Companion)
			: TSoftObjectPtr<USkeletalMesh>();
		const FSolidCharacterMesh* Entry = Company
			? Company->GetCharacterMesh(Companion.CharacterMeshIndex)
			: nullptr;
		if (!Entry || Entry->Name.Equals(TEXT("Viking"), ESearchCase::IgnoreCase))
		{
			return Mesh;
		}

		if (USkeletalMesh* Found = SolidContentLookup::FindSkeletalMeshByToken(*Entry->AssetToken))
		{
			return Found;
		}
		if (!Mesh.IsNull())
		{
			return Mesh;
		}

		// Distinct from the Viking path so a missing pack does not dress the companion as the captain.
		const FString Path = FString::Printf(
			TEXT("/Game/Fab/%s/%s.%s"), *Entry->Name, *Entry->Name, *Entry->Name);
		return TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Path));
	}

	void CopyBodyControls(ASolidCompanionCharacter* Companion, const FSolidCompanion* Record, const FString& DisplayName)
	{
		if (!Companion)
		{
			return;
		}
		FSolidBodyControls Controls = SolidBodyControls::FromSeed(
			FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(DisplayName)));
		if (Record)
		{
			Controls.Height = Record->BodyHeight;
			Controls.Build = Record->BodyBuild;
			Controls.Gender = Record->BodyGender;
		}
		Companion->SetBodyControls(Controls.Height, Controls.Build, Controls.Gender);
	}

	ASolidCharacter* FindCaptain(UWorld* World)
	{
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Cast<ASolidCharacter>(Controller->GetPawn()) : nullptr;
	}

	int32 TownIdForEvent(const TCHAR* EventName)
	{
		if (!EventName)
		{
			return INDEX_NONE;
		}
		for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
		{
			if (FCString::Strcmp(SolidEvents::Definitions[Index].Name, EventName) == 0)
			{
				return SolidEvents::Definitions[Index].Parameter;
			}
		}
		return INDEX_NONE;
	}
}

static_assert(
	ASolidGameMode::DefaultCompanionCount <= SolidEvents::RosterCount,
	"The starting selection cannot be larger than the event roster.");

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
	Company->InitializeDefaultCharacterMeshes();
	Company->InitializeDefaultCompanions();

	if (!Party)
	{
		Party = NewObject<USolidParty>(this, TEXT("Party"));
		// Bind once. InitializeFromCompany always activates F1 and reselects
		// Sam and Alex, and the enter-town timer calls this every 0.1s.
		Party->InitializeFromCompany(Company);
	}
}

void ASolidGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogSolid, Warning, TEXT("Build %s"), SOLID_BUILD_ID);
	EnsureCompanyAndParty();
	RestoreTravelSession();

	if (IsInDungeon())
	{
		BeginDungeonInterior();
		FinishTravelArrival();
		return;
	}

	if (IsInLair())
	{
		BeginLairInterior();
		FinishTravelArrival();
		return;
	}

	if (IsInTower())
	{
		BeginTowerInterior();
		FinishTravelArrival();
		return;
	}

	FinishTravelArrival();
	EnsureTerrainStreamer();

	// Player pawn / terrain may not be ready on the first PIE frame — keep sampling.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EventRefreshTimer, this, &ASolidGameMode::RefreshEvents, 0.1f, true);
		World->GetTimerManager().SetTimer(
			VegetationSpawnTimer, this, &ASolidGameMode::EnsureVegetation, 0.5f, false);
	}
	RefreshEvents();
	EnsureVegetation();
}

bool ASolidGameMode::IsInDungeon() const
{
	if (SolidDungeonLevel::RequestsDungeon(OptionsString))
	{
		return true;
	}
	const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>();
	return Instance && Instance->IsInDungeon();
}

bool ASolidGameMode::IsInLair() const
{
	if (SolidLair::RequestsVisit(OptionsString))
	{
		return true;
	}
	const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>();
	return Instance && Instance->IsInLair();
}

bool ASolidGameMode::IsInTower() const
{
	if (SolidDungeonLevel::RequestsTower(OptionsString))
	{
		return true;
	}
	const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>();
	return Instance && Instance->IsInTower();
}

bool ASolidGameMode::AreTunnelsOpen() const
{
	return SolidLair::TunnelsOpen(Configuration);
}

void ASolidGameMode::SaveTravelSession(
	const bool bInDungeon,
	const bool bReturnToDungeonMouth,
	const bool bInLair,
	const bool bReturnToLairMouth,
	const TCHAR LairEnd,
	const bool bInTower,
	const bool bReturnToTowerDoor,
	const FIntPoint TowerCell,
	const FSolidStronghold Tower)
{
	USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>();
	if (!Instance)
	{
		UE_LOG(LogSolid, Error, TEXT("Closed-visit travel needs USolidGameInstance."));
		return;
	}

	FSolidTravelSession Session;
	Session.Configuration = Configuration;
	Session.bInDungeon = bInDungeon;
	Session.bReturnToDungeonMouth = bReturnToDungeonMouth;
	Session.bInLair = bInLair;
	Session.bReturnToLairMouth = bReturnToLairMouth;
	Session.LairEnd = LairEnd;
	Session.bInTower = bInTower;
	Session.bReturnToTowerDoor = bReturnToTowerDoor;
	Session.TowerCell = TowerCell;
	Session.Tower = Tower;
	if (Party)
	{
		Session.SelectedCompanionIndices = Party->GetSelectedCompanionIndices();
		Session.ActiveBattlePlanSlot = Party->GetActiveAssignedSlot();
	}
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* Controller = World->GetFirstPlayerController())
		{
			if (const ASolidCharacter* Captain = Cast<ASolidCharacter>(Controller->GetPawn()))
			{
				Session.Sight = Captain->GetSight();
			}
		}
	}
	Instance->Capture(Session);
}

void ASolidGameMode::RestoreTravelSession()
{
	const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>();
	if (!Instance || !Instance->HasSession())
	{
		return;
	}

	const FSolidTravelSession& Session = Instance->GetSession();
	Configuration = Session.Configuration;
	EnsureCompanyAndParty();
	if (Party)
	{
		Party->SetSelectedCompanions(Session.SelectedCompanionIndices);
		Party->SelectAssignedSlot(Session.ActiveBattlePlanSlot);
	}
}

void ASolidGameMode::ApplyCaptainFromSession()
{
	if (bCaptainTravelApplied)
	{
		return;
	}

	const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>();
	if (!Instance || !Instance->HasSession())
	{
		bCaptainTravelApplied = true;
		return;
	}

	ASolidCharacter* Captain = FindCaptain(GetWorld());
	if (!Captain)
	{
		return;
	}

	const FSolidTravelSession& Session = Instance->GetSession();
	if (SolidEvents::IsSightEnabled(Configuration, Session.Sight))
	{
		Captain->SetSight(Session.Sight);
	}
	if (Configuration.CaptainTitle)
	{
		Captain->SetCharacterDisplayName(Configuration.CaptainTitle);
	}
	bCaptainTravelApplied = true;
}

void ASolidGameMode::PlaceCaptainInDungeon()
{
	if (bCaptainPlacedInDungeon || !DungeonInterior)
	{
		return;
	}

	ASolidCharacter* Captain = FindCaptain(GetWorld());
	if (!Captain)
	{
		return;
	}

	float HalfHeight = 96.f;
	if (const UCapsuleComponent* Capsule = Captain->GetCapsuleComponent())
	{
		HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	}
	FVector Spawn = DungeonInterior->GetCaptainSpawnWorld();
	Spawn.Z += HalfHeight + 4.f;
	const FRotator View = DungeonInterior->GetCaptainView();
	Captain->ArriveByPortal(Spawn, View.Yaw, View.Pitch);
	// The pawn can exist a frame before it is possessed. Wait, or the old tilt sticks.
	bCaptainPlacedInDungeon = Captain->GetController() != nullptr;
}

void ASolidGameMode::FinishTravelArrival()
{
	ApplyCaptainFromSession();
	if (IsInDungeon())
	{
		HideTemplateProps();
		PlaceCaptainInDungeon();
	}
	else if (IsInLair())
	{
		HideTemplateProps();
		PlaceCaptainInLair();
	}
	else if (IsInTower())
	{
		HideTemplateProps();
		PlaceCaptainInTower();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const bool bDone = bCaptainTravelApplied
		&& (!IsInDungeon() || bCaptainPlacedInDungeon)
		&& (!IsInLair() || bCaptainPlacedInLair)
		&& (!IsInTower() || bCaptainPlacedInTower);
	if (bDone)
	{
		World->GetTimerManager().ClearTimer(CaptainArrivalTimer);
		return;
	}

	if (!World->GetTimerManager().IsTimerActive(CaptainArrivalTimer))
	{
		World->GetTimerManager().SetTimer(
			CaptainArrivalTimer, this, &ASolidGameMode::FinishTravelArrival, 0.1f, true);
	}
}

void ASolidGameMode::BeginDungeonInterior()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!DungeonInterior)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DungeonInterior = World->SpawnActor<ASolidDungeonInterior>(
			ASolidDungeonInterior::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
		if (DungeonInterior)
		{
			DungeonInterior->BuildVisuals();
		}
	}

	HideTemplateProps();
	PlaceCaptainInDungeon();
	UE_LOG(LogSolid, Warning, TEXT("Dungeon interior loaded. Companions return when the captain leaves."));
}

void ASolidGameMode::BeginLairInterior()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!LairInterior)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		LairInterior = World->SpawnActor<ASolidLairInterior>(
			ASolidLairInterior::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
		if (LairInterior)
		{
			LairInterior->BuildVisuals();
		}
	}

	HideTemplateProps();
	PlaceCaptainInLair();
	UE_LOG(LogSolid, Warning, TEXT("Iglin tunnels loaded. Companions return when the captain leaves."));
}

void ASolidGameMode::PlaceCaptainInLair()
{
	if (bCaptainPlacedInLair || !LairInterior)
	{
		return;
	}

	ASolidCharacter* Captain = FindCaptain(GetWorld());
	if (!Captain)
	{
		return;
	}

	TCHAR End = SolidLair::RequestedEnd(OptionsString);
	if (!SolidLair::IsEndMarker(End))
	{
		if (const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>())
		{
			if (Instance->HasSession())
			{
				End = Instance->GetSession().LairEnd;
			}
		}
	}
	if (!SolidLair::IsEndMarker(End))
	{
		End = USolidWorldMap::XMarker;
	}

	float HalfHeight = 96.f;
	if (const UCapsuleComponent* Capsule = Captain->GetCapsuleComponent())
	{
		HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	}
	FVector Spawn = LairInterior->GetCaptainSpawnWorld(End);
	Spawn.Z += HalfHeight + 4.f;
	const FRotator View = LairInterior->GetCaptainView(End);
	Captain->ArriveByPortal(Spawn, View.Yaw, View.Pitch);
	bCaptainPlacedInLair = Captain->GetController() != nullptr;
}

void ASolidGameMode::HideTemplateProps()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Lvl_ThirdPerson still contains its playground. The colorway blocks
	// (several of them green) stand inside the room we build at the origin.
	int32 Hidden = 0;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* Prop = *It;
		if (!IsValid(Prop) || Prop->IsHidden())
		{
			continue;
		}
		Prop->SetActorHiddenInGame(true);
		Prop->SetActorEnableCollision(false);
		++Hidden;
	}
	if (Hidden > 0)
	{
		UE_LOG(LogSolid, Warning, TEXT("Hid %d template blocks in the dungeon."), Hidden);
	}
}

SolidWorldTime::FStamp ASolidGameMode::WorldStamp() const
{
	if (const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>())
	{
		return Instance->WorldStamp();
	}
	return SolidWorldTime::FromSession(1, 0.0);
}

void ASolidGameMode::TravelToDungeon()
{
	if (!IsCaveEnabled())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || IsInDungeon() || IsInLair() || IsInTower())
	{
		return;
	}
	const SolidWorldTime::FStamp When = WorldStamp();
	SolidEvents::RememberStronghold(Configuration, When, /*bEnter=*/true);
	UE_LOG(LogSolid, Warning, TEXT("Journal: %s  Enter Stronghold"), *SolidWorldTime::Format(When));
	SaveTravelSession(/*bInDungeon=*/true, /*bReturnToDungeonMouth=*/false);
	UGameplayStatics::OpenLevel(World, FName(SolidDungeonLevel::WorldMapPath), true, SolidDungeonLevel::EnterOptions());
}

void ASolidGameMode::TravelToWorld()
{
	UWorld* World = GetWorld();
	if (!World || !IsInDungeon())
	{
		return;
	}
	// Companions are spawned again once this world is the open world.
	// The party builder is not opened here.
	const SolidWorldTime::FStamp When = WorldStamp();
	SolidEvents::RememberStronghold(Configuration, When, /*bEnter=*/false);
	UE_LOG(LogSolid, Warning, TEXT("Journal: %s  Exit Stronghold"), *SolidWorldTime::Format(When));
	SaveTravelSession(/*bInDungeon=*/false, /*bReturnToDungeonMouth=*/true);
	UGameplayStatics::OpenLevel(World, FName(SolidDungeonLevel::WorldMapPath), true, SolidDungeonLevel::LeaveOptions());
}

void ASolidGameMode::TravelToLair(const TCHAR EndMarker)
{
	if (!AreTunnelsOpen() || !SolidLair::IsEndMarker(EndMarker))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || IsInLair() || IsInDungeon() || IsInTower())
	{
		return;
	}
	const TCHAR End = FChar::ToUpper(EndMarker);
	const SolidWorldTime::FStamp When = WorldStamp();
	SolidEvents::RememberLair(Configuration, When, /*bEnter=*/true);
	UE_LOG(LogSolid, Warning, TEXT("Journal: %s  Enter Iglin tunnels"), *SolidWorldTime::Format(When));
	SaveTravelSession(
		/*bInDungeon=*/false,
		/*bReturnToDungeonMouth=*/false,
		/*bInLair=*/true,
		/*bReturnToLairMouth=*/false,
		End);
	UGameplayStatics::OpenLevel(World, FName(SolidDungeonLevel::WorldMapPath), true, SolidLair::EnterOptions(End));
}

void ASolidGameMode::TravelFromLair(const TCHAR EndMarker)
{
	UWorld* World = GetWorld();
	if (!World || !IsInLair())
	{
		return;
	}
	const TCHAR End = SolidLair::IsEndMarker(EndMarker) ? FChar::ToUpper(EndMarker) : USolidWorldMap::XMarker;
	const SolidWorldTime::FStamp When = WorldStamp();
	SolidEvents::RememberLair(Configuration, When, /*bEnter=*/false);
	UE_LOG(LogSolid, Warning, TEXT("Journal: %s  Exit Iglin tunnels"), *SolidWorldTime::Format(When));
	SaveTravelSession(
		/*bInDungeon=*/false,
		/*bReturnToDungeonMouth=*/false,
		/*bInLair=*/false,
		/*bReturnToLairMouth=*/true,
		End);
	UGameplayStatics::OpenLevel(World, FName(SolidDungeonLevel::WorldMapPath), true, SolidDungeonLevel::LeaveOptions());
}

void ASolidGameMode::TravelToTower(const FIntPoint Cell)
{
	UWorld* World = GetWorld();
	if (!World || IsInDungeon() || IsInLair() || IsInTower())
	{
		return;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldMap* WorldMap = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	TArray<SolidStrongholds::FEntrance> Entrances;
	SolidStrongholds::CollectEntrances(WorldMap, WorldMin, WorldMax, Entrances);
	FSolidStronghold Tower;
	bool bHaveTower = false;
	for (const SolidStrongholds::FEntrance& Entrance : Entrances)
	{
		SolidTowerStronghold::FPlan Plan;
		if (Entrance.Cell != Cell || !SolidTowerStronghold::PlanFor(Entrance.Stronghold, Plan))
		{
			continue;
		}
		Tower = Entrance.Stronghold;
		bHaveTower = true;
		break;
	}
	if (!bHaveTower
		&& Configuration.ActiveStrongholdCell == Cell
		&& Configuration.ActiveStronghold.Type == ESolidStrongholdType::Tower)
	{
		SolidTowerStronghold::FPlan Plan;
		SolidStrongholds::FEntrance SiteEntrance;
		if (SolidTowerStronghold::PlanFor(Configuration.ActiveStronghold, Plan)
			&& SolidStrongholds::MakeEntrance(
				WorldMap,
				WorldMin,
				WorldMax,
				Cell,
				Configuration.ActiveStronghold,
				SiteEntrance))
		{
			Tower = Configuration.ActiveStronghold;
			bHaveTower = true;
		}
	}
	if (!bHaveTower)
	{
		return;
	}

	const SolidWorldTime::FStamp When = WorldStamp();
	SolidEvents::RememberStronghold(Configuration, When, /*bEnter=*/true);
	UE_LOG(LogSolid, Warning, TEXT("Journal: %s  Enter Stronghold"), *SolidWorldTime::Format(When));
	SaveTravelSession(
		/*bInDungeon=*/false,
		/*bReturnToDungeonMouth=*/false,
		/*bInLair=*/false,
		/*bReturnToLairMouth=*/false,
		/*LairEnd=*/0,
		/*bInTower=*/true,
		/*bReturnToTowerDoor=*/false,
		Cell,
		Tower);
	UGameplayStatics::OpenLevel(World, FName(SolidDungeonLevel::WorldMapPath), true, SolidDungeonLevel::TowerEnterOptions());
}

bool ASolidGameMode::EnterChosenStronghold(
	const FIntPoint Cell,
	const ESolidStrongholdSize Size,
	const ESolidStrongholdType Type)
{
	UWorld* World = GetWorld();
	if (!World || IsInDungeon() || IsInLair() || IsInTower())
	{
		return false;
	}
	if (!SolidStrongholdTransition::Commit(Configuration, Cell, Size, Type))
	{
		return false;
	}

	const SolidWorldTime::FStamp When = WorldStamp();
	SolidEvents::RememberStronghold(Configuration, When, /*bEnter=*/true);
	UE_LOG(LogSolid, Warning, TEXT("Journal: %s  Enter Stronghold"), *SolidWorldTime::Format(When));
	SaveTravelSession(
		/*bInDungeon=*/false,
		/*bReturnToDungeonMouth=*/false,
		/*bInLair=*/false,
		/*bReturnToLairMouth=*/false,
		/*LairEnd=*/0,
		/*bInTower=*/true,
		/*bReturnToTowerDoor=*/false,
		Configuration.ActiveStrongholdCell,
		Configuration.ActiveStronghold);
	UGameplayStatics::OpenLevel(World, FName(SolidDungeonLevel::WorldMapPath), true, SolidDungeonLevel::TowerEnterOptions());
	return true;
}

void ASolidGameMode::TravelFromTower()
{
	UWorld* World = GetWorld();
	if (!World || !IsInTower())
	{
		return;
	}

	FIntPoint Cell = FIntPoint::ZeroValue;
	FSolidStronghold Tower;
	if (const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>())
	{
		Cell = Instance->GetSession().TowerCell;
		Tower = Instance->GetSession().Tower;
	}

	const SolidWorldTime::FStamp When = WorldStamp();
	SolidEvents::RememberStronghold(Configuration, When, /*bEnter=*/false);
	UE_LOG(LogSolid, Warning, TEXT("Journal: %s  Exit Stronghold"), *SolidWorldTime::Format(When));
	SaveTravelSession(
		/*bInDungeon=*/false,
		/*bReturnToDungeonMouth=*/false,
		/*bInLair=*/false,
		/*bReturnToLairMouth=*/false,
		/*LairEnd=*/0,
		/*bInTower=*/false,
		/*bReturnToTowerDoor=*/true,
		Cell,
		Tower);
	UGameplayStatics::OpenLevel(World, FName(SolidDungeonLevel::WorldMapPath), true, SolidDungeonLevel::LeaveOptions());
}

void ASolidGameMode::BeginTowerInterior()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!TowerInterior)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TowerInterior = World->SpawnActor<ASolidTowerInterior>(
			ASolidTowerInterior::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
		if (TowerInterior)
		{
			FSolidStronghold Tower;
			FIntPoint Cell = FIntPoint::ZeroValue;
			if (const USolidGameInstance* Instance = GetGameInstance<USolidGameInstance>())
			{
				Tower = Instance->GetSession().Tower;
				Cell = Instance->GetSession().TowerCell;
			}
			SolidTowerStronghold::FPlan Plan;
			if (!SolidTowerStronghold::PlanFor(Tower, Plan))
			{
				Plan = SolidTowerStronghold::Imperious;
			}
			TowerInterior->SetTowerCell(Cell);
			TowerInterior->SetPlan(Plan);
			TowerInterior->BuildVisuals();
			TowerInterior->PlaceFurnishings(Configuration.Furnishings);
		}
	}

	HideTemplateProps();
	PlaceCaptainInTower();
	UE_LOG(LogSolid, Warning, TEXT("Tower interior loaded. Companions return when the captain leaves."));
}

void ASolidGameMode::PlaceCaptainInTower()
{
	if (bCaptainPlacedInTower || !TowerInterior)
	{
		return;
	}

	ASolidCharacter* Captain = FindCaptain(GetWorld());
	if (!Captain)
	{
		return;
	}

	float HalfHeight = 96.f;
	if (const UCapsuleComponent* Capsule = Captain->GetCapsuleComponent())
	{
		HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	}
	FVector Spawn = TowerInterior->GetCaptainSpawnWorld();
	Spawn.Z += HalfHeight + 4.f;
	const FRotator View = TowerInterior->GetCaptainView();
	Captain->ArriveByPortal(Spawn, View.Yaw, View.Pitch);
	bCaptainPlacedInTower = Captain->GetController() != nullptr;
}

void ASolidGameMode::RefreshEvents()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (IsInDungeon() || IsInLair() || IsInTower())
	{
		return;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
	if (!Streamer && bAutoSpawnTerrainStreamer)
	{
		Streamer = ASolidTerrainStreamer::EnsureExists(World);
	}
	// The captain is moved onto location 0 once. Sampling earlier would
	// treat the map PlayerStart as the town they "entered".
	if (!Streamer || !Streamer->HasAttemptedStartTownRelocate())
	{
		return;
	}

	USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
	USolidWorldMap* WorldMap = Terrain ? Terrain->GetWorldMap() : nullptr;
	APlayerController* PC = World->GetFirstPlayerController();
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!Terrain || !WorldMap || !WorldMap->IsLoaded() || !PlayerPawn)
	{
		return;
	}

	const FVector Location = PlayerPawn->GetActorLocation();
	const int32 TownId = WorldMap->SampleLocationId(
		Location.X,
		Location.Y,
		Terrain->GetWorldMinXY(),
		Terrain->GetWorldMaxXY());
	NoteTownEntry(TownId);
	NoteBrokenBridgeRamp(Location);
	EnsureCompanion();
}

void ASolidGameMode::NoteTownEntry(const int32 TownId)
{
	const SolidEvents::FResult Result = SolidEvents::NotifyOccupiedTown(Configuration, TownId, WorldStamp());
	if (Result.bFired)
	{
		UE_LOG(LogSolid, Warning,
			TEXT("Event: entered town %d (%s). Sight %s. Max party %d. Title %s. Bridge %s."),
			TownId,
			*SolidTowns::NameFor(TownId),
			Result.bSetSight ? SolidSight::Label(Result.Sight) : TEXT("unchanged"),
			Configuration.MaxPartySize,
			Configuration.CaptainTitle ? Configuration.CaptainTitle : TEXT(""),
			Result.bRepairBridge ? TEXT("repaired") : TEXT("unchanged"));
	}

	if (Result.bRepairBridge)
	{
		RepairBrokenBridges();
	}

	if (ASolidCharacter* Captain = FindCaptain(GetWorld()))
	{
		if (Result.bSetSight)
		{
			Captain->SetSight(Result.Sight);
		}
		if (Result.bSetTitle && Result.CaptainTitle)
		{
			Captain->SetCharacterDisplayName(Result.CaptainTitle);
		}
	}
	PresentEventResult(Result);
}

void ASolidGameMode::PresentEventResult(const SolidEvents::FResult& Result)
{
	if (Result.bShowMessage && Result.MessageId && Result.MessageId[0])
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (ASolidHUD* HUD = Cast<ASolidHUD>(PC->GetHUD()))
				{
					const TCHAR* Text = SolidMessages::TextFor(Result.MessageId);
					HUD->ShowMessage(Result.MessageId, Text ? Text : TEXT(""));
				}
			}
		}
	}
	if (Result.bEnterCave)
	{
		TravelToDungeon();
	}
}

void ASolidGameMode::NoteMessageClosed(const TCHAR* MessageId)
{
	const SolidEvents::FResult Result = SolidEvents::NotifyMessageClosed(Configuration, MessageId, WorldStamp());
	if (Result.bFired)
	{
		UE_LOG(LogSolid, Warning, TEXT("Event: message %s closed. Cave %s."),
			MessageId ? MessageId : TEXT(""),
			Result.bEnterCave ? TEXT("entered") : TEXT("unchanged"));
	}
	PresentEventResult(Result);
}

void ASolidGameMode::NoteBrokenBridgeRamp(const FVector& Location)
{
	for (const TObjectPtr<ASolidBridge>& Actor : SpawnedBridges)
	{
		const ASolidBridge* Bridge = Actor.Get();
		if (!Bridge || !Bridge->HasBridge() || !Bridge->GetBridge().bIglinRelion)
		{
			continue;
		}
		if (!SolidGroundMove::OnRamp(Bridge->GetBridge(), Location.X, Location.Y))
		{
			continue;
		}

		const SolidEvents::FResult Result = SolidEvents::NotifyReachBrokenBridge(Configuration, WorldStamp());
		if (Result.bFired)
		{
			UE_LOG(LogSolid, Warning, TEXT("Event: Reach broken bridge."));
		}
		PresentEventResult(Result);
		return;
	}
}

void ASolidGameMode::FireParadeEvents()
{
	const int32 Stay = Configuration.OccupiedTown;
	auto Visit = [this](const int32 TownId)
	{
		if (TownId == INDEX_NONE)
		{
			return;
		}
		// A repeat visit to the town the captain is already on does not fire.
		if (Configuration.OccupiedTown == TownId)
		{
			Configuration.OccupiedTown = INDEX_NONE;
		}
		NoteTownEntry(TownId);
	};
	Visit(TownIdForEvent(TEXT("Enter Iglin")));
	Visit(TownIdForEvent(TEXT("Enter Relion")));
	Visit(TownIdForEvent(TEXT("Enter Dragongate")));
	Configuration.OccupiedTown = Stay;
	EnsureCompanion();
}

bool ASolidGameMode::IsSightEnabled(const ESolidSight Sight) const
{
	return SolidEvents::IsSightEnabled(Configuration, Sight);
}

void ASolidGameMode::RepairBrokenBridges()
{
	for (const TObjectPtr<ASolidBridge>& Actor : SpawnedBridges)
	{
		ASolidBridge* Bridge = Actor.Get();
		if (Bridge && Bridge->HasBridge() && Bridge->GetBridge().bBroken)
		{
			Bridge->SetBroken(false);
		}
	}
}

bool ASolidGameMode::SelectBattlePlanSlot(const int32 SlotIndex)
{
	EnsureCompanyAndParty();
	if (!Party)
	{
		return false;
	}

	if (const FSolidBattlePlan* Plan = Party->GetAssignedBattlePlan(SlotIndex))
	{
		if (Plan->Formation == ESolidBattleFormation::Parade)
		{
			FireParadeEvents();
		}
	}

	if (!Party->SelectAssignedSlot(SlotIndex))
	{
		return false;
	}

	UE_LOG(LogSolid, Warning, TEXT("%s"), *Party->GetActiveBattlePlanDebugString());
	return true;
}

bool ASolidGameMode::TeleportPartyToTown(const int32 TownId)
{
	UWorld* World = GetWorld();
	if (!World || !SolidTowns::Find(TownId))
	{
		return false;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldMap* WorldMap = Terrain ? Terrain->GetWorldMap() : nullptr;
	APlayerController* PC = World->GetFirstPlayerController();
	ASolidCharacter* Captain = PC ? Cast<ASolidCharacter>(PC->GetPawn()) : nullptr;
	if (!Streamer || !Terrain || !Terrain->IsBuilt() || !WorldMap || !WorldMap->IsLoaded() || !Captain)
	{
		return false;
	}
	if (!Streamer->HasAttemptedStartTownRelocate())
	{
		return false;
	}

	FSolidWorldLocation Location;
	if (!WorldMap->FindLocation(TownId, Location))
	{
		return false;
	}

	FIntPoint Cell;
	if (!SolidPortal::ChooseLandingCell(Location.Cells, Cell))
	{
		return false;
	}

	FVector2D CellXY = FVector2D::ZeroVector;
	if (!WorldMap->CellToWorldXY(Cell, Terrain->GetWorldMinXY(), Terrain->GetWorldMaxXY(), CellXY))
	{
		return false;
	}

	const FVector2D Landing = SolidPortal::LandingXY(CellXY, Streamer->StartTownPawnOffsetXY);
	const FVector Delta = SolidPortal::PartyDelta(Captain->GetActorLocation(), Landing);
	const float LandZ = Streamer->GetHeightAt(FVector(Landing.X, Landing.Y, 0.f)) + Streamer->CollisionHeightBias;
	float CapsuleHalf = 96.f;
	if (const UCapsuleComponent* Capsule = Captain->GetCapsuleComponent())
	{
		CapsuleHalf = Capsule->GetScaledCapsuleHalfHeight();
	}
	const FVector NewLoc(Landing.X, Landing.Y, LandZ + CapsuleHalf + Streamer->SnapHeightPadding);

	const FVector2D ToTown = CellXY - Landing;
	float Yaw = Captain->GetActorRotation().Yaw;
	if (!ToTown.IsNearlyZero())
	{
		Yaw = FMath::RadiansToDegrees(FMath::Atan2(ToTown.Y, ToTown.X));
	}

	Captain->ArriveByPortal(NewLoc, Yaw);
	Streamer->RelocateCompanionsByDelta(Delta);
	RefreshEvents();

	UE_LOG(LogSolid, Warning,
		TEXT("Portal to %s (%d) at (%.0f, %.0f) Z=%.1f"),
		*SolidTowns::NameFor(TownId), TownId, Landing.X, Landing.Y, NewLoc.Z);
	return true;
}

bool ASolidGameMode::TeleportPartyToTunnelMouth(const TCHAR EndMarker)
{
	UWorld* World = GetWorld();
	const TCHAR End = SolidLair::IsEndMarker(EndMarker) ? FChar::ToUpper(EndMarker) : 0;
	if (!World || !End)
	{
		return false;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldMap* WorldMap = Terrain ? Terrain->GetWorldMap() : nullptr;
	APlayerController* PC = World->GetFirstPlayerController();
	ASolidCharacter* Captain = PC ? Cast<ASolidCharacter>(PC->GetPawn()) : nullptr;
	if (!Streamer || !Terrain || !Terrain->IsBuilt() || !WorldMap || !WorldMap->IsLoaded() || !Captain)
	{
		return false;
	}
	if (!Streamer->HasAttemptedStartTownRelocate())
	{
		return false;
	}

	TArray<SolidLair::FEntrance> Entrances;
	SolidLair::CollectEntrances(WorldMap, Terrain->GetWorldMinXY(), Terrain->GetWorldMaxXY(), Entrances);
	const SolidLair::FEntrance* Mouth = nullptr;
	for (const SolidLair::FEntrance& Entrance : Entrances)
	{
		if (Entrance.EndMarker == End)
		{
			Mouth = &Entrance;
			break;
		}
	}
	if (!Mouth)
	{
		return false;
	}

	const FVector2D Landing = SolidPortal::NorthOfEntrance(Mouth->WorldXY);
	const FVector Delta = SolidPortal::PartyDelta(Captain->GetActorLocation(), Landing);
	const float LandZ = Streamer->GetHeightAt(FVector(Landing.X, Landing.Y, 0.f)) + Streamer->CollisionHeightBias;
	float CapsuleHalf = 96.f;
	if (const UCapsuleComponent* Capsule = Captain->GetCapsuleComponent())
	{
		CapsuleHalf = Capsule->GetScaledCapsuleHalfHeight();
	}
	const FVector NewLoc(Landing.X, Landing.Y, LandZ + CapsuleHalf + Streamer->SnapHeightPadding);

	Captain->ArriveByPortal(NewLoc, SolidPortal::YawTowardMouth(Mouth->FacingYawDeg));
	Streamer->RelocateCompanionsByDelta(Delta);

	const SolidEvents::FResult Result = SolidEvents::MarkComplete(
		Configuration, SolidEvents::IdDrivenFromVillage, WorldStamp());
	if (Result.bFired)
	{
		UE_LOG(LogSolid, Warning, TEXT("Event: Driven from the village recorded by the tunnel portal."));
	}
	if (ASolidCharacter* Landed = FindCaptain(GetWorld()))
	{
		if (Result.bSetTitle && Result.CaptainTitle)
		{
			Landed->SetCharacterDisplayName(Result.CaptainTitle);
		}
	}
	PresentEventResult(Result);
	RefreshEvents();

	UE_LOG(LogSolid, Warning,
		TEXT("Portal to %s at (%.0f, %.0f) Z=%.1f"),
		SolidLair::IglinTunnels.Name, Landing.X, Landing.Y, NewLoc.Z);
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
	const int32 PartySlotIndex,
	const TSoftObjectPtr<USkeletalMesh>& Mesh,
	const ESolidVocation Vocation)
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
	const int32 FormationCount = FMath::Max(SolidEvents::PartyCount(Configuration), 1);
	const FVector2D Slot = SolidBattleFormationSlots::SlotOffset(
		Formation, PartySlotIndex, FormationCount, Spacing);

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

	const FTransform SpawnTransform(PlayerPawn->GetActorRotation(), SpawnLoc);
	ASolidCompanionCharacter* Companion = World->SpawnActorDeferred<ASolidCompanionCharacter>(
		ClassToSpawn,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Companion)
	{
		UE_LOG(LogSolid, Error, TEXT("Failed to spawn companion %s."), *DisplayName);
		return nullptr;
	}

	const float Behind = FMath::Abs(Slot.X);
	Companion->FollowDistance = FMath::Max(Behind, 120.f);
	Companion->SideOffset = Slot.Y;
	Companion->CatchUpDistance = FMath::Max(700.f, Behind + 400.f);
	Companion->SetCompanionMesh(Mesh);
	Companion->SetPartySlotIndex(PartySlotIndex);
	Companion->SetVocation(Vocation);
	Companion->SetCharacterDisplayName(DisplayName);
	FLinearColor TorsoColor;
	FLinearColor LegColor;
	FLinearColor SkinColor;
	SolidCompanionColors::ColorsForName(DisplayName, TorsoColor, LegColor, SkinColor);
	if (Company)
	{
		if (const FSolidCompanion* Record = Company->GetCompanion(Company->FindCompanionIndexByName(DisplayName)))
		{
			TorsoColor = Record->TorsoColor;
			LegColor = Record->LegColor;
			SkinColor = Record->SkinColor;
		}
	}
	Companion->SetBodyColors(TorsoColor, LegColor, SkinColor);
	CopyBodyControls(
		Companion,
		Company ? Company->GetCompanion(Company->FindCompanionIndexByName(DisplayName)) : nullptr,
		DisplayName);
	Companion->SetFollowTarget(PlayerPawn);
	Companion->FinishSpawning(SpawnTransform);

	UE_LOG(LogSolid, Warning,
		TEXT("Spawned companion %s slot=%d at %s (fwd=%.0f right=%.0f) following %s"),
		*DisplayName, PartySlotIndex, *SpawnLoc.ToCompactString(), Slot.X, Slot.Y, *PlayerPawn->GetName());
	return Companion;
}

void ASolidGameMode::RefreshFieldedParty()
{
	EnsureCompanion();
}

void ASolidGameMode::EnsureCompanion()
{
	struct FRefreshCaptainName
	{
		ASolidGameMode* GameMode = nullptr;
		~FRefreshCaptainName()
		{
			if (ASolidCharacter* Captain = FindCaptain(GameMode ? GameMode->GetWorld() : nullptr))
			{
				Captain->UpdateNameLabelVisibility();
			}
		}
	} Refresh{this};

	if (IsInDungeon() || IsInLair() || IsInTower())
	{
		for (const TObjectPtr<ASolidCompanionCharacter>& Companion : SpawnedCompanions)
		{
			if (Companion)
			{
				Companion->Destroy();
			}
		}
		SpawnedCompanions.Reset();
		return;
	}

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

	const int32 Wanted = SolidEvents::PartyCount(Configuration);
	TArray<FSolidCompanion> Fielded;
	if (Party)
	{
		Party->CollectFieldedCompanions(Wanted, Fielded);
	}

	for (int32 Index = SpawnedCompanions.Num() - 1; Index >= 0; --Index)
	{
		ASolidCompanionCharacter* Companion = SpawnedCompanions[Index];
		const int32 Slot = Companion ? Companion->GetPartySlotIndex() : INDEX_NONE;
		const bool bKeep = Companion
			&& Fielded.IsValidIndex(Slot)
			&& Companion->GetCharacterDisplayName() == Fielded[Slot].Name;
		if (bKeep)
		{
			const TSoftObjectPtr<USkeletalMesh> Mesh = ResolveFieldedMesh(Company, Fielded[Slot]);
			if (Companion->GetCompanionMesh().ToSoftObjectPath() != Mesh.ToSoftObjectPath())
			{
				Companion->SetCompanionMesh(Mesh);
			}
			if (!Companion->GetTorsoColor().Equals(Fielded[Slot].TorsoColor)
				|| !Companion->GetLegColor().Equals(Fielded[Slot].LegColor)
				|| !Companion->GetSkinColor().Equals(Fielded[Slot].SkinColor))
			{
				Companion->SetBodyColors(
					Fielded[Slot].TorsoColor,
					Fielded[Slot].LegColor,
					Fielded[Slot].SkinColor);
			}
			if (Companion->GetBodyHeight() != Fielded[Slot].BodyHeight
				|| Companion->GetBodyBuild() != Fielded[Slot].BodyBuild
				|| Companion->GetBodyGender() != Fielded[Slot].BodyGender)
			{
				Companion->SetBodyControls(
					Fielded[Slot].BodyHeight,
					Fielded[Slot].BodyBuild,
					Fielded[Slot].BodyGender);
			}
			continue;
		}

		if (Companion)
		{
			UE_LOG(LogSolid, Warning,
				TEXT("Removed companion %s (max party %d)."),
				*Companion->GetCharacterDisplayName(),
				Wanted);
			Companion->Destroy();
		}
		SpawnedCompanions.RemoveAt(Index);
	}

	if (SpawnedCompanions.Num() >= Fielded.Num())
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
		return;
	}

	// Wait for the streamer to finish the one-shot location-0 relocate so companions
	// spawn next to the Captain at the starting town, not at the map PlayerStart.
	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
	if (Streamer && !Streamer->HasAttemptedStartTownRelocate())
	{
		return;
	}

	UClass* ClassToSpawn = CompanionClass
		? CompanionClass.Get()
		: ASolidCompanionCharacter::StaticClass();

	auto HasSlot = [this](const int32 Slot) -> bool
	{
		for (const TObjectPtr<ASolidCompanionCharacter>& Companion : SpawnedCompanions)
		{
			if (Companion && Companion->GetPartySlotIndex() == Slot)
			{
				return true;
			}
		}
		return false;
	};

	for (int32 Slot = 0; Slot < Fielded.Num(); ++Slot)
	{
		if (HasSlot(Slot))
		{
			continue;
		}
		const TSoftObjectPtr<USkeletalMesh> Mesh = ResolveFieldedMesh(Company, Fielded[Slot]);
		if (ASolidCompanionCharacter* Companion = SpawnCompanion(
			World, PlayerPawn, ClassToSpawn, Fielded[Slot].Name, Slot, Mesh, Fielded[Slot].Vocation))
		{
			SpawnedCompanions.Add(Companion);
		}
	}
}

void ASolidGameMode::ChooseForagePiles(const USolidTerrainMap* Map)
{
	if (bForageChosen || !Map || !Map->IsBuilt())
	{
		return;
	}
	if (!SolidForage::WorldHasPiles(IsInDungeon(), IsInTower(), IsInLair()))
	{
		return;
	}

	SolidForestTrees::FScatterParams Trees;
	Trees.Density = ForestTreeDensity;
	Trees.MaxTrees = MaxForestTrees;
	Trees.JitterCm = ForestTreeJitterCm;

	TArray<int32> TreePoints;
	SolidForestTrees::CollectChosenPointIndices(Map->GetPoints(), ForestTreeSeed, Trees, TreePoints);

	SolidForage::FScatterParams Piles;
	Piles.Density = SolidForage::Density;
	Piles.MaxPiles = SolidForage::MaxPiles;
	Piles.JitterCm = ForestTreeJitterCm;
	const int32 Scattered = SolidForage::CollectChoices(
		Map->GetPoints(), TreePoints, SolidForage::Seed, Piles, ForageChoices);

	int32 NearIglin = 0;
	FVector2D IglinXY = FVector2D::ZeroVector;
	const USolidWorldMap* WorldMap = Map->GetWorldMap();
	if (WorldMap && WorldMap->GetLocationWorldXY(
		SolidTowns::Iglin, Map->GetWorldMinXY(), Map->GetWorldMaxXY(), IglinXY))
	{
		NearIglin = SolidForage::GuaranteeForestCells(
			Map->GetPoints(),
			WorldMap,
			Map->GetWorldMinXY(),
			Map->GetWorldMaxXY(),
			IglinXY,
			SolidForage::IglinRadiusCm,
			TreePoints,
			ForageChoices);
	}

	bForageChosen = true;
	UE_LOG(LogSolid, Warning, TEXT("Chose %d forage piles (%d scattered, %d near Iglin)"),
		ForageChoices.Num(), Scattered, NearIglin);
}

void ASolidGameMode::SeedForagePiles()
{
	if (!bForageChosen || ForageChoices.Num() == 0)
	{
		return;
	}
	if (!SolidForage::WorldHasPiles(IsInDungeon(), IsInTower(), IsInLair()))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
	const USolidTerrainMap* Map = Streamer ? Streamer->GetTerrainMap() : nullptr;
	if (!Map || !Map->IsBuilt())
	{
		return;
	}

	TArray<int32> Ready;
	SolidForage::CollectReadyToSeed(
		Map->GetPoints(), ForageChoices, Configuration.TakenForagePiles, SeededForage, Ready);
	if (Ready.Num() == 0)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	int32 Spawned = 0;
	for (const int32 Index : Ready)
	{
		const SolidForage::FChoice& Choice = ForageChoices[Index];
		FVector SpawnLoc(Choice.XY.X, Choice.XY.Y, 0.f);
		const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
		SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

		ASolidForagePile* Pile = World->SpawnActor<ASolidForagePile>(
			ASolidForagePile::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		if (!Pile)
		{
			continue;
		}

		Pile->SetChoice(Index, Choice.ItemId);
		Pile->BuildVisuals();
		SpawnedForagePiles.Add(Pile);
		SeededForage.Add(Index);
		++Spawned;
	}

	if (Spawned > 0)
	{
		UE_LOG(LogSolid, Warning, TEXT("Seeded %d forage piles"), Spawned);
	}
}

bool ASolidGameMode::TakeForagePile(ASolidForagePile* Pile)
{
	if (!Pile)
	{
		return false;
	}
	const int32 Index = Pile->GetChoiceIndex();
	if (!ForageChoices.IsValidIndex(Index))
	{
		return false;
	}
	if (!SolidForage::TakePile(Configuration, Index, ForageChoices[Index].ItemId))
	{
		return false;
	}

	SpawnedForagePiles.Remove(Pile);
	Pile->Destroy();
	return true;
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
	SpawnedDungeonEntrances.RemoveAll([](const TObjectPtr<ASolidDungeonEntrance>& Entrance)
	{
		return !IsValid(Entrance);
	});
	SpawnedTowerShells.RemoveAll([](const TObjectPtr<ASolidTowerShell>& Shell)
	{
		return !IsValid(Shell);
	});
	SpawnedStrongholdStones.RemoveAll([](const TObjectPtr<ASolidStrongholdStone>& Stone)
	{
		return !IsValid(Stone);
	});
	SpawnedBridges.RemoveAll([](const TObjectPtr<ASolidBridge>& Bridge)
	{
		return !IsValid(Bridge);
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

	if (SpawnedDungeonEntrances.Num() == 0)
	{
		for (TActorIterator<ASolidDungeonEntrance> It(World); It; ++It)
		{
			SpawnedDungeonEntrances.Add(*It);
		}
	}

	if (SpawnedTowerShells.Num() == 0)
	{
		for (TActorIterator<ASolidTowerShell> It(World); It; ++It)
		{
			SpawnedTowerShells.Add(*It);
		}
	}

	if (SpawnedBridges.Num() == 0)
	{
		for (TActorIterator<ASolidBridge> It(World); It; ++It)
		{
			SpawnedBridges.Add(*It);
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
	TArray<SolidStrongholds::FEntrance> CaveEntrances;
	TArray<SolidLair::FEntrance> TunnelEntrances;
	TArray<FVector2D> SignClearCenters;
	if (const USolidWorldMap* WorldMap = Streamer->GetTerrainMap()->GetWorldMap())
	{
		const FVector2D WorldMin = Streamer->GetTerrainMap()->GetWorldMinXY();
		const FVector2D WorldMax = Streamer->GetTerrainMap()->GetWorldMaxXY();
		SolidTownSigns::CollectPlacements(WorldMap, WorldMin, WorldMax, SignPlacements);
		SolidStrongholds::CollectEntrances(WorldMap, WorldMin, WorldMax, CaveEntrances);
		SolidLair::CollectEntrances(WorldMap, WorldMin, WorldMax, TunnelEntrances);
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

		if (const USolidWorldMap* GateMap = Streamer->GetTerrainMap()->GetWorldMap())
		{
			TArray<SolidWallgate::FPiece> GatePieces;
			SolidWallgate::CollectPieces(
				GateMap,
				Streamer->GetTerrainMap()->GetWorldMinXY(),
				Streamer->GetTerrainMap()->GetWorldMaxXY(),
				GatePieces);
			for (const SolidWallgate::FPiece& Piece : GatePieces)
			{
				FVector SpawnLoc(Piece.CenterXY.X, Piece.CenterXY.Y, 0.f);
				const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
				SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

				ASolidBuilding* Building = World->SpawnActor<ASolidBuilding>(
					ASolidBuilding::StaticClass(),
					SpawnLoc,
					FRotator(0.f, Piece.YawDeg, 0.f),
					SpawnParams);
				if (!Building)
				{
					continue;
				}

				Building->FootprintXCm = Piece.FootprintXCm;
				Building->FootprintYCm = Piece.FootprintYCm;
				Building->BodyHeightCm = Piece.BodyHeightCm;
				Building->RoofHeightCm = Piece.RoofHeightCm;
				if (Piece.bStoneWall)
				{
					Building->BodyColor = SolidWallgate::StoneColor();
				}
				Building->BuildVisuals();
				SpawnedTownBuildings.Add(Building);
				++Spawned;
			}
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
				FRotator(0.f, Placement.YawDeg, 0.f),
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

	if (SpawnedDungeonEntrances.Num() == 0)
	{
		int32 Spawned = 0;
		for (const SolidStrongholds::FEntrance& Entrance : CaveEntrances)
		{
			if (!SolidStronghold::UsesCaveMouth(Entrance.Stronghold))
			{
				continue;
			}

			FVector SpawnLoc(Entrance.WorldXY.X, Entrance.WorldXY.Y, 0.f);
			const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
			SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

			ASolidDungeonEntrance* Cave = World->SpawnActor<ASolidDungeonEntrance>(
				ASolidDungeonEntrance::StaticClass(),
				SpawnLoc,
				FRotator(0.f, Entrance.FacingYawDeg, 0.f),
				SpawnParams);
			if (!Cave)
			{
				continue;
			}

			Cave->BuildVisuals();
			SpawnedDungeonEntrances.Add(Cave);
			++Spawned;
		}

		int32 TunnelMouths = 0;
		for (const SolidLair::FEntrance& Entrance : TunnelEntrances)
		{
			FVector SpawnLoc(Entrance.WorldXY.X, Entrance.WorldXY.Y, 0.f);
			const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
			SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

			ASolidDungeonEntrance* Mouth = World->SpawnActor<ASolidDungeonEntrance>(
				ASolidDungeonEntrance::StaticClass(),
				SpawnLoc,
				FRotator(0.f, Entrance.FacingYawDeg, 0.f),
				SpawnParams);
			if (!Mouth)
			{
				continue;
			}

			Mouth->SetLairEnd(Entrance.EndMarker);
			Mouth->BuildVisuals();
			SpawnedDungeonEntrances.Add(Mouth);
			++TunnelMouths;
		}

		UE_LOG(LogSolid, Warning, TEXT("Spawned %d cave entrances and %d tunnel mouths"), Spawned, TunnelMouths);
	}

	if (SpawnedTowerShells.Num() == 0)
	{
		USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
		USolidWorldMap* TowerMap = Terrain ? Terrain->GetWorldMap() : nullptr;
		int32 Spawned = 0;
		if (TowerMap && TowerMap->IsLoaded())
		{
			const FVector2D WorldMin = Terrain->GetWorldMinXY();
			const FVector2D WorldMax = Terrain->GetWorldMaxXY();
			for (const SolidStrongholds::FEntrance& Entrance : CaveEntrances)
			{
				SolidTowerStronghold::FPlan Plan;
				if (!SolidTowerStronghold::PlanFor(Entrance.Stronghold, Plan))
				{
					continue;
				}

				FVector2D CellXY = FVector2D::ZeroVector;
				if (!TowerMap->CellToWorldXY(Entrance.Cell, WorldMin, WorldMax, CellXY))
				{
					continue;
				}

				FVector SpawnLoc(CellXY.X, CellXY.Y, 0.f);
				const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
				SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;

				ASolidTowerShell* Shell = World->SpawnActor<ASolidTowerShell>(
					ASolidTowerShell::StaticClass(),
					SpawnLoc,
					FRotator(0.f, SolidTowerStronghold::ShellYawDeg(Entrance.FacingYawDeg), 0.f),
					SpawnParams);
				if (!Shell)
				{
					continue;
				}

				Shell->SetPlan(Plan);
				Shell->SetTowerCell(Entrance.Cell);
				Shell->BuildVisuals();
				SpawnedTowerShells.Add(Shell);
				++Spawned;
			}
		}

		UE_LOG(LogSolid, Warning, TEXT("Spawned %d tower shells"), Spawned);
	}

	if (!bSpawnedStrongholdSites)
	{
		USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
		USolidWorldMap* SiteMap = Terrain ? Terrain->GetWorldMap() : nullptr;
		int32 Stones = 0;
		int32 Shells = 0;
		if (SiteMap && SiteMap->IsLoaded())
		{
			const FVector2D WorldMin = Terrain->GetWorldMinXY();
			const FVector2D WorldMax = Terrain->GetWorldMaxXY();
			bSpawnedStrongholdSites = true;
			for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
			{
				for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
				{
					ESolidStrongholdSize SiteSize = ESolidStrongholdSize::Meagre;
					if (!SiteMap->StrongholdSiteAtCell(Col, Row, SiteSize))
					{
						continue;
					}

					const FIntPoint SiteCell(Col, Row);
					FSolidStronghold Exterior;
					bool bShell = false;
					if (SiteCell == Configuration.ActiveStrongholdCell
						&& Configuration.ActiveStronghold.Type == ESolidStrongholdType::Tower)
					{
						Exterior = Configuration.ActiveStronghold;
						bShell = true;
					}
					else
					{
						for (const FLeftStronghold& Left : Configuration.LeftStrongholds)
						{
							if (Left.Cell == SiteCell && Left.Stronghold.Type == ESolidStrongholdType::Tower)
							{
								Exterior = Left.Stronghold;
								bShell = true;
								break;
							}
						}
					}

					FVector2D CellXY = FVector2D::ZeroVector;
					if (!SiteMap->CellToWorldXY(SiteCell, WorldMin, WorldMax, CellXY))
					{
						continue;
					}
					FVector SpawnLoc(CellXY.X, CellXY.Y, 0.f);
					const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
					SolidStrongholds::FGrassFace Face;
					const float Yaw = SolidStrongholds::FindGrassFace(SiteMap, SiteCell, Face)
						? Face.FacingYawDeg
						: 0.f;

					SolidTowerStronghold::FPlan Plan;
					if (bShell && SolidTowerStronghold::PlanFor(Exterior, Plan))
					{
						SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;
						ASolidTowerShell* Shell = World->SpawnActor<ASolidTowerShell>(
							ASolidTowerShell::StaticClass(),
							SpawnLoc,
							FRotator(0.f, SolidTowerStronghold::ShellYawDeg(Yaw), 0.f),
							SpawnParams);
						if (!Shell)
						{
							continue;
						}
						Shell->SetPlan(Plan);
						Shell->SetTowerCell(SiteCell);
						Shell->BuildVisuals();
						SpawnedTowerShells.Add(Shell);
						++Shells;
						continue;
					}

					SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;
					ASolidStrongholdStone* Stone = World->SpawnActor<ASolidStrongholdStone>(
						ASolidStrongholdStone::StaticClass(),
						SpawnLoc,
						FRotator::ZeroRotator,
						SpawnParams);
					if (!Stone)
					{
						continue;
					}
					Stone->SetSite(SiteCell, SiteSize);
					Stone->BuildVisuals();
					SpawnedStrongholdStones.Add(Stone);
					++Stones;
				}
			}
		}
		UE_LOG(LogSolid, Warning, TEXT("Spawned %d stronghold stones and %d site shells"), Stones, Shells);
	}

	if (SpawnedBridges.Num() == 0)
	{
		USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
		USolidWorldMap* BridgeMap = Terrain ? Terrain->GetWorldMap() : nullptr;
		if (BridgeMap && BridgeMap->IsLoaded())
		{
			TArray<SolidBridge::FBridge> Bridges;
			SolidBridge::Collect(
				*BridgeMap,
				Terrain->GetWorldMinXY(),
				Terrain->GetWorldMaxXY(),
				[Streamer](const float X, const float Y)
				{
					return Streamer->GetHeightAt(FVector(X, Y, 0.f));
				},
				Bridges);

			int32 Spawned = 0;
			for (const SolidBridge::FBridge& Bridge : Bridges)
			{
				ASolidBridge* Actor = World->SpawnActor<ASolidBridge>(
					ASolidBridge::StaticClass(),
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					SpawnParams);
				if (!Actor)
				{
					continue;
				}
				SolidBridge::FBridge Spec = Bridge;
				if (Configuration.bIglinBridgeRepaired)
				{
					Spec.bBroken = false;
				}
				Actor->SetBridge(Spec);
				SpawnedBridges.Add(Actor);
				++Spawned;
			}
			UE_LOG(LogSolid, Warning, TEXT("Spawned %d bridges"), Spawned);
		}
	}

	ChooseForagePiles(Streamer->GetTerrainMap());
	SeedForagePiles();
}
