#include "SolidAutoPlaythrough.h"

#include "SolidCharacter.h"
#include "SolidCore1.h"
#include "SolidClickMove.h"
#include "SolidGameMode.h"
#include "GameFramework/PlayerController.h"
#include "SolidGameInstance.h"
#include "SolidGameState.h"
#include "Company/SolidCaptain.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Events/SolidMessages.h"
#include "HUD/SolidHUD.h"
#include "Inventory/SolidInventory.h"
#include "Lairs/SolidLair.h"
#include "Lairs/SolidLairEntrance.h"
#include "Lairs/SolidLairInterior.h"
#include "Events/SolidEncounter.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldChart.h"
#include "Company/SolidSquad.h"
#include "Company/SolidCompany.h"
#include "BattlePlans/SolidBattlePlan.h"
#include "Menus/SolidSquadBuilder.h"
#include "Menus/SolidPortal.h"
#include "Company/SolidCompanionCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Towns/SolidTowns.h"
#include "Strongholds/Sites/SolidStrongholdSite.h"
#include "Algo/Reverse.h"
#include "Misc/Parse.h"

namespace
{
	const SolidStrongholdGrid::FCell StandCells[] = {
		{ -1, 0 }, { 0, 0 }, { 1, 0 }, { 1, 1 },
	};

	const SolidStrongholdGrid::FCell EntranceCells[] = {
		{ 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -2, 0 },
	};
}

using FStep = FSolidAutoPlaythrough::FStep;
using EWalk = FSolidAutoPlaythrough::EWalk;

const FStep FSolidAutoPlaythrough::Script[FSolidAutoPlaythrough::ScriptCount] = {
	FStep::WalkToMarker(TEXT("Walk to Iglin Bridge encounter"), ESolidWorldChartMarkKind::Encounter, TEXT("Iglin Bridge")),
	FStep::WakeInStronghold(TEXT("Wake in the cave")),
	FStep::DisableEncounter(TEXT("Disable the Iglin Bridge encounter"), TEXT("Iglin Bridge")),
	FStep::MoveTo(TEXT("Move to the stand"), StandCells, UE_ARRAY_COUNT(StandCells)),
	FStep::CraftItem(TEXT("Craft the torch"), SolidItemType::IdTorch),
	FStep::EquipItem(TEXT("Equip the torch"), SolidItemType::IdTorch, SolidInventory::ESlot::OffHand),
	FStep::EquipItem(TEXT("Equip the sword"), SolidItemType::IdRustySword, SolidInventory::ESlot::MainHand).ClosePage(),
	FStep::MoveTo(TEXT("Move to the entrance"), EntranceCells, UE_ARRAY_COUNT(EntranceCells)),
	FStep::ExitStronghold(TEXT("Leave the cave")),
	FStep::TakePlayshot(TEXT("Take playshot")),
	FStep::WalkToMarker(TEXT("Walk to Iglin"), ESolidWorldChartMarkKind::Town, TEXT("Iglin")),
	FStep::CloseMessage(TEXT("Close the driven-out message"), SolidMessages::IdDrivenFromVillage),
	FStep::WalkToMarker(TEXT("Walk to the Iglin tunnels"), ESolidWorldChartMarkKind::LairEntrance, TEXT("Iglin tunnels")),
	FStep::EnterLair(TEXT("Enter the Iglin tunnels"), SolidLair::IdIglinTunnels, SolidLair::Entrance),
	FStep::MoveTo(TEXT("Move to the Iglin tunnels exit"), EWalk::Lair, SolidLair::IdIglinTunnels, SolidLair::Exit),
	FStep::ExitLair(TEXT("Leave the Iglin tunnels"), SolidLair::IdIglinTunnels, SolidLair::Exit),
	FStep::WalkToMarker(TEXT("Walk to Relion"), ESolidWorldChartMarkKind::Town, TEXT("Relion")),
	FStep::WalkToMarker(TEXT("Walk to Kanfold"), ESolidWorldChartMarkKind::Town, TEXT("Kanfold")),
	FStep::AimRaven(TEXT("Aim raven high behind the squad"), HighTrailPitchDeg, HighTrailArmCm),
	FStep::SelectBattlePlan(TEXT("Form Line"), TEXT("Line")),
	FStep::SelectBattlePlan(TEXT("Form Column"), TEXT("Column")),
	FStep::SelectBattlePlan(TEXT("Form Loose mob"), TEXT("Loose mob")),
	FStep::SelectBattlePlan(TEXT("Form Tight mob"), TEXT("Tight mob")),
	FStep::WalkToMarker(TEXT("Walk to Dragongate"), ESolidWorldChartMarkKind::Town, TEXT("Dragongate")),
	FStep::WalkToMarker(TEXT("Walk to the Imperious site"), ESolidWorldChartMarkKind::StrongholdSite, TEXT("Imperious")),
	FStep::ChooseStronghold(TEXT("Choose Imperious Tower"), ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower),
	FStep::WakeInStronghold(TEXT("Wake in the tower")),
	FStep::ExitStronghold(TEXT("Leave the tower")),
	FStep::BuildSquad(TEXT("Build squad Max"), TEXT("Max")),
	FStep::BuildSquad(TEXT("Build squad Morgan"), TEXT("Morgan")),
	FStep::BuildSquad(TEXT("Build squad Quinn"), TEXT("Quinn")).ClosePage(),
};

const FStep FSolidAutoPlaythrough::Parade[FSolidAutoPlaythrough::ParadeCount] = {
	FStep::PortalToMarker(
		TEXT("Portal to Dragongate"),
		ESolidWorldChartMarkKind::Town,
		TEXT("Dragongate"),
		0.f,
		-1000.f),
	FStep::TakePlayshot(TEXT("Playshot after the portal")),
	FStep::BuildSquad(TEXT("Build squad Sam"), TEXT("Sam")).ClosePage(),
	FStep::TakePlayshot(TEXT("Playshot after Sam")),
	FStep::FaceDirection(TEXT("Face south"), SolidTowns::SignFacesSouthYawDeg),
	FStep::TakePlayshot(TEXT("Playshot after facing south")),
	FStep::SelectBattlePlan(TEXT("Form Parade"), TEXT("Parade")),
	FStep::TakePlayshot(TEXT("Playshot after Parade")),
	FStep::EnableSight(TEXT("Enable true sight"), ESolidSight::True),
	FStep::Pause(3.f, TEXT("Pause")),
	FStep::TakePlayshot(TEXT("Take playshot")),
};

const FSolidAutoPlaythrough::FScript FSolidAutoPlaythrough::Scripts[FSolidAutoPlaythrough::ScriptsCount] = {
	{ TEXT("Full-playthrough"), Script, ScriptCount, true, true },
	{ TEXT("Companion parade"), Parade, ParadeCount, false, false },
};

namespace
{
	constexpr float ProgressCm = 1.f;
	/** Right-button drag, degrees per second. A hitch cannot spend more than one slice. */
	constexpr float TurnDegreesPerSecond = 180.f;
	constexpr float MaxTurnSliceSeconds = 0.05f;
	/** Inside one tunnel cell. The next center, a full cell away, stays ahead. */
	constexpr float LairArriveCm = SolidLair::CellCm * 0.45f;
	/**
	 * The walk stops within FSolidAutoPlaythrough::ArriveCm of the chart mark.
	 * The opening stands on the grass side of that mark.
	 */
	constexpr float MouthReachCm = 600.f;

	/** Center of the mesh bounds. The component origin is the ground at the lip. */
	FVector MeshCenter(const UStaticMeshComponent& Mesh)
	{
		return Mesh.CalcBounds(Mesh.GetComponentTransform()).GetBox().GetCenter();
	}

	bool SameItem(const FSolidItem& Item, const TCHAR* Id)
	{
		return Item.Id() && Id && FCString::Strcmp(Item.Id(), Id) == 0;
	}

	SolidInventory::FSlotHit ChestCell(const int32 Index)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bChest = true;
		Hit.Index = Index;
		return Hit;
	}

	SolidInventory::FSlotHit CraftSlot(const int32 Index)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bCraft = true;
		Hit.Index = Index;
		Hit.Column = Index;
		return Hit;
	}

	SolidInventory::FSlotHit PackCell(const int32 Column, const int32 Row)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bPack = true;
		Hit.Column = Column;
		Hit.Row = Row;
		Hit.Index = SolidInventory::PackIndex(Column, Row);
		return Hit;
	}

	SolidInventory::FSlotHit WornSlot(const SolidInventory::ESlot Slot)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.Slot = Slot;
		Hit.Index = SolidInventory::WornIndex(Slot);
		return Hit;
	}

	ASolidHUD* HudOf(const ASolidCharacter& Captain)
	{
		const APlayerController* PlayerController = Cast<APlayerController>(Captain.GetController());
		return PlayerController ? Cast<ASolidHUD>(PlayerController->GetHUD()) : nullptr;
	}

	ASolidStrongholdInterior* FindInterior(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<ASolidStrongholdInterior> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	/** The chest is on the page, and a message is not covering it. */
	bool InventoryPageCanDraw(const ASolidGameMode& Mode, const ASolidCharacter& Captain)
	{
		const ASolidHUD* HUD = HudOf(Captain);
		USolidGameState* State = Mode.GetConfiguration();
		const USolidCompany* Company = Mode.GetCompany();
		const FSolidStronghold* Hold = Company ? Company->GetStronghold() : nullptr;
		return HUD
			&& !HUD->IsMessageVisible()
			&& State
			&& Hold
			&& SolidStrongholdTransition::CompanyChestOpen(*State, Hold, Mode.IsInStronghold());
	}

	const FSolidWorldChartMark* FindChartMark(
		const USolidWorldChart& Chart,
		const ESolidWorldChartMarkKind Kind,
		const TCHAR* Name)
	{
		if (!Name)
		{
			return nullptr;
		}
		ESolidStrongholdSize SiteSize = ESolidStrongholdSize::Meagre;
		const bool bSiteSize = Kind == ESolidWorldChartMarkKind::StrongholdSite
			&& SolidStronghold::SizeFromToken(Name, SiteSize);
		for (const FSolidWorldChartMark& Mark : Chart.Marks)
		{
			if (Mark.Kind != Kind)
			{
				continue;
			}
			if (bSiteSize)
			{
				if (Mark.StrongholdSize == SiteSize)
				{
					return &Mark;
				}
				continue;
			}
			if (Mark.Name.Equals(Name, ESearchCase::IgnoreCase))
			{
				return &Mark;
			}
		}
		return nullptr;
	}

	bool IsIglinTunnels(const TCHAR* LairId)
	{
		return SolidLair::FindById(LairId) == &SolidLair::IglinTunnels;
	}

	bool LairSessionEnd(const USolidGameInstance& Instance, const TCHAR End)
	{
		return Instance.HasSession() && Instance.GetSession().LairEnd == FChar::ToUpper(End);
	}

	/**
	 * True sight, then the right-button drag, until the view is on this mesh.
	 * True while that turn is still running.
	 */
	bool StillTurningToward(const float DeltaSeconds, ASolidCharacter& Captain, const UStaticMeshComponent* Mesh)
	{
		if (!Mesh)
		{
			return false;
		}
		if (!Captain.IsTrueSight())
		{
			Captain.SetSight(ESolidSight::True);
		}

		const FVector Here = Captain.GetActorLocation();
		const FVector At = MeshCenter(*Mesh);
		float Yaw = 0.f;
		if (!FSolidAutoPlaythrough::TrailYawDeg(FVector2D(At.X - Here.X, At.Y - Here.Y), 0.f, Yaw))
		{
			return false;
		}

		const float YawError = FMath::FindDeltaAngleDegrees(Captain.GetControlRotation().Yaw, Yaw);
		if (FMath::Abs(YawError) < 0.05f)
		{
			return false;
		}

		const float MaxDegrees = TurnDegreesPerSecond * FMath::Min(DeltaSeconds, MaxTurnSliceSeconds);
		Captain.DragRightMouse(Yaw, MaxDegrees);
		return true;
	}

	/** Aim the pointer at this mesh and press the left button. The click handler runs. */
	bool ClickMesh(ASolidCharacter& Captain, const UStaticMeshComponent* Target)
	{
		Captain.ReleaseRightMouse();
		APlayerController* PlayerController = Cast<APlayerController>(Captain.GetController());
		if (!Target || !PlayerController)
		{
			return false;
		}

		FVector2D Screen = FVector2D::ZeroVector;
		if (!PlayerController->ProjectWorldLocationToScreen(MeshCenter(*Target), Screen))
		{
			return false;
		}
		Captain.SimulateLeftClick(Screen.X, Screen.Y);
		return true;
	}

	ASolidLairInterior* FindLairInterior(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<ASolidLairInterior> It(World); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	ASolidLairEntrance* FindMouthActor(UWorld* World, const TCHAR End)
	{
		if (!World || !SolidLair::IsEndMarker(End))
		{
			return nullptr;
		}
		const TCHAR Marker = FChar::ToUpper(End);
		for (TActorIterator<ASolidLairEntrance> It(World); It; ++It)
		{
			if (It->IsLairMouth() && It->GetEndMarker() == Marker)
			{
				return *It;
			}
		}
		return nullptr;
	}

	SolidStrongholdFurniture::FPlacedFurnishing* CompanyChest(ASolidGameMode& Mode)
	{
		USolidCompany* Company = Mode.GetCompany();
		FSolidStronghold* Stronghold = Company ? Company->GetStronghold() : nullptr;
		return Stronghold ? Stronghold->StorageAt(0) : nullptr;
	}

}

bool FSolidAutoPlaythrough::CraftTorch(SolidInventory::FLoadout& Loadout, SolidStorage::FContainer& Chest)
{
	const int32 Stick = SolidStorage::Index(1, 1);
	const int32 Pitch = SolidStorage::Index(0, 2);
	if (Stick == INDEX_NONE || Pitch == INDEX_NONE)
	{
		return false;
	}
	if (!SolidInventory::Move(&Chest, Loadout, ChestCell(Stick), CraftSlot(0)))
	{
		return false;
	}
	if (!SolidInventory::Move(&Chest, Loadout, ChestCell(Pitch), CraftSlot(1)))
	{
		return false;
	}
	if (!SolidInventory::Craft(Loadout))
	{
		return false;
	}
	if (!SolidInventory::Move(&Chest, Loadout, CraftSlot(0), ChestCell(Stick)))
	{
		return false;
	}
	return SolidInventory::Move(&Chest, Loadout, CraftSlot(1), ChestCell(Pitch));
}

bool FSolidAutoPlaythrough::EquipItem(
	SolidInventory::FLoadout& Loadout,
	SolidStorage::FContainer* Chest,
	const TCHAR* ItemId,
	const SolidInventory::ESlot Slot)
{
	const int32 Worn = SolidInventory::WornIndex(Slot);
	if (Worn == INDEX_NONE || !ItemId)
	{
		return false;
	}
	if (SameItem(Loadout.Worn[Worn], ItemId))
	{
		return true;
	}

	SolidInventory::FSlotHit From;
	for (int32 Index = 0; Index < SolidInventory::WornCount; ++Index)
	{
		if (SameItem(Loadout.Worn[Index], ItemId))
		{
			From = WornSlot(static_cast<SolidInventory::ESlot>(Index));
			return From.Index != INDEX_NONE
				&& SolidInventory::Move(Chest, Loadout, From, WornSlot(Slot));
		}
	}
	for (int32 Row = 0; Row < SolidInventory::PackRows; ++Row)
	{
		for (int32 Column = 0; Column < SolidInventory::PackColumns; ++Column)
		{
			const int32 Index = SolidInventory::PackIndex(Column, Row);
			if (Index != INDEX_NONE && SameItem(Loadout.Pack[Index], ItemId))
			{
				return SolidInventory::Move(Chest, Loadout, PackCell(Column, Row), WornSlot(Slot));
			}
		}
	}
	if (Chest)
	{
		for (int32 Row = 0; Row < SolidStorage::Rows; ++Row)
		{
			for (int32 Column = 0; Column < SolidStorage::Columns; ++Column)
			{
				const int32 Index = SolidStorage::Index(Column, Row);
				if (Index != INDEX_NONE && SameItem(Chest->Slots[Index], ItemId))
				{
					return SolidInventory::Move(Chest, Loadout, ChestCell(Index), WornSlot(Slot));
				}
			}
		}
	}
	return false;
}

bool FSolidAutoPlaythrough::CanStart(const USolidGameState& State)
{
	return CanStart(State, 0);
}

bool FSolidAutoPlaythrough::CanStart(const USolidGameState& State, const int32 ScriptIndex)
{
	if (ScriptIndex < 0 || ScriptIndex >= ScriptsCount)
	{
		return false;
	}
	if (!Scripts[ScriptIndex].bDisableAfterMove)
	{
		return true;
	}
	return !State.bAutoPlaythroughDim;
}

void FSolidAutoPlaythrough::Start(USolidGameState& State)
{
	Start(State, 0);
}

void FSolidAutoPlaythrough::Start(USolidGameState&, const int32 ScriptIndex)
{
	RunningScript = 0;
	if (ScriptIndex >= 0 && ScriptIndex < ScriptsCount)
	{
		RunningScript = ScriptIndex;
	}
	bRunning = true;
	StepIndex = 0;
	LoggedStep = INDEX_NONE;
	bAimRavenHeld = false;
	bHoldQuitForPlayshot = false;
	ResetWalk();
	NoteStepStart();
}

bool FSolidAutoPlaythrough::LaunchName(const TCHAR* CommandLine, FString& OutName)
{
	OutName.Reset();
	if (!CommandLine)
	{
		return false;
	}
	return FParse::Value(CommandLine, TEXT("Autoplay="), OutName) && !OutName.IsEmpty();
}

int32 FSolidAutoPlaythrough::FindScript(const TCHAR* Name)
{
	if (!Name)
	{
		return INDEX_NONE;
	}
	for (int32 Index = 0; Index < ScriptsCount; ++Index)
	{
		if (Scripts[Index].Name && FCString::Strcmp(Scripts[Index].Name, Name) == 0)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

const TCHAR* FSolidAutoPlaythrough::RunningName() const
{
	return Running().Name;
}

bool FSolidAutoPlaythrough::BeginLaunch(USolidGameState& State, const int32 ScriptIndex)
{
	if (bLaunchHandled)
	{
		return false;
	}
	bLaunchHandled = true;
	if (ScriptIndex < 0 || ScriptIndex >= ScriptsCount)
	{
		return false;
	}
	Start(State, ScriptIndex);
	bQuitWhenFinished = true;
	return true;
}

const FSolidAutoPlaythrough::FScript& FSolidAutoPlaythrough::Running() const
{
	if (RunningScript < 0 || RunningScript >= ScriptsCount || !Scripts[RunningScript].Steps)
	{
		return Scripts[0];
	}
	return Scripts[RunningScript];
}

void FSolidAutoPlaythrough::NoteCaptainPlace(USolidGameState& State, const FVector& Location)
{
	if (State.bAutoPlaythroughDim || !State.bAutoPlaythroughPlaced)
	{
		return;
	}
	if (!State.bAutoPlaythroughAnchorValid)
	{
		State.AutoPlaythroughAnchor = Location;
		State.bAutoPlaythroughAnchorValid = true;
		return;
	}
	if (FVector::Dist2D(State.AutoPlaythroughAnchor, Location) > MovedCm)
	{
		State.bAutoPlaythroughDim = true;
	}
}

void FSolidAutoPlaythrough::Stop(const TCHAR* Reason)
{
	if (bRunning)
	{
		UE_LOG(LogSolid, Warning, TEXT("Auto playthrough stop: %s"), Reason ? Reason : TEXT("no reason given"));
	}
	NoteStepEnd();
	bRunning = false;
	bAimRavenHeld = false;
	ResetWalk();
}

void FSolidAutoPlaythrough::NoteWait(const TCHAR* Reason)
{
	if (WaitReason == Reason)
	{
		return;
	}
	WaitReason = Reason;
	if (Reason)
	{
		UE_LOG(LogSolid, Warning, TEXT("Auto playthrough wait: %s"), Reason);
	}
}

void FSolidAutoPlaythrough::NoteStepStart()
{
	const FScript& ScriptRun = Running();
	if (!bRunning || LoggedStep == StepIndex || StepIndex < 0 || StepIndex >= ScriptRun.StepCount || !ScriptRun.Steps)
	{
		return;
	}
	LoggedStep = StepIndex;
	UE_LOG(LogSolid, Warning, TEXT("Auto playthrough start %d: %s"), StepIndex + 1, ScriptRun.Steps[StepIndex].Name);
}

void FSolidAutoPlaythrough::NoteStepEnd()
{
	const FScript& ScriptRun = Running();
	if (LoggedStep < 0 || LoggedStep >= ScriptRun.StepCount || !ScriptRun.Steps)
	{
		return;
	}
	UE_LOG(LogSolid, Warning, TEXT("Auto playthrough end %d: %s"), LoggedStep + 1, ScriptRun.Steps[LoggedStep].Name);
	LoggedStep = INDEX_NONE;
}

void FSolidAutoPlaythrough::Tick(const float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain)
{
	if (!bRunning)
	{
		Captain.ReleaseRightMouse();
		return;
	}

	const FScript& ScriptRun = Running();
	if (StepIndex < 0 || StepIndex >= ScriptRun.StepCount || !ScriptRun.Steps)
	{
		Stop(TEXT("step index is outside the script"));
		Captain.ReleaseRightMouse();
		return;
	}

	const FStep& Step = ScriptRun.Steps[StepIndex];
	const int32 StepAtStart = StepIndex;
	const EStepType StepType = Step.Type;
	const bool bSteering = TickCurrent(DeltaSeconds, Mode, Captain, Step);
	if (!bRunning)
	{
		Captain.ReleaseRightMouse();
		return;
	}
	if (!bSteering)
	{
		Captain.ReleaseRightMouse();
	}
	if (StepIndex == StepAtStart && StepType != EStepType::WalkTo && StepType != EStepType::MoveTo)
	{
		StallAccumulated += DeltaSeconds;
		if (StallAccumulated >= StallSeconds)
		{
			Stop(TEXT("step stall"));
		}
	}
	ApplyAimRaven(DeltaSeconds, Captain);
}

bool FSolidAutoPlaythrough::TickCurrent(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	switch (Step.Type)
	{
	case EStepType::WalkTo:
	case EStepType::MoveTo:
		return TickWalkTo(DeltaSeconds, Mode, Captain, Step);
	case EStepType::CloseMessage:
		return TickCloseMessage(DeltaSeconds, Captain, Step);
	case EStepType::CraftItem:
		return TickCraftItem(Mode, Captain, Step);
	case EStepType::EquipItem:
		return TickEquipItem(DeltaSeconds, Mode, Captain, Step);
	case EStepType::DisableEncounter:
		TickDisableEncounter(Mode, Step);
		return false;
	case EStepType::ExitStronghold:
		return TickExit(DeltaSeconds, Mode, Captain);
	case EStepType::EnterLair:
		return TickEnterLair(DeltaSeconds, Mode, Captain, Step);
	case EStepType::ExitLair:
		return TickExitLair(DeltaSeconds, Mode, Captain, Step);
	case EStepType::WakeInStronghold:
		TickWakeInStronghold(Mode);
		return false;
	case EStepType::SelectBattlePlan:
		return TickSelectBattlePlan(DeltaSeconds, Mode, Step);
	case EStepType::AimRaven:
		return TickAimRaven(DeltaSeconds, Step);
	case EStepType::ChooseStronghold:
		return TickChooseStronghold(DeltaSeconds, Mode, Captain, Step);
	case EStepType::BuildSquad:
		return TickBuildSquad(DeltaSeconds, Mode, Captain, Step);
	case EStepType::PortalToMarker:
		return TickPortal(Mode, Captain, Step);
	case EStepType::Pause:
		return TickPause(DeltaSeconds, Step);
	case EStepType::EnableSight:
		return TickEnableSight(Captain, Step);
	case EStepType::FaceDirection:
		return TickFaceDirection(Mode, Captain, Step);
	case EStepType::TakePlayshot:
		return TickTakePlayshot(Captain);
	case EStepType::Custom:
		TickCustom(Step);
		return false;
	default:
		Stop();
		return false;
	}
}

void FSolidAutoPlaythrough::Advance(const TCHAR* Reason)
{
	UE_LOG(LogSolid, Warning, TEXT("Auto playthrough advance: %s"), Reason ? Reason : TEXT("no reason given"));
	NoteStepEnd();
	++StepIndex;
	ResetWalk();
	if (StepIndex >= Running().StepCount)
	{
		Stop(TEXT("script finished"));
		return;
	}
	NoteStepStart();
}

void FSolidAutoPlaythrough::ResetWalk()
{
	bWalkReady = false;
	WaitReason = nullptr;
	bPageOpen = false;
	PointIndex = 0;
	Points.Reset();
	bHasBestDistance = false;
	BestDistanceCm = 0.f;
	StallAccumulated = 0.f;
	WaitAccumulated = 0.f;
}

void FSolidAutoPlaythrough::NoteDistance(const float DeltaSeconds, const float DistanceCm)
{
	if (!bHasBestDistance || DistanceCm < BestDistanceCm - ProgressCm)
	{
		BestDistanceCm = DistanceCm;
		bHasBestDistance = true;
		StallAccumulated = 0.f;
		return;
	}

	StallAccumulated += DeltaSeconds;
	if (StallAccumulated >= StallSeconds)
	{
		Stop(TEXT("distance stall"));
	}
}

bool FSolidAutoPlaythrough::BuildWalk(
	const ASolidGameMode& Mode,
	const FVector& CaptainLocation,
	const FStep& Step)
{
	UWorld* World = Mode.GetWorld();
	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Chart)
	{
		NoteWait(TEXT("no chart"));
		return false;
	}
	if (!Chart->IsLoaded())
	{
		NoteWait(TEXT("chart is not loaded"));
		return false;
	}

	const FSolidWorldChartMark* Mark = Step.MarkName
		? FindChartMark(*Chart, Step.MarkKind, Step.MarkName)
		: nullptr;
	if (!Mark)
	{
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Auto playthrough mark lookup failed: kind %d name %s"),
			static_cast<int32>(Step.MarkKind),
			Step.MarkName ? Step.MarkName : TEXT("(null)"));
		Stop(TEXT("mark lookup failed"));
		return false;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	FVector2D WorldXY = FVector2D::ZeroVector;
	if (!Chart->TerrainPointToWorld(Mark->TerrainPoint, WorldMin, WorldMax, WorldXY))
	{
		NoteWait(TEXT("mark has no world position"));
		return false;
	}

	Points.Reset();
	Points.Add(WorldXY);
	PointIndex = 0;
	bWalkReady = true;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	NoteWait(nullptr);
	UE_LOG(
		LogSolid,
		Warning,
		TEXT("Auto playthrough walk ready: mark (%.0f, %.0f) from (%.0f, %.0f)"),
		WorldXY.X,
		WorldXY.Y,
		CaptainLocation.X,
		CaptainLocation.Y);
	return true;
}

bool FSolidAutoPlaythrough::TickWalk(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	if (!Step.MarkName)
	{
		Stop(TEXT("walk has no mark name"));
		return false;
	}

	const FVector Location = Captain.GetActorLocation();
	const FVector2D Here(Location.X, Location.Y);
	if (!bWalkReady && !BuildWalk(Mode, Location, Step))
	{
		return false;
	}
	if (!Points.IsValidIndex(PointIndex))
	{
		Stop(TEXT("walk has no current point"));
		return false;
	}

	const FVector2D Target = Points[PointIndex];
	if (SolidClickMove::HasArrived(Here, Target, ArriveCm))
	{
		bHasBestDistance = false;
		StallAccumulated = 0.f;
		Advance(TEXT("reached the mark"));
		return false;
	}

	NoteDistance(DeltaSeconds, FVector2D::Distance(Here, Target));
	if (!bRunning)
	{
		return false;
	}

	return Steer(DeltaSeconds, Captain, Target, true);
}

bool FSolidAutoPlaythrough::Steer(
	const float DeltaSeconds,
	ASolidCharacter& Captain,
	const FVector2D& Target,
	const bool bLast,
	const float ArriveRadiusCm)
{
	const FVector Location = Captain.GetActorLocation();
	const FVector2D Here(Location.X, Location.Y);
	if (Captain.IsTrueSight())
	{
		const float SteerCm = bLast ? 1.f : ArriveRadiusCm;
		const FVector2D Direction = SolidClickMove::PlanarDirection(Here, Target, SteerCm);
		if (Direction.IsNearlyZero())
		{
			return false;
		}
		const float MaxDegrees = TurnDegreesPerSecond * FMath::Min(DeltaSeconds, MaxTurnSliceSeconds);
		Captain.DragRightMouse(
			FVector(Direction.X, Direction.Y, 0.f).Rotation().Yaw,
			MaxDegrees);
		Captain.SimulateKey(EKeys::W);
		return true;
	}

	// A screen click can hit a tree in front of the point. Walk to the point.
	Captain.SetClickMoveDestination(FVector(Target.X, Target.Y, Location.Z));
	return false;
}

bool FSolidAutoPlaythrough::FollowPoints(
	const float DeltaSeconds,
	const FVector2D& Here,
	FVector2D& OutTarget,
	bool& bOutFinished,
	const float ArriveRadiusCm)
{
	bOutFinished = false;
	if (!Points.IsValidIndex(PointIndex))
	{
		bOutFinished = bWalkReady;
		return false;
	}

	const FVector2D Target = Points[PointIndex];
	const bool bLast = PointIndex + 1 >= Points.Num();
	if (SolidClickMove::HasArrived(Here, Target, ArriveRadiusCm))
	{
		bHasBestDistance = false;
		StallAccumulated = 0.f;
		if (!bLast)
		{
			++PointIndex;
			return false;
		}
		PointIndex = Points.Num();
		bOutFinished = true;
		return false;
	}

	NoteDistance(DeltaSeconds, FVector2D::Distance(Here, Target));
	if (!bRunning)
	{
		return false;
	}
	OutTarget = Target;
	return true;
}

bool FSolidAutoPlaythrough::BuildHexWalk(
	const ASolidGameMode& Mode,
	const SolidStrongholdGrid::FCell* Cells,
	const int32 Count)
{
	ASolidStrongholdInterior* Interior = FindInterior(Mode.GetWorld());
	if (!Interior || !Cells || Count <= 0)
	{
		return false;
	}

	Points.Reset();
	Points.Reserve(Count);
	const FTransform& ActorXf = Interior->GetActorTransform();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector2D Local = SolidStrongholdGrid::HexCenter(
			Cells[Index].Col,
			Cells[Index].Row,
			SolidDungeonLevel::MeagreOrigin());
		const FVector World = ActorXf.TransformPosition(FVector(Local.X, Local.Y, 0.f));
		Points.Add(FVector2D(World.X, World.Y));
	}
	PointIndex = 0;
	bWalkReady = Points.Num() > 0;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	return bWalkReady;
}

bool FSolidAutoPlaythrough::LairCells(
	const TCHAR* LairId,
	const FIntPoint From,
	const FIntPoint To,
	TArray<FIntPoint>& OutCells)
{
	OutCells.Reset();
	if (!IsIglinTunnels(LairId))
	{
		return false;
	}
	if (!SolidLair::IsOpenCell(From.X, From.Y) || !SolidLair::IsOpenCell(To.X, To.Y))
	{
		return false;
	}
	if (From == To)
	{
		OutCells.Add(From);
		return true;
	}

	const int32 Count = SolidLair::MazeCols * SolidLair::MazeRows;
	TArray<int32> CameFrom;
	CameFrom.Init(INDEX_NONE, Count);
	const auto IndexOf = [](const FIntPoint Cell)
	{
		return Cell.X + Cell.Y * SolidLair::MazeCols;
	};
	TArray<FIntPoint> Queue;
	Queue.Reserve(Count);
	CameFrom[IndexOf(From)] = IndexOf(From);
	Queue.Add(From);
	const FIntPoint Steps[] = {
		{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
	};
	int32 Head = 0;
	while (Head < Queue.Num())
	{
		const FIntPoint Here = Queue[Head++];
		if (Here == To)
		{
			break;
		}
		for (const FIntPoint& Step : Steps)
		{
			const FIntPoint Next(Here.X + Step.X, Here.Y + Step.Y);
			if (!SolidLair::IsOpenCell(Next.X, Next.Y))
			{
				continue;
			}
			const int32 NextIndex = IndexOf(Next);
			if (CameFrom[NextIndex] != INDEX_NONE)
			{
				continue;
			}
			CameFrom[NextIndex] = IndexOf(Here);
			Queue.Add(Next);
		}
	}
	if (CameFrom[IndexOf(To)] == INDEX_NONE)
	{
		return false;
	}

	FIntPoint Walk = To;
	while (Walk != From)
	{
		OutCells.Add(Walk);
		const int32 Previous = CameFrom[IndexOf(Walk)];
		Walk = FIntPoint(Previous % SolidLair::MazeCols, Previous / SolidLair::MazeCols);
	}
	OutCells.Add(From);
	Algo::Reverse(OutCells);
	return OutCells.Num() > 0;
}

bool FSolidAutoPlaythrough::LairCells(
	const TCHAR* LairId,
	const TCHAR FromEnd,
	const TCHAR ToEnd,
	TArray<FIntPoint>& OutCells)
{
	OutCells.Reset();
	if (!SolidLair::IsEndMarker(FromEnd) || !SolidLair::IsEndMarker(ToEnd))
	{
		return false;
	}
	return LairCells(LairId, SolidLair::CellOf(FromEnd), SolidLair::CellOf(ToEnd), OutCells);
}

bool FSolidAutoPlaythrough::TrailYawDeg(
	const FVector2D& Movement,
	const float YawOffsetDeg,
	float& OutYaw)
{
	if (Movement.IsNearlyZero())
	{
		return false;
	}
	OutYaw = FVector(Movement.X, Movement.Y, 0.f).Rotation().Yaw + YawOffsetDeg;
	return true;
}

int32 FSolidAutoPlaythrough::BattlePlanSlot(const USolidSquad& Squad, const TCHAR* PlanName)
{
	if (!PlanName || !PlanName[0])
	{
		return INDEX_NONE;
	}
	for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
	{
		const FSolidBattlePlan* Plan = Squad.GetAssignedBattlePlan(Slot);
		if (Plan && Plan->Is(PlanName))
		{
			return Slot;
		}
	}
	return INDEX_NONE;
}

bool FSolidAutoPlaythrough::BuildLairWalk(
	const ASolidGameMode& Mode,
	const FVector& CaptainLocation,
	const FStep& Step)
{
	if (!IsIglinTunnels(Step.LairId) || !SolidLair::IsEndMarker(Step.EndMarker))
	{
		Stop();
		return false;
	}
	if (!Mode.IsInLair() || !Mode.HasPlacedCaptainInLair())
	{
		return false;
	}

	ASolidLairInterior* Interior = FindLairInterior(Mode.GetWorld());
	if (!Interior)
	{
		return false;
	}

	const FVector Local = Interior->GetActorTransform().InverseTransformPosition(CaptainLocation);
	const FIntPoint Here(
		FMath::FloorToInt(Local.X / SolidLair::CellCm),
		FMath::FloorToInt(Local.Y / SolidLair::CellCm));
	TArray<FIntPoint> Cells;
	if (!LairCells(Step.LairId, Here, SolidLair::CellOf(Step.EndMarker), Cells))
	{
		Stop();
		return false;
	}

	Points.Reset();
	Points.Reserve(Cells.Num());
	const FTransform& ActorXf = Interior->GetActorTransform();
	for (const FIntPoint& Cell : Cells)
	{
		const FVector2D Center = SolidLair::CellCenter(Cell.X, Cell.Y);
		const FVector World = ActorXf.TransformPosition(FVector(Center.X, Center.Y, 0.f));
		Points.Add(FVector2D(World.X, World.Y));
	}
	PointIndex = 0;
	bWalkReady = Points.Num() > 0;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	return bWalkReady;
}

bool FSolidAutoPlaythrough::TickWalkTo(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	switch (Step.Walk)
	{
	case EWalk::Marker:
		return TickWalk(DeltaSeconds, Mode, Captain, Step);
	case EWalk::Hexes:
		return TickHexWalk(DeltaSeconds, Mode, Captain, Step);
	case EWalk::Lair:
		return TickWalkLair(DeltaSeconds, Mode, Captain, Step);
	default:
		Stop();
		return false;
	}
}

bool FSolidAutoPlaythrough::TickHexWalk(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	if (!bWalkReady && !BuildHexWalk(Mode, Step.Cells, Step.CellCount))
	{
		return false;
	}

	const FVector2D Here(Captain.GetActorLocation().X, Captain.GetActorLocation().Y);
	FVector2D Target = FVector2D::ZeroVector;
	bool bFinished = false;
	if (FollowPoints(DeltaSeconds, Here, Target, bFinished))
	{
		return Steer(DeltaSeconds, Captain, Target, false);
	}
	if (bRunning && bFinished)
	{
		Advance();
	}
	return false;
}

void FSolidAutoPlaythrough::TickCustom(const FStep& Step)
{
	// A later custom row switches on Step.CustomId.
	(void)Step;
	Stop();
}

void FSolidAutoPlaythrough::TickWakeInStronghold(const ASolidGameMode& Mode)
{
	if (Mode.HasPlacedCaptainInStronghold())
	{
		bAimRavenHeld = false;
		Advance();
	}
}

bool FSolidAutoPlaythrough::TickCraftItem(
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	if (!Step.ItemId || FCString::Strcmp(Step.ItemId, SolidItemType::IdTorch) != 0)
	{
		Stop();
		return false;
	}

	if (bPageOpen)
	{
		USolidGameState* State = Mode.GetConfiguration();
		if (!State)
		{
			return false;
		}
		SolidStrongholdFurniture::FPlacedFurnishing* Piece = CompanyChest(Mode);
		if (!Piece)
		{
			Stop();
			return false;
		}
		SolidStorage::FContainer Chest = Piece->Container();
		if (!CraftTorch(State->Inventory, Chest))
		{
			Piece->SetContainer(Chest);
			Stop();
			return false;
		}
		Piece->SetContainer(Chest);
		Advance();
		return false;
	}

	if (!InventoryPageCanDraw(Mode, Captain))
	{
		return false;
	}
	if (ASolidHUD* HUD = HudOf(Captain))
	{
		if (!HUD->IsInventoryVisible())
		{
			HUD->ToggleInventory();
		}
		bPageOpen = HUD->IsInventoryVisible();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickEquipItem(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	USolidGameState* State = Mode.GetConfiguration();
	if (!State || !Step.ItemId)
	{
		Stop();
		return false;
	}
	SolidStrongholdFurniture::FPlacedFurnishing* Piece = CompanyChest(Mode);
	if (!Piece)
	{
		Stop();
		return false;
	}
	SolidStorage::FContainer Chest = Piece->Container();
	if (!EquipItem(State->Inventory, &Chest, Step.ItemId, Step.Slot))
	{
		Piece->SetContainer(Chest);
		Stop();
		return false;
	}
	Piece->SetContainer(Chest);
	if (!Step.bClosePage)
	{
		Advance();
		return false;
	}
	WaitAccumulated += DeltaSeconds;
	if (WaitAccumulated < Step.PauseSeconds)
	{
		return false;
	}
	if (ASolidHUD* HUD = HudOf(Captain))
	{
		if (HUD->IsInventoryVisible())
		{
			HUD->ToggleInventory();
		}
	}
	Advance();
	return false;
}

void FSolidAutoPlaythrough::TickDisableEncounter(ASolidGameMode& Mode, const FStep& Step)
{
	if (!Step.EncounterName)
	{
		Stop();
		return;
	}
	Mode.DisableEncounter(Step.EncounterName);
	Advance();
}

bool FSolidAutoPlaythrough::ClickMouth(ASolidGameMode& Mode, ASolidCharacter& Captain)
{
	UWorld* World = Mode.GetWorld();
	ASolidStrongholdInterior* Interior = FindInterior(World);
	if (!ClickMesh(Captain, Interior ? Interior->GetPassageMesh() : nullptr))
	{
		return false;
	}
	const USolidGameInstance* Instance = World ? World->GetGameInstance<USolidGameInstance>() : nullptr;
	return Instance && Instance->WantsStrongholdExit();
}

bool FSolidAutoPlaythrough::TickExit(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain)
{
	// Raven sight aims the squad boom. The passage click needs true sight looking at the opening.
	bAimRavenHeld = false;
	UWorld* World = Mode.GetWorld();
	const ASolidStrongholdInterior* Interior = FindInterior(World);
	const UStaticMeshComponent* Passage = Interior ? Interior->GetPassageMesh() : nullptr;
	if (StillTurningToward(DeltaSeconds, Captain, Passage))
	{
		return true;
	}

	if (ClickMouth(Mode, Captain))
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickWalkLair(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	const FVector Location = Captain.GetActorLocation();
	if (!bWalkReady && !BuildLairWalk(Mode, Location, Step))
	{
		return false;
	}

	const FVector2D Here(Location.X, Location.Y);
	FVector2D Target = FVector2D::ZeroVector;
	bool bFinished = false;
	if (FollowPoints(DeltaSeconds, Here, Target, bFinished, LairArriveCm))
	{
		return Steer(DeltaSeconds, Captain, Target, false, LairArriveCm);
	}
	if (bRunning && bFinished)
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickEnterLair(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	if (!IsIglinTunnels(Step.LairId) || !SolidLair::IsEndMarker(Step.EndMarker))
	{
		Stop();
		return false;
	}

	bAimRavenHeld = false;
	UWorld* World = Mode.GetWorld();
	const ASolidLairEntrance* Entrance = FindMouthActor(World, Step.EndMarker);
	const UStaticMeshComponent* Mouth = Entrance ? Entrance->GetMouthMesh() : nullptr;
	if (!Mouth)
	{
		NoteWait(TEXT("no tunnel mouth"));
		return false;
	}

	const bool bTurning = StillTurningToward(DeltaSeconds, Captain, Mouth);
	if (bTurning)
	{
		WaitAccumulated += DeltaSeconds;
	}
	else
	{
		ClickMesh(Captain, Mouth);
	}

	const USolidGameInstance* Instance = World ? World->GetGameInstance<USolidGameInstance>() : nullptr;
	const bool bInside = Instance && Instance->IsInLair() && LairSessionEnd(*Instance, Step.EndMarker);
	if (bInside)
	{
		Advance();
		return false;
	}
	if (bTurning && WaitAccumulated < StallSeconds)
	{
		return true;
	}
	if (!Mode.AreTunnelsOpen())
	{
		NoteWait(TEXT("tunnels are shut"));
		return false;
	}

	const FVector Center = MeshCenter(*Mouth);
	const float Reach = FVector::Dist2D(Captain.GetActorLocation(), Center);
	if (Reach > MouthReachCm)
	{
		NoteWait(TEXT("tunnel mouth is out of reach"));
		return false;
	}

	Mode.TravelToLair(Step.EndMarker);
	if (Instance && Instance->IsInLair() && LairSessionEnd(*Instance, Step.EndMarker))
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickExitLair(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	if (!IsIglinTunnels(Step.LairId) || !SolidLair::IsEndMarker(Step.EndMarker))
	{
		Stop();
		return false;
	}
	if (!Mode.IsInLair() || !Mode.HasPlacedCaptainInLair())
	{
		return false;
	}

	bAimRavenHeld = false;
	UWorld* World = Mode.GetWorld();
	const ASolidLairInterior* Interior = FindLairInterior(World);
	const UStaticMeshComponent* Mouth = Interior ? Interior->GetEndMouth(Step.EndMarker) : nullptr;
	if (StillTurningToward(DeltaSeconds, Captain, Mouth))
	{
		return true;
	}
	if (!ClickMesh(Captain, Mouth))
	{
		return false;
	}

	const USolidGameInstance* Instance = World ? World->GetGameInstance<USolidGameInstance>() : nullptr;
	if (Instance && Instance->WantsLairMouthSpawn() && LairSessionEnd(*Instance, Step.EndMarker))
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickCloseMessage(
	const float DeltaSeconds,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	ASolidHUD* HUD = HudOf(Captain);
	if (!HUD || !HUD->IsMessageVisible() || !Step.MessageId || HUD->GetMessageId() != Step.MessageId)
	{
		WaitAccumulated = 0.f;
		return false;
	}
	WaitAccumulated += DeltaSeconds;
	if (WaitAccumulated < Step.PauseSeconds)
	{
		return false;
	}
	HUD->CloseMessage();
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickSelectBattlePlan(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	const FStep& Step)
{
	USolidSquad* Squad = Mode.GetSquad();
	if (!Squad || !Step.PlanName)
	{
		Stop();
		return false;
	}

	const FSolidBattlePlan* Active = Squad->GetActiveBattlePlan();
	if (!Active || !Active->Is(Step.PlanName))
	{
		int32 Slot = BattlePlanSlot(*Squad, Step.PlanName);
		if (Slot == INDEX_NONE)
		{
			USolidGameState* State = Mode.GetConfiguration();
			USolidCompany* Company = Mode.GetCompany();
			USolidCaptain* SessionCaptain = Company ? Company->GetCaptain() : nullptr;
			if (!State || !SessionCaptain)
			{
				return false;
			}
			SolidEvents::FResult Result;
			SolidEvents::ApplyChange(
				*State,
				*SessionCaptain,
				SolidEvents::Change::EnableBattlePlan(Step.PlanName),
				Result);
			Squad->SyncEnabledBattlePlans(*State);
			Slot = BattlePlanSlot(*Squad, Step.PlanName);
		}
		if (Slot == INDEX_NONE || !Mode.SelectBattlePlanSlot(Slot))
		{
			Stop();
			return false;
		}
	}

	WaitAccumulated += DeltaSeconds;
	if (WaitAccumulated < Step.PauseSeconds)
	{
		return false;
	}
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickAimRaven(const float DeltaSeconds, const FStep& Step)
{
	bAimRavenHeld = true;
	RavenPitchDeg = Step.PitchDeg;
	RavenArmCm = Step.ArmCm;
	RavenYawOffsetDeg = Step.YawOffsetDeg;
	WaitAccumulated += DeltaSeconds;
	if (WaitAccumulated < Step.PauseSeconds)
	{
		return false;
	}
	Advance();
	return false;
}

void FSolidAutoPlaythrough::ApplyAimRaven(const float DeltaSeconds, ASolidCharacter& Captain)
{
	if (!bAimRavenHeld)
	{
		return;
	}

	float DesiredYaw = Captain.GetActorRotation().Yaw + RavenYawOffsetDeg;
	if (Captain.HasClickMoveDestination())
	{
		const FVector Here = Captain.GetActorLocation();
		const FVector Dest = Captain.GetClickMoveDestination();
		const FVector2D Movement = SolidClickMove::PlanarDirection(
			FVector2D(Here.X, Here.Y),
			FVector2D(Dest.X, Dest.Y));
		TrailYawDeg(Movement, RavenYawOffsetDeg, DesiredYaw);
	}

	float YawDeg = DesiredYaw;
	const float MaxDegrees = TurnDegreesPerSecond * DeltaSeconds;
	if (MaxDegrees > 0.f)
	{
		YawDeg = FMath::FixedTurn(Captain.GetControlRotation().Yaw, DesiredYaw, MaxDegrees);
	}
	Captain.AimRaven(RavenPitchDeg, YawDeg, RavenArmCm);
}

bool FSolidAutoPlaythrough::TickChooseStronghold(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	ASolidHUD* HUD = HudOf(Captain);
	if (!HUD)
	{
		return false;
	}

	if (!HUD->IsStrongholdChoiceOpen())
	{
		UWorld* World = Mode.GetWorld();
		ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
		USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
		USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
		const FVector2D Here(Captain.GetActorLocation().X, Captain.GetActorLocation().Y);
		const FSolidWorldChartMark* SiteMark = nullptr;
		FVector2D SiteXY = FVector2D::ZeroVector;
		float Best = TNumericLimits<float>::Max();
		constexpr float SiteRangeCm = 400.f;
		if (Chart && Chart->IsLoaded())
		{
			const FVector2D WorldMin = Terrain->GetWorldMinXY();
			const FVector2D WorldMax = Terrain->GetWorldMaxXY();
			for (const FSolidWorldChartMark& Mark : Chart->Marks)
			{
				if (Mark.Kind != ESolidWorldChartMarkKind::StrongholdSite)
				{
					continue;
				}
				FVector2D WorldXY = FVector2D::ZeroVector;
				if (!Chart->TerrainPointToWorld(Mark.TerrainPoint, WorldMin, WorldMax, WorldXY))
				{
					continue;
				}
				const float Dist = FVector2D::Distance(Here, WorldXY);
				if (Dist <= SiteRangeCm && Dist < Best)
				{
					Best = Dist;
					SiteMark = &Mark;
					SiteXY = WorldXY;
				}
			}
		}
		if (!SiteMark)
		{
			UE_LOG(LogSolid, Error, TEXT("Auto playthrough: no stronghold site within 4m of the captain."));
			Stop(TEXT("no stronghold site within 4m"));
			return false;
		}

		ASolidStrongholdSite* Site = nullptr;
		float BestActor = TNumericLimits<float>::Max();
		for (TActorIterator<ASolidStrongholdSite> It(World); It; ++It)
		{
			const FVector Loc = It->GetActorLocation();
			const float Dist = FVector2D::DistSquared(FVector2D(Loc.X, Loc.Y), SiteXY);
			if (Dist < BestActor)
			{
				BestActor = Dist;
				Site = *It;
			}
		}
		if (!Site)
		{
			return false;
		}
		Captain.OfferStrongholdSite(Site);
		return false;
	}

	WaitAccumulated += DeltaSeconds;
	if (WaitAccumulated < Step.PauseSeconds)
	{
		return false;
	}

	int32 SizeIndex = INDEX_NONE;
	int32 TypeIndex = INDEX_NONE;
	for (int32 Index = 0; Index < HUD->GetStrongholdChoiceCount(); ++Index)
	{
		HUD->SetStrongholdChoiceIndex(Index);
		if (HUD->GetStrongholdChoiceIndex() != Index)
		{
			continue;
		}
		if (HUD->GetStrongholdChoiceKind() == ESolidStrongholdChoiceKind::Size
			&& HUD->GetStrongholdChoiceSize() == Step.StrongholdSize)
		{
			SizeIndex = Index;
		}
		if (HUD->GetStrongholdChoiceKind() == ESolidStrongholdChoiceKind::Type
			&& HUD->GetStrongholdChoiceType() == Step.StrongholdType)
		{
			TypeIndex = Index;
		}
	}
	if (SizeIndex == INDEX_NONE || TypeIndex == INDEX_NONE)
	{
		Stop();
		return false;
	}
	HUD->SetStrongholdChoiceIndex(SizeIndex);
	HUD->SetStrongholdChoiceIndex(TypeIndex);
	if (HUD->GetStrongholdChoiceSize() != Step.StrongholdSize
		|| HUD->GetStrongholdChoiceType() != Step.StrongholdType)
	{
		Stop();
		return false;
	}
	Captain.ConfirmStrongholdChoice();
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickBuildSquad(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	if (!Step.CompanionName)
	{
		Stop();
		return false;
	}

	ASolidHUD* HUD = HudOf(Captain);
	USolidSquad* Squad = Mode.GetSquad();
	USolidCompany* Company = Mode.GetCompany();
	USolidGameState* State = Mode.GetConfiguration();
	USolidCaptain* SessionCaptain = Company ? Company->GetCaptain() : nullptr;
	if (!HUD || !Squad || !Company || !State || !SessionCaptain)
	{
		return false;
	}

	const int32 Selected = Squad->GetSelectedCompanionCount();
	int32 Companion = Company->FindCompanionIndexByName(Step.CompanionName);
	const bool bSelected = Companion != INDEX_NONE
		&& Squad->GetSelectedCompanionIndices().Contains(Companion);
	if (!bSelected)
	{
		if (Selected >= State->MaxSquadSize)
		{
			SolidEvents::FResult SizeResult;
			SolidEvents::ApplyChange(
				*State,
				*SessionCaptain,
				SolidEvents::Change::SetMaxSquadSize(Selected + 1),
				SizeResult);
		}
		if (Companion == INDEX_NONE)
		{
			SolidEvents::FResult AddResult;
			SolidEvents::ApplyChange(
				*State,
				*SessionCaptain,
				SolidEvents::Change::AddCompanion(Step.CompanionName),
				AddResult);
		}
	}

	if (!HUD->IsSquadBuilderOpen())
	{
		HUD->OpenSquadBuilder();
		return false;
	}

	Companion = Company->FindCompanionIndexByName(Step.CompanionName);
	if (Companion == INDEX_NONE)
	{
		Stop();
		return false;
	}

	const TArray<int32>& Slots = Squad->GetSelectedCompanionIndices();
	bool bAlready = false;
	int32 Empty = INDEX_NONE;
	const int32 MaxSquad = Mode.GetMaxSquadSize();
	for (int32 Slot = 0; Slot < MaxSquad; ++Slot)
	{
		const int32 Chosen = Slots.IsValidIndex(Slot) ? Slots[Slot] : INDEX_NONE;
		if (Chosen == Companion)
		{
			bAlready = true;
			break;
		}
		if (Empty == INDEX_NONE && Chosen == INDEX_NONE)
		{
			Empty = Slot;
		}
	}
	if (!bAlready)
	{
		if (Empty == INDEX_NONE)
		{
			Stop();
			return false;
		}
		SolidSquadBuilder::FDrag Drag;
		Drag.Kind = SolidSquadBuilder::EDrag::Companion;
		Drag.Index = Companion;
		Drag.Slot = INDEX_NONE;
		SolidSquadBuilder::FHit Hit;
		Hit.Kind = SolidSquadBuilder::EHit::SquadBox;
		Hit.Index = Empty;
		if (!SolidSquadBuilder::Drop(Squad, MaxSquad, Drag, Hit))
		{
			Stop();
			return false;
		}
	}

	WaitAccumulated += DeltaSeconds;
	if (WaitAccumulated < Step.PauseSeconds)
	{
		return false;
	}
	if (Step.bClosePage)
	{
		HUD->ConfirmSquadBuilder();
	}
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickPortal(
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	UWorld* World = Mode.GetWorld();
	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Streamer || !Terrain || !Terrain->IsBuilt() || !Chart || !Chart->IsLoaded()
		|| !Streamer->HasAttemptedStartTownRelocate())
	{
		NoteWait(TEXT("portal waiting for the world"));
		return false;
	}

	const FSolidWorldChartMark* Mark = Step.MarkName
		? FindChartMark(*Chart, Step.MarkKind, Step.MarkName)
		: nullptr;
	if (!Mark)
	{
		Stop(TEXT("portal mark lookup failed"));
		return false;
	}

	FVector2D WorldXY = FVector2D::ZeroVector;
	if (!Chart->TerrainPointToWorld(Mark->TerrainPoint, Terrain->GetWorldMinXY(), Terrain->GetWorldMaxXY(), WorldXY))
	{
		NoteWait(TEXT("portal mark has no world position"));
		return false;
	}
	WorldXY.X += Step.OffsetXCm;
	WorldXY.Y += Step.OffsetYCm;

	const FVector Delta = SolidPortal::SquadDelta(Captain.GetActorLocation(), WorldXY);
	const float LandZ = Streamer->GetHeightAt(FVector(WorldXY.X, WorldXY.Y, 0.f)) + Streamer->CollisionHeightBias;
	float CapsuleHalf = 96.f;
	if (const UCapsuleComponent* Capsule = Captain.GetCapsuleComponent())
	{
		CapsuleHalf = Capsule->GetScaledCapsuleHalfHeight();
	}
	const FVector NewLoc(WorldXY.X, WorldXY.Y, LandZ + CapsuleHalf + Streamer->SnapHeightPadding);
	Captain.ArriveByPortal(NewLoc, Captain.GetActorRotation().Yaw);
	Streamer->RelocateCompanionsByDelta(Delta);
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickPause(const float DeltaSeconds, const FStep& Step)
{
	WaitAccumulated += DeltaSeconds;
	if (WaitAccumulated < Step.PauseSeconds)
	{
		return false;
	}
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickEnableSight(ASolidCharacter& Captain, const FStep& Step)
{
	Captain.SetSight(Step.Sight);
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickFaceDirection(
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	const FRotator Facing(0.f, Step.FacingYawDeg, 0.f);
	Captain.SetActorRotation(Facing);
	for (const TObjectPtr<ASolidCompanionCharacter>& Companion : Mode.GetCompanions())
	{
		if (Companion)
		{
			Companion->SetActorRotation(Facing);
		}
	}
	Advance();
	return false;
}

bool FSolidAutoPlaythrough::TickTakePlayshot(ASolidCharacter& Captain)
{
	const bool bLastStep = StepIndex + 1 >= Running().StepCount;
	if (bQuitWhenFinished && bLastStep)
	{
		HoldQuitUntilPlayshotWritten();
	}
	Captain.TakePlayshot();
	Advance();
	return false;
}
