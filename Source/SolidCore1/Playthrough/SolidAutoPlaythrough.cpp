#include "SolidAutoPlaythrough.h"

#include "SolidCharacter.h"
#include "SolidClickMove.h"
#include "SolidGameMode.h"
#include "GameFramework/PlayerController.h"
#include "SolidGameInstance.h"
#include "SolidGameState.h"
#include "SolidGroundMove.h"
#include "Company/SolidCaptain.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Events/SolidMessages.h"
#include "HUD/SolidHUD.h"
#include "Inventory/SolidInventory.h"
#include "Lairs/SolidLair.h"
#include "Lairs/SolidLairEntrance.h"
#include "Lairs/SolidLairInterior.h"
#include "Monster/SolidEncounter.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidRoad.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldChart.h"
#include "Towns/SolidTowns.h"
#include "Towns/SolidWallgate.h"
#include "Company/SolidSquad.h"
#include "Company/SolidCompany.h"
#include "BattlePlans/SolidBattlePlan.h"
#include "Menus/SolidSquadBuilder.h"
#include "Strongholds/Sites/SolidStrongholdSite.h"
#include "Strongholds/Tower/SolidTower.h"
#include "Algo/Reverse.h"

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
	FStep::WalkTo(TEXT("Walk to the monster encounter"), EWalk::MonsterEncounter),
	FStep::WakeInStronghold(TEXT("Wake in the cave")),
	FStep::DisableEncounter(TEXT("Disable the Iglin Bridge encounter"), TEXT("Iglin Bridge")),
	FStep::WalkTo(TEXT("Walk to the stand"), StandCells, UE_ARRAY_COUNT(StandCells)),
	FStep::CraftItem(TEXT("Craft the torch"), SolidItemType::IdTorch),
	FStep::EquipItem(TEXT("Equip the torch"), SolidItemType::IdTorch, SolidInventory::ESlot::OffHand),
	FStep::EquipItem(TEXT("Equip the sword"), SolidItemType::IdRustySword, SolidInventory::ESlot::MainHand).ClosePage(),
	FStep::WalkTo(TEXT("Walk to the entrance"), EntranceCells, UE_ARRAY_COUNT(EntranceCells)),
	FStep::ExitStronghold(TEXT("Leave the cave")),
	FStep::WalkTo(TEXT("Walk to Iglin"), EWalk::TrailToIglin),
	FStep::CloseMessage(TEXT("Close the driven-out message"), SolidMessages::IdDrivenFromVillage),
	FStep::WalkTo(TEXT("Walk to the Iglin tunnels"), EWalk::LairEntrance),
	FStep::EnterLair(TEXT("Enter the Iglin tunnels"), SolidLair::IdIglinTunnels, SolidLair::Entrance),
	FStep::WalkTo(TEXT("Walk the Iglin tunnels"), EWalk::Lair, SolidLair::IdIglinTunnels, SolidLair::Exit),
	FStep::ExitLair(TEXT("Leave the Iglin tunnels"), SolidLair::IdIglinTunnels, SolidLair::Exit),
	FStep::WalkTo(TEXT("Walk to Relion"), EWalk::Town, SolidTowns::Relion),
	FStep::WalkTo(TEXT("Walk to Kanfold"), EWalk::Town, SolidTowns::Kanfold),
	FStep::AimRaven(TEXT("Aim raven high behind the squad"), HighTrailPitchDeg, HighTrailArmCm),
	FStep::SelectBattlePlan(TEXT("Form Line"), TEXT("Line")),
	FStep::SelectBattlePlan(TEXT("Form Column"), TEXT("Column")),
	FStep::SelectBattlePlan(TEXT("Form Loose mob"), TEXT("Loose mob")),
	FStep::SelectBattlePlan(TEXT("Form Tight mob"), TEXT("Tight mob")),
	FStep::WalkTo(TEXT("Walk to Dragongate"), EWalk::Town, SolidTowns::Dragongate),
	FStep::WalkTo(TEXT("Walk to the Imperious site"), EWalk::StrongholdSite, ESolidStrongholdSize::Imperious),
	FStep::ChooseStronghold(TEXT("Choose Imperious Tower"), ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower),
	FStep::WakeInStronghold(TEXT("Wake in the tower")),
	FStep::ExitStronghold(TEXT("Leave the tower")),
	FStep::BuildSquad(TEXT("Build squad Max"), TEXT("Max")),
	FStep::BuildSquad(TEXT("Build squad Morgan"), TEXT("Morgan")),
	FStep::BuildSquad(TEXT("Build squad Quinn"), TEXT("Quinn")).ClosePage(),
};

namespace
{
	constexpr float ProgressCm = 1.f;
	/** A step this far onto a ramp is refused. The walk stays on the grass. */
	constexpr float RampProbeCm = 80.f;
	/** Right-button drag, degrees per second. A hitch cannot spend more than one slice. */
	constexpr float TurnDegreesPerSecond = 180.f;
	constexpr float MaxTurnSliceSeconds = 0.05f;
	constexpr float SegmentStepCm = 50.f;
	constexpr TCHAR EncounterName[] = TEXT("Iglin Bridge");
	/** Inside one tunnel cell. The next center, a full cell away, stays ahead. */
	constexpr float LairArriveCm = SolidLair::CellCm * 0.45f;

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

	bool IsMeagreDungeon(const SolidRoad::FEndpoint& End)
	{
		if (!End.bStronghold)
		{
			return false;
		}
		FString Name = End.StrongholdName;
		Name.ReplaceInline(TEXT(" "), TEXT(""));
		return Name.Equals(TEXT("MeagreDungeon"), ESearchCase::IgnoreCase);
	}

	bool IsCaveTrail(const SolidRoad::FDefinition& Definition)
	{
		if (Definition.Type != SolidRoad::EType::Trail)
		{
			return false;
		}
		const bool bFromCave = IsMeagreDungeon(Definition.From)
			&& !Definition.To.bStronghold
			&& Definition.To.LocationId == SolidTowns::Iglin;
		const bool bToCave = IsMeagreDungeon(Definition.To)
			&& !Definition.From.bStronghold
			&& Definition.From.LocationId == SolidTowns::Iglin;
		return bFromCave || bToCave;
	}

	const SolidRoad::FRoute* FindCaveTrail(const USolidWorldChart& Chart)
	{
		for (const SolidRoad::FRoute& Route : Chart.GetRoads())
		{
			if (IsCaveTrail(Route.Definition))
			{
				return &Route;
			}
		}
		return nullptr;
	}

	bool WalkableCell(const USolidWorldChart& Chart, const FIntPoint Cell)
	{
		if (Cell.X < 0 || Cell.X >= USolidWorldChart::MapWidth
			|| Cell.Y < 0 || Cell.Y >= USolidWorldChart::MapHeight)
		{
			return false;
		}
		const ESolidBiome Biome = Chart.GetBiomeAtCell(Cell.X, Cell.Y);
		return Biome == ESolidBiome::Grassland || Biome == ESolidBiome::Town;
	}

	const SolidLair::FEntrance* FindLairEntrance(
		const TArray<SolidLair::FEntrance>& Entrances,
		const TCHAR EndMarker)
	{
		for (const SolidLair::FEntrance& Entrance : Entrances)
		{
			if (Entrance.EndMarker == EndMarker)
			{
				return &Entrance;
			}
		}
		return nullptr;
	}

	bool OnIglinRamp(const ASolidGameMode& Mode, const FVector2D& Point);

	bool GrassPath(
		const USolidWorldChart& Chart,
		const FIntPoint From,
		const FIntPoint To,
		TArray<FIntPoint>& OutCells)
	{
		OutCells.Reset();
		if (!WalkableCell(Chart, From) || !WalkableCell(Chart, To))
		{
			return false;
		}
		if (From == To)
		{
			OutCells.Add(To);
			return true;
		}

		const int32 CellCount = USolidWorldChart::MapWidth * USolidWorldChart::MapHeight;
		TArray<int32> CameFrom;
		CameFrom.Init(INDEX_NONE, CellCount);
		TArray<FIntPoint> Queue;
		Queue.Reserve(CellCount);
		const auto IndexOf = [](const FIntPoint Cell)
		{
			return Cell.X + Cell.Y * USolidWorldChart::MapWidth;
		};
		CameFrom[IndexOf(From)] = IndexOf(From);
		Queue.Add(From);
		int32 Head = 0;
		const FIntPoint Steps[] = {
			{ 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, 0 },
		};
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
				if (!WalkableCell(Chart, Next))
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
			Walk = FIntPoint(Previous % USolidWorldChart::MapWidth, Previous / USolidWorldChart::MapWidth);
		}
		OutCells.Add(From);
		Algo::Reverse(OutCells);
		return OutCells.Num() > 0;
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

	bool IsIglinRelionRoad(const SolidRoad::FDefinition& Definition)
	{
		const int32 From = Definition.From.LocationId;
		const int32 To = Definition.To.LocationId;
		return (From == SolidTowns::Iglin && To == SolidTowns::Relion)
			|| (From == SolidTowns::Relion && To == SolidTowns::Iglin);
	}

	const SolidRoad::FRoute* FindIglinRelionRoad(const USolidWorldChart& Chart)
	{
		for (const SolidRoad::FRoute& Route : Chart.GetRoads())
		{
			if (IsIglinRelionRoad(Route.Definition))
			{
				return &Route;
			}
		}
		return nullptr;
	}

	bool IsIglinBridge(const FSolidEncounter& Encounter)
	{
		return Encounter.Definition.Name
			&& FCString::Strcmp(Encounter.Definition.Name, EncounterName) == 0;
	}

	bool OnIglinRamp(const ASolidGameMode& Mode, const FVector2D& Point)
	{
		for (const TObjectPtr<ASolidBridge>& Actor : Mode.GetBridges())
		{
			const ASolidBridge* Bridge = Actor.Get();
			if (!Bridge || !Bridge->HasBridge() || !Bridge->GetBridge().bIglinRelion)
			{
				continue;
			}
			if (SolidGroundMove::OnRamp(Bridge->GetBridge(), Point.X, Point.Y))
			{
				return true;
			}
		}
		return false;
	}

	bool SegmentOnIglinRamp(const ASolidGameMode& Mode, const FVector2D& From, const FVector2D& To)
	{
		const float Length = FVector2D::Distance(From, To);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Length / SegmentStepCm));
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(Steps);
			if (OnIglinRamp(Mode, FMath::Lerp(From, To, T)))
			{
				return true;
			}
		}
		return false;
	}

	/** True when a placed Iglin Bridge member can see this ground point. */
	bool IglinBridgeDetects(const ASolidGameMode& Mode, const FVector2D& Here)
	{
		for (const FSolidEncounter& Encounter : Mode.GetEncounters())
		{
			if (!IsIglinBridge(Encounter))
			{
				continue;
			}
			for (const FSolidMonsterGroup& Group : Encounter.Groups)
			{
				for (int32 Index = 0; Index < Group.Num(); ++Index)
				{
					const FSolidMonster* Monster = Group.Get(Index);
					if (Monster && Monster->Detects(Here))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	/** The placed member closest to Iglin. That is the one the walk from town meets. */
	const FSolidMonster* ClosestIglinBridgeMonster(const ASolidGameMode& Mode, const FVector2D& IglinXY)
	{
		const FSolidMonster* Found = nullptr;
		float Best = TNumericLimits<float>::Max();
		for (const FSolidEncounter& Encounter : Mode.GetEncounters())
		{
			if (!IsIglinBridge(Encounter))
			{
				continue;
			}
			for (const FSolidMonsterGroup& Group : Encounter.Groups)
			{
				for (int32 Index = 0; Index < Group.Num(); ++Index)
				{
					const FSolidMonster* Monster = Group.Get(Index);
					if (!Monster || Monster->PointIndex == INDEX_NONE)
					{
						continue;
					}
					const float Dist = FVector2D::DistSquared(Monster->XY, IglinXY);
					if (Dist < Best)
					{
						Best = Dist;
						Found = Monster;
					}
				}
			}
		}
		return Found;
	}

	/**
	 * A point inside that monster's detection, toward Iglin, and off the ramps.
	 * Standing there starts the fight. Standing on a ramp would show the message instead.
	 */
	bool EncounterApproach(
		const ASolidGameMode& Mode,
		const FSolidMonster& Monster,
		const FVector2D& IglinXY,
		FVector2D& OutPoint)
	{
		const FVector2D TowardIglin = IglinXY - Monster.XY;
		const float Length = TowardIglin.Size();
		if (Length < 1.f || Monster.DetectionRange <= 1.f)
		{
			return false;
		}

		const FVector2D Direction = TowardIglin / Length;
		const FVector2D Perp(-Direction.Y, Direction.X);
		const float Limit = Monster.DetectionRange - 1.f;
		auto Consider = [&Mode, &Monster, Limit, &OutPoint](const FVector2D& Point)
		{
			if (FVector2D::Distance(Point, Monster.XY) > Limit || OnIglinRamp(Mode, Point))
			{
				return false;
			}
			OutPoint = Point;
			return true;
		};

		const float Preferred = FMath::Min(Monster.DetectionRange * 0.5f, Limit);
		if (Consider(Monster.XY + Direction * Preferred))
		{
			return true;
		}
		for (float Distance = Preferred; Distance <= Limit; Distance += SegmentStepCm)
		{
			if (Consider(Monster.XY + Direction * Distance))
			{
				return true;
			}
		}
		const float Sides[] = {1.f, -1.f};
		for (float Distance = Preferred; Distance <= Limit; Distance += SegmentStepCm)
		{
			for (const float Side : Sides)
			{
				for (float Across = SegmentStepCm; Across <= Limit; Across += SegmentStepCm)
				{
					if (Consider(Monster.XY + Direction * Distance + Perp * Side * Across))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	/** Slide the approach sideways until the straight line from the road stays off the ramps. */
	bool ClearOfRamp(
		const ASolidGameMode& Mode,
		const FSolidMonster& Monster,
		const FVector2D& From,
		const FVector2D& Approach,
		FVector2D& OutPoint)
	{
		if (!SegmentOnIglinRamp(Mode, From, Approach))
		{
			OutPoint = Approach;
			return true;
		}

		const FVector2D Delta = Approach - From;
		const float DeltaLength = Delta.Size();
		const FVector2D Perp = DeltaLength > 1.f
			? FVector2D(-Delta.Y, Delta.X) / DeltaLength
			: FVector2D(1.f, 0.f);
		const float Limit = Monster.DetectionRange - 1.f;
		const float Sides[] = {1.f, -1.f};
		for (const float Side : Sides)
		{
			for (float Across = 100.f; Across <= 2000.f; Across += 100.f)
			{
				const FVector2D Candidate = Approach + Perp * Side * Across;
				if (FVector2D::Distance(Candidate, Monster.XY) > Limit || OnIglinRamp(Mode, Candidate))
				{
					continue;
				}
				if (SegmentOnIglinRamp(Mode, From, Candidate))
				{
					continue;
				}
				OutPoint = Candidate;
				return true;
			}
		}
		return false;
	}

	bool IsIglinTunnels(const TCHAR* LairId)
	{
		return SolidLair::FindById(LairId) == &SolidLair::IglinTunnels;
	}

	bool LairSessionEnd(const USolidGameInstance& Instance, const TCHAR End)
	{
		return Instance.HasSession() && Instance.GetSession().LairEnd == FChar::ToUpper(End);
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
		if (!PlayerController->ProjectWorldLocationToScreen(Target->GetComponentLocation(), Screen))
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
	return !State.bAutoPlaythroughDim;
}

void FSolidAutoPlaythrough::Start(USolidGameState& State)
{
	State.bAutoPlaythroughDim = true;
	bRunning = true;
	StepIndex = 0;
	bAimRavenHeld = false;
	ResetWalk();
}

void FSolidAutoPlaythrough::Stop()
{
	bRunning = false;
	bAimRavenHeld = false;
	ResetWalk();
}

void FSolidAutoPlaythrough::Tick(const float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain)
{
	if (!bRunning)
	{
		Captain.ReleaseRightMouse();
		return;
	}

	if (StepIndex < 0 || StepIndex >= ScriptCount)
	{
		Stop();
		Captain.ReleaseRightMouse();
		return;
	}

	const bool bSteering = TickCurrent(DeltaSeconds, Mode, Captain, Script[StepIndex]);
	if (!bSteering)
	{
		Captain.ReleaseRightMouse();
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
		return TickEnterLair(Mode, Captain, Step);
	case EStepType::ExitLair:
		return TickExitLair(Mode, Captain, Step);
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
	case EStepType::Custom:
		TickCustom(Step);
		return false;
	default:
		Stop();
		return false;
	}
}

void FSolidAutoPlaythrough::Advance()
{
	++StepIndex;
	ResetWalk();
	if (StepIndex >= ScriptCount)
	{
		Stop();
	}
}

void FSolidAutoPlaythrough::ResetWalk()
{
	bWalkReady = false;
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
		Stop();
	}
}

bool FSolidAutoPlaythrough::BuildWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation)
{
	UWorld* World = Mode.GetWorld();
	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Chart || !Chart->IsLoaded() || Mode.GetBridges().Num() == 0)
	{
		return false;
	}

	const SolidRoad::FRoute* Road = FindIglinRelionRoad(*Chart);
	if (!Road || Road->Cells.Num() == 0)
	{
		return false;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	FVector2D IglinXY = FVector2D::ZeroVector;
	if (!Chart->GetLocationWorldXY(SolidTowns::Iglin, WorldMin, WorldMax, IglinXY))
	{
		return false;
	}

	const FSolidMonster* Monster = ClosestIglinBridgeMonster(Mode, IglinXY);
	FVector2D Approach = FVector2D::ZeroVector;
	if (!Monster || !EncounterApproach(Mode, *Monster, IglinXY, Approach))
	{
		return false;
	}

	TArray<FIntPoint> Cells = Road->Cells;
	if (Road->Definition.From.LocationId != SolidTowns::Iglin)
	{
		Algo::Reverse(Cells);
	}

	const FVector2D Paint = SolidRoad::StripPaintOffset();
	TArray<FVector2D> Centers;
	Centers.Reserve(Cells.Num());
	for (const FIntPoint& Cell : Cells)
	{
		FVector2D Center = FVector2D::ZeroVector;
		if (Chart->CellToWorldXY(Cell, WorldMin, WorldMax, Center))
		{
			Centers.Add(Center + Paint);
		}
	}
	if (Centers.Num() == 0)
	{
		return false;
	}

	int32 LastSafe = INDEX_NONE;
	for (int32 Index = 0; Index < Centers.Num(); ++Index)
	{
		if (OnIglinRamp(Mode, Centers[Index]))
		{
			break;
		}
		if (LastSafe != INDEX_NONE && SegmentOnIglinRamp(Mode, Centers[LastSafe], Centers[Index]))
		{
			break;
		}
		LastSafe = Index;
	}

	const FVector2D Here(CaptainLocation.X, CaptainLocation.Y);
	int32 Closest = 0;
	int32 Start = 0;
	if (LastSafe != INDEX_NONE)
	{
		float BestApproach = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index <= LastSafe; ++Index)
		{
			const float Dist = FVector2D::DistSquared(Centers[Index], Approach);
			if (Dist < BestApproach)
			{
				BestApproach = Dist;
				Closest = Index;
			}
		}

		float Nearest = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index <= Closest; ++Index)
		{
			const float Dist = FVector2D::DistSquared(Centers[Index], Here);
			if (Dist < Nearest)
			{
				Nearest = Dist;
				Start = Index;
			}
		}
	}

	Points.Reset();
	if (LastSafe != INDEX_NONE)
	{
		Points.Reserve(Closest - Start + 2);
		for (int32 Index = Start; Index <= Closest; ++Index)
		{
			Points.Add(Centers[Index]);
		}
	}

	const FVector2D From = Points.Num() > 0 ? Points.Last() : Here;
	FVector2D Final = Approach;
	ClearOfRamp(Mode, *Monster, From, Approach, Final);
	Points.Add(Final);
	PointIndex = 0;
	bWalkReady = true;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	return Points.Num() > 0;
}

bool FSolidAutoPlaythrough::TickWalk(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain)
{
	const FVector Location = Captain.GetActorLocation();
	const FVector2D Here(Location.X, Location.Y);
	if (IglinBridgeDetects(Mode, Here))
	{
		Advance();
		return false;
	}
	if (!bWalkReady && !BuildWalk(Mode, Location))
	{
		return false;
	}
	if (!Points.IsValidIndex(PointIndex))
	{
		Stop();
		return false;
	}

	const FVector2D Target = Points[PointIndex];
	const bool bLast = PointIndex + 1 >= Points.Num();
	if (!bLast && SolidClickMove::HasArrived(Here, Target, ArriveCm))
	{
		++PointIndex;
		bHasBestDistance = false;
		StallAccumulated = 0.f;
		return false;
	}

	NoteDistance(DeltaSeconds, FVector2D::Distance(Here, Target));
	if (!bRunning)
	{
		return false;
	}

	// The last point sits inside detection. Keep steering until a raptor sees him.
	return Steer(DeltaSeconds, Mode, Captain, Target, bLast, true);
}

bool FSolidAutoPlaythrough::Steer(
	const float DeltaSeconds,
	const ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FVector2D& Target,
	const bool bLast,
	const bool bAvoidRamps,
	const float ArriveRadiusCm)
{
	const FVector Location = Captain.GetActorLocation();
	const FVector2D Here(Location.X, Location.Y);
	if (Captain.IsTrueSight())
	{
		const float SteerCm = bLast ? 1.f : ArriveRadiusCm;
		const FVector2D Direction = SolidClickMove::PlanarDirection(Here, Target, SteerCm);
		if (Direction.IsNearlyZero() || (bAvoidRamps && OnIglinRamp(Mode, Here + Direction * RampProbeCm)))
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

	if (bAvoidRamps && SegmentOnIglinRamp(Mode, Here, Target))
	{
		return false;
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

bool FSolidAutoPlaythrough::BuildIglinTrail(const ASolidGameMode& Mode, const FVector& CaptainLocation)
{
	UWorld* World = Mode.GetWorld();
	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Chart || !Chart->IsLoaded())
	{
		return false;
	}

	const SolidRoad::FRoute* Road = FindCaveTrail(*Chart);
	if (!Road || Road->Cells.Num() == 0)
	{
		return false;
	}

	TArray<FIntPoint> Cells = Road->Cells;
	const bool bIglinFirst = !Road->Definition.From.bStronghold
		&& Road->Definition.From.LocationId == SolidTowns::Iglin;
	if (bIglinFirst)
	{
		Algo::Reverse(Cells);
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	const FVector2D Paint = SolidRoad::StripPaintOffset();
	TArray<FVector2D> Centers;
	Centers.Reserve(Cells.Num());
	for (const FIntPoint& Cell : Cells)
	{
		FVector2D Center = FVector2D::ZeroVector;
		if (Chart->CellToWorldXY(Cell, WorldMin, WorldMax, Center))
		{
			Centers.Add(Center + Paint);
		}
	}
	if (Centers.Num() == 0)
	{
		return false;
	}

	const FVector2D Here(CaptainLocation.X, CaptainLocation.Y);
	int32 Start = 0;
	float Nearest = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Centers.Num(); ++Index)
	{
		const float Dist = FVector2D::DistSquared(Centers[Index], Here);
		if (Dist < Nearest)
		{
			Nearest = Dist;
			Start = Index;
		}
	}

	Points.Reset();
	Points.Reserve(Centers.Num() - Start);
	for (int32 Index = Start; Index < Centers.Num(); ++Index)
	{
		Points.Add(Centers[Index]);
	}
	PointIndex = 0;
	bWalkReady = Points.Num() > 0;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	return bWalkReady;
}

bool FSolidAutoPlaythrough::BuildLairEntranceWalk(
	const ASolidGameMode& Mode,
	const FVector& CaptainLocation)
{
	UWorld* World = Mode.GetWorld();
	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Chart || !Chart->IsLoaded())
	{
		return false;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	TArray<SolidLair::FEntrance> Entrances;
	SolidLair::CollectEntrances(Chart, WorldMin, WorldMax, Entrances);
	const SolidLair::FEntrance* Mouth = FindLairEntrance(Entrances, SolidLair::Entrance);
	if (!Mouth)
	{
		return false;
	}

	FIntPoint Start = FIntPoint::ZeroValue;
	if (!Chart->CellAtWorld(CaptainLocation.X, CaptainLocation.Y, WorldMin, WorldMax, Start))
	{
		return false;
	}

	TArray<FIntPoint> Cells;
	if (!GrassPath(*Chart, Start, Mouth->GrassCell, Cells))
	{
		Stop();
		return false;
	}

	Points.Reset();
	Points.Reserve(Cells.Num() + 1);
	for (const FIntPoint& Cell : Cells)
	{
		FVector2D Center = FVector2D::ZeroVector;
		if (Chart->CellToWorldXY(Cell, WorldMin, WorldMax, Center))
		{
			Points.Add(Center);
		}
	}
	Points.Add(Mouth->WorldXY);
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

bool FSolidAutoPlaythrough::TownRoads(
	const TArray<SolidRoad::FRoute>& Routes,
	const int32 FromTown,
	const int32 ToTown,
	TArray<FIntPoint>& OutCells)
{
	OutCells.Reset();
	if (FromTown == INDEX_NONE || ToTown == INDEX_NONE || FromTown == ToTown)
	{
		return false;
	}

	const auto TownEnd = [](const SolidRoad::FEndpoint& End)
	{
		return End.bStronghold || End.LocationId == INDEX_NONE ? INDEX_NONE : End.LocationId;
	};

	struct FLink
	{
		int32 Town = INDEX_NONE;
		int32 Route = INDEX_NONE;
	};
	TMap<int32, TArray<FLink>> Links;
	for (int32 Index = 0; Index < Routes.Num(); ++Index)
	{
		const SolidRoad::FRoute& Route = Routes[Index];
		const int32 From = TownEnd(Route.Definition.From);
		const int32 To = TownEnd(Route.Definition.To);
		if (Route.Cells.Num() == 0 || From == INDEX_NONE || To == INDEX_NONE || From == To)
		{
			continue;
		}
		FLink Forward;
		Forward.Town = To;
		Forward.Route = Index;
		Links.FindOrAdd(From).Add(Forward);
		FLink Back;
		Back.Town = From;
		Back.Route = Index;
		Links.FindOrAdd(To).Add(Back);
	}

	TMap<int32, int32> CameTown;
	TMap<int32, int32> CameRoute;
	TArray<int32> Queue;
	CameTown.Add(FromTown, FromTown);
	Queue.Add(FromTown);
	int32 Head = 0;
	while (Head < Queue.Num() && !CameTown.Contains(ToTown))
	{
		const int32 Here = Queue[Head++];
		const TArray<FLink>* Neighbors = Links.Find(Here);
		if (!Neighbors)
		{
			continue;
		}
		for (const FLink& Link : *Neighbors)
		{
			if (CameTown.Contains(Link.Town))
			{
				continue;
			}
			CameTown.Add(Link.Town, Here);
			CameRoute.Add(Link.Town, Link.Route);
			Queue.Add(Link.Town);
		}
	}
	if (!CameRoute.Contains(ToTown))
	{
		return false;
	}

	TArray<int32> Chain;
	for (int32 Town = ToTown; Town != FromTown;)
	{
		const int32* Route = CameRoute.Find(Town);
		const int32* Previous = CameTown.Find(Town);
		if (!Route || !Previous || *Previous == Town)
		{
			return false;
		}
		Chain.Add(*Route);
		Town = *Previous;
	}
	Algo::Reverse(Chain);

	int32 Leave = FromTown;
	for (const int32 RouteIndex : Chain)
	{
		const SolidRoad::FRoute& Route = Routes[RouteIndex];
		TArray<FIntPoint> Cells = Route.Cells;
		const int32 From = TownEnd(Route.Definition.From);
		const int32 To = TownEnd(Route.Definition.To);
		if (From != Leave)
		{
			Algo::Reverse(Cells);
		}
		int32 Start = 0;
		if (OutCells.Num() > 0 && Cells.Num() > 0 && Cells[0] == OutCells.Last())
		{
			Start = 1;
		}
		for (int32 Index = Start; Index < Cells.Num(); ++Index)
		{
			OutCells.Add(Cells[Index]);
		}
		Leave = From == Leave ? To : From;
	}
	return OutCells.Num() > 0;
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

bool FSolidAutoPlaythrough::StrongholdSiteCell(
	const USolidWorldChart& Chart,
	const ESolidStrongholdSize Size,
	FIntPoint& OutCell)
{
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Column = 0; Column < USolidWorldChart::MapWidth; ++Column)
		{
			ESolidStrongholdSize Found = ESolidStrongholdSize::Meagre;
			if (Chart.StrongholdSiteAtCell(Column, Row, Found) && Found == Size)
			{
				OutCell = FIntPoint(Column, Row);
				return true;
			}
		}
	}
	return false;
}

bool FSolidAutoPlaythrough::WalkPointForCell(
	const USolidWorldChart& Chart,
	const FIntPoint& Cell,
	const FVector2D& WorldMinXY,
	const FVector2D& WorldMaxXY,
	FVector2D& OutPoint)
{
	const int32 TownId = Chart.GetLocationIdAtCell(Cell.X, Cell.Y);
	if (TownId != INDEX_NONE
		&& SolidWallgate::GateCenterXY(&Chart, TownId, WorldMinXY, WorldMaxXY, OutPoint))
	{
		return true;
	}
	return Chart.CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, OutPoint);
}

bool FSolidAutoPlaythrough::TowerWalkPoints(
	const SolidStronghold::FSolidTower& Tower,
	const int32 FromFloor,
	const int32 ToFloor,
	TArray<FVector2D>& OutPoints)
{
	OutPoints.Reset();
	const int32 Floors = Tower.FloorCount();
	if (FromFloor < 1 || ToFloor < 1 || FromFloor > Floors || ToFloor > Floors)
	{
		return false;
	}

	TArray<FVector2D> Up;
	Up.Add(Tower.DoorHexCenter());
	const SolidStronghold::FPlan& Plan = Tower.Plan();
	for (int32 Code = 0; Code < Floors - 1; ++Code)
	{
		const SolidStronghold::FWedge Flight = SolidStronghold::FlightWedge(Plan, Code);
		if (!Flight.bOpen)
		{
			continue;
		}
		const float Radius = Tower.InteriorRadiusCm(Tower.FloorZCm(Code)) - Plan.StairWidthCm * 0.5f;
		if (Radius <= 0.f)
		{
			continue;
		}
		for (int32 Tread = 0; Tread < Plan.StairTreadCount; ++Tread)
		{
			const float Along =
				(static_cast<float>(Tread) + 0.5f) / static_cast<float>(Plan.StairTreadCount);
			const float Angle = Flight.StartRad + Along * Flight.SweepRad;
			Up.Add(FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius));
		}
		const SolidStronghold::FWedge Landing = SolidStronghold::LandingWedge(Plan, Code + 1);
		if (Landing.bOpen)
		{
			const float LandingRadius =
				Tower.InteriorRadiusCm(Tower.FloorZCm(Code + 1)) - Plan.StairWidthCm * 0.5f;
			const float Angle = Landing.StartRad + Landing.SweepRad * 0.5f;
			Up.Add(FVector2D(FMath::Cos(Angle) * LandingRadius, FMath::Sin(Angle) * LandingRadius));
		}
	}

	if (Up.Num() == 0)
	{
		return false;
	}
	if (ToFloor >= FromFloor)
	{
		OutPoints = MoveTemp(Up);
	}
	else
	{
		Algo::Reverse(Up);
		OutPoints = MoveTemp(Up);
	}
	return OutPoints.Num() > 0;
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

bool FSolidAutoPlaythrough::BuildTownWalk(
	const ASolidGameMode& Mode,
	const FVector& CaptainLocation,
	const FStep& Step)
{
	if (!SolidTowns::Find(Step.TownId))
	{
		Stop();
		return false;
	}

	UWorld* World = Mode.GetWorld();
	const USolidGameInstance* Instance = World ? World->GetGameInstance<USolidGameInstance>() : nullptr;
	if (Instance && Instance->WantsLairMouthSpawn())
	{
		return false;
	}

	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Chart || !Chart->IsLoaded())
	{
		return false;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	FIntPoint HereCell = FIntPoint::ZeroValue;
	if (!Chart->CellAtWorld(CaptainLocation.X, CaptainLocation.Y, WorldMin, WorldMax, HereCell))
	{
		return false;
	}

	TArray<FIntPoint> Cells;
	bool bPaint = false;
	int32 OccupiedTown = INDEX_NONE;
	if (USolidCompany* Company = Mode.GetCompany())
	{
		if (const USolidCaptain* Record = Company->GetCaptain())
		{
			OccupiedTown = Record->OccupiedTown;
		}
	}
	const int32 HereTown = TownWalkFrom(
		Chart->GetLocationIdAtCell(HereCell.X, HereCell.Y),
		OccupiedTown);
	if (HereTown != INDEX_NONE && HereTown != Step.TownId)
	{
		if (!TownRoads(Chart->GetRoads(), HereTown, Step.TownId, Cells))
		{
			Stop();
			return false;
		}
		bPaint = true;
		int32 Start = 0;
		int32 Nearest = TNumericLimits<int32>::Max();
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			const int32 Dist = FMath::Abs(Cells[Index].X - HereCell.X) + FMath::Abs(Cells[Index].Y - HereCell.Y);
			if (Dist < Nearest)
			{
				Nearest = Dist;
				Start = Index;
			}
		}
		if (Start > 0)
		{
			Cells.RemoveAt(0, Start);
		}
	}
	else
	{
		FSolidWorldLocation Location;
		if (!Chart->FindLocation(Step.TownId, Location) || Location.Cells.Num() == 0)
		{
			Stop();
			return false;
		}
		int32 Best = 0;
		int32 Nearest = TNumericLimits<int32>::Max();
		for (int32 Index = 0; Index < Location.Cells.Num(); ++Index)
		{
			const FIntPoint& Cell = Location.Cells[Index];
			const int32 Dist = FMath::Abs(Cell.X - HereCell.X) + FMath::Abs(Cell.Y - HereCell.Y);
			if (Dist < Nearest)
			{
				Nearest = Dist;
				Best = Index;
			}
		}
		if (!GrassPath(*Chart, HereCell, Location.Cells[Best], Cells))
		{
			Stop();
			return false;
		}
	}

	const FVector2D Paint = bPaint ? SolidRoad::StripPaintOffset() : FVector2D::ZeroVector;
	FVector2D Gate = FVector2D::ZeroVector;
	const bool bGate = SolidWallgate::GateCenterXY(Chart, Step.TownId, WorldMin, WorldMax, Gate);
	Points.Reset();
	Points.Reserve(Cells.Num());
	bool bAddedGate = false;
	for (const FIntPoint& Cell : Cells)
	{
		if (bGate && Chart->GetLocationIdAtCell(Cell.X, Cell.Y) == Step.TownId)
		{
			if (!bAddedGate)
			{
				Points.Add(Gate);
				bAddedGate = true;
			}
			continue;
		}
		FVector2D Center = FVector2D::ZeroVector;
		if (Chart->CellToWorldXY(Cell, WorldMin, WorldMax, Center))
		{
			Points.Add(Center + Paint);
		}
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
	case EWalk::MonsterEncounter:
		return TickWalk(DeltaSeconds, Mode, Captain);
	case EWalk::Hexes:
		return TickHexWalk(DeltaSeconds, Mode, Captain, Step);
	case EWalk::TrailToIglin:
		return TickWalkToIglin(DeltaSeconds, Mode, Captain);
	case EWalk::LairEntrance:
		return TickWalkToLairEntrance(DeltaSeconds, Mode, Captain);
	case EWalk::Lair:
		return TickWalkLair(DeltaSeconds, Mode, Captain, Step);
	case EWalk::Town:
		return TickWalkTown(DeltaSeconds, Mode, Captain, Step);
	case EWalk::StrongholdSite:
		return TickWalkSite(DeltaSeconds, Mode, Captain, Step);
	case EWalk::Tower:
		return TickWalkTower(DeltaSeconds, Mode, Captain, Step);
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
		return Steer(DeltaSeconds, Mode, Captain, Target, false, false);
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
		if (!CraftTorch(State->Inventory, State->CaveChest))
		{
			Stop();
			return false;
		}
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
	if (!EquipItem(State->Inventory, &State->CaveChest, Step.ItemId, Step.Slot))
	{
		Stop();
		return false;
	}
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
	if (!Captain.IsTrueSight())
	{
		Captain.SetSight(ESolidSight::True);
	}

	UWorld* World = Mode.GetWorld();
	const ASolidStrongholdInterior* Interior = FindInterior(World);
	const UStaticMeshComponent* Passage = Interior ? Interior->GetPassageMesh() : nullptr;
	if (!Passage)
	{
		return false;
	}

	const FVector Here = Captain.GetActorLocation();
	const FVector At = Passage->GetComponentLocation();
	float Yaw = 0.f;
	if (!TrailYawDeg(FVector2D(At.X - Here.X, At.Y - Here.Y), 0.f, Yaw))
	{
		return false;
	}

	const float YawError = FMath::FindDeltaAngleDegrees(Captain.GetControlRotation().Yaw, Yaw);
	if (FMath::Abs(YawError) >= 0.05f)
	{
		const float MaxDegrees = TurnDegreesPerSecond * FMath::Min(DeltaSeconds, MaxTurnSliceSeconds);
		Captain.DragRightMouse(Yaw, MaxDegrees);
		return true;
	}

	if (ClickMouth(Mode, Captain))
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickWalkToIglin(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain)
{
	USolidCompany* Company = Mode.GetCompany();
	const USolidCaptain* Record = Company ? Company->GetCaptain() : nullptr;
	if (Record && Record->OccupiedTown == SolidTowns::Iglin)
	{
		Advance();
		return false;
	}

	const FVector Location = Captain.GetActorLocation();
	if (!bWalkReady && !BuildIglinTrail(Mode, Location))
	{
		return false;
	}

	const FVector2D Here(Location.X, Location.Y);
	FVector2D Target = FVector2D::ZeroVector;
	bool bFinished = false;
	if (FollowPoints(DeltaSeconds, Here, Target, bFinished))
	{
		return Steer(DeltaSeconds, Mode, Captain, Target, false, false);
	}
	return false;
}

bool FSolidAutoPlaythrough::TickWalkToLairEntrance(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain)
{
	const FVector Location = Captain.GetActorLocation();
	if (!bWalkReady && !BuildLairEntranceWalk(Mode, Location))
	{
		return false;
	}

	const FVector2D Here(Location.X, Location.Y);
	FVector2D Target = FVector2D::ZeroVector;
	bool bFinished = false;
	if (FollowPoints(DeltaSeconds, Here, Target, bFinished))
	{
		return Steer(DeltaSeconds, Mode, Captain, Target, false, false);
	}
	if (bRunning && bFinished)
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
		return Steer(DeltaSeconds, Mode, Captain, Target, false, false, LairArriveCm);
	}
	if (bRunning && bFinished)
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickWalkTown(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	USolidCompany* Company = Mode.GetCompany();
	const USolidCaptain* Record = Company ? Company->GetCaptain() : nullptr;
	if (Step.TownId != INDEX_NONE && Record && Record->OccupiedTown == Step.TownId)
	{
		Advance();
		return false;
	}

	const FVector Location = Captain.GetActorLocation();
	if (!bWalkReady && !BuildTownWalk(Mode, Location, Step))
	{
		return false;
	}

	const FVector2D Here(Location.X, Location.Y);
	FVector2D Target = FVector2D::ZeroVector;
	bool bFinished = false;
	if (FollowPoints(DeltaSeconds, Here, Target, bFinished))
	{
		return Steer(DeltaSeconds, Mode, Captain, Target, false, false);
	}
	return false;
}

bool FSolidAutoPlaythrough::TickEnterLair(
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	if (!IsIglinTunnels(Step.LairId) || !SolidLair::IsEndMarker(Step.EndMarker))
	{
		Stop();
		return false;
	}

	UWorld* World = Mode.GetWorld();
	const ASolidLairEntrance* Entrance = FindMouthActor(World, Step.EndMarker);
	const UStaticMeshComponent* Mouth = Entrance ? Entrance->GetMouthMesh() : nullptr;
	if (!ClickMesh(Captain, Mouth))
	{
		return false;
	}

	const USolidGameInstance* Instance = World ? World->GetGameInstance<USolidGameInstance>() : nullptr;
	if (Instance && Instance->IsInLair() && LairSessionEnd(*Instance, Step.EndMarker))
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickExitLair(
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

	UWorld* World = Mode.GetWorld();
	const ASolidLairInterior* Interior = FindLairInterior(World);
	const UStaticMeshComponent* Mouth = Interior ? Interior->GetEndMouth(Step.EndMarker) : nullptr;
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
		const int32 Slot = BattlePlanSlot(*Squad, Step.PlanName);
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

bool FSolidAutoPlaythrough::BuildSiteWalk(
	const ASolidGameMode& Mode,
	const FVector& CaptainLocation,
	const FStep& Step)
{
	UWorld* World = Mode.GetWorld();
	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Chart || !Chart->IsLoaded())
	{
		return false;
	}

	FIntPoint Site = FIntPoint::ZeroValue;
	if (!StrongholdSiteCell(*Chart, Step.StrongholdSize, Site))
	{
		Stop();
		return false;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	FIntPoint HereCell = FIntPoint::ZeroValue;
	if (!Chart->CellAtWorld(CaptainLocation.X, CaptainLocation.Y, WorldMin, WorldMax, HereCell))
	{
		return false;
	}

	TArray<FIntPoint> Cells;
	if (!GrassPath(*Chart, HereCell, Site, Cells))
	{
		Stop();
		return false;
	}

	Points.Reset();
	Points.Reserve(Cells.Num());
	for (const FIntPoint& Cell : Cells)
	{
		FVector2D Point = FVector2D::ZeroVector;
		if (!WalkPointForCell(*Chart, Cell, WorldMin, WorldMax, Point))
		{
			continue;
		}
		if (Points.Num() > 0 && Points.Last().Equals(Point, 1.f))
		{
			continue;
		}
		Points.Add(Point);
	}

	const FVector2D Here(CaptainLocation.X, CaptainLocation.Y);
	int32 Start = 0;
	float Nearest = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const float Dist = FVector2D::DistSquared(Points[Index], Here);
		if (Dist < Nearest)
		{
			Nearest = Dist;
			Start = Index;
		}
	}
	if (Start > 0)
	{
		Points.RemoveAt(0, Start);
	}

	PointIndex = 0;
	bWalkReady = Points.Num() > 0;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	return bWalkReady;
}

bool FSolidAutoPlaythrough::BuildTowerWalk(
	const ASolidGameMode& Mode,
	const FVector& CaptainLocation,
	const FStep& Step)
{
	if (Step.Floor < 1)
	{
		Stop();
		return false;
	}
	if (!Mode.IsInStronghold() || !Mode.HasPlacedCaptainInStronghold())
	{
		return false;
	}

	USolidCompany* Company = Mode.GetCompany();
	const FSolidStronghold* Hold = Company ? Company->GetStronghold() : nullptr;
	if (!Hold || Hold->Type != ESolidStrongholdType::Tower)
	{
		return false;
	}
	const SolidStronghold::FSolidTower& Tower = static_cast<const SolidStronghold::FSolidTower&>(*Hold);
	ASolidStrongholdInterior* Interior = FindInterior(Mode.GetWorld());
	if (!Interior)
	{
		return false;
	}

	TArray<FVector2D> Local;
	const int32 FromFloor = Step.Floor > 1 ? 1 : Tower.FloorCount();
	if (!TowerWalkPoints(Tower, FromFloor, Step.Floor, Local))
	{
		Stop();
		return false;
	}

	Points.Reset();
	Points.Reserve(Local.Num());
	const FTransform& ActorXf = Interior->GetActorTransform();
	for (const FVector2D& Point : Local)
	{
		const FVector World = ActorXf.TransformPosition(FVector(Point.X, Point.Y, 0.f));
		Points.Add(FVector2D(World.X, World.Y));
	}

	const FVector2D Here(CaptainLocation.X, CaptainLocation.Y);
	int32 Start = 0;
	float Nearest = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const float Dist = FVector2D::DistSquared(Points[Index], Here);
		if (Dist < Nearest)
		{
			Nearest = Dist;
			Start = Index;
		}
	}
	if (Start > 0)
	{
		Points.RemoveAt(0, Start);
	}

	PointIndex = 0;
	bWalkReady = Points.Num() > 0;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	return bWalkReady;
}

bool FSolidAutoPlaythrough::TickWalkSite(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	const FVector Location = Captain.GetActorLocation();
	if (!bWalkReady && !BuildSiteWalk(Mode, Location, Step))
	{
		return false;
	}

	const FVector2D Here(Location.X, Location.Y);
	FVector2D Target = FVector2D::ZeroVector;
	bool bFinished = false;
	if (FollowPoints(DeltaSeconds, Here, Target, bFinished))
	{
		return Steer(DeltaSeconds, Mode, Captain, Target, false, false);
	}
	if (bRunning && bFinished)
	{
		Advance();
	}
	return false;
}

bool FSolidAutoPlaythrough::TickWalkTower(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain,
	const FStep& Step)
{
	const FVector Location = Captain.GetActorLocation();
	if (!bWalkReady && !BuildTowerWalk(Mode, Location, Step))
	{
		return false;
	}

	const FVector2D Here(Location.X, Location.Y);
	FVector2D Target = FVector2D::ZeroVector;
	bool bFinished = false;
	if (FollowPoints(DeltaSeconds, Here, Target, bFinished, TowerArriveCm))
	{
		return Steer(DeltaSeconds, Mode, Captain, Target, false, false, TowerArriveCm);
	}
	if (bRunning && bFinished)
	{
		Advance();
	}
	return false;
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
		FIntPoint SiteCell = FIntPoint::ZeroValue;
		if (!Chart || !StrongholdSiteCell(*Chart, Step.StrongholdSize, SiteCell))
		{
			Stop();
			return false;
		}

		ASolidStrongholdSite* Site = nullptr;
		for (TActorIterator<ASolidStrongholdSite> It(World); It; ++It)
		{
			if (It->GetCell() == SiteCell)
			{
				Site = *It;
				break;
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

	int32 Choice = INDEX_NONE;
	for (int32 Index = 0; Index < HUD->GetStrongholdChoiceCount(); ++Index)
	{
		HUD->SetStrongholdChoiceIndex(Index);
		if (HUD->GetStrongholdChoiceSize() == Step.StrongholdSize
			&& HUD->GetStrongholdChoiceType() == Step.StrongholdType)
		{
			Choice = Index;
			break;
		}
	}
	if (Choice == INDEX_NONE)
	{
		Stop();
		return false;
	}
	HUD->SetStrongholdChoiceIndex(Choice);
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
	if (!HUD || !Squad || !Company)
	{
		return false;
	}

	if (!HUD->IsSquadBuilderOpen())
	{
		if (SolidSquadBuilder::CanOpen(Mode.GetMaxSquadSize())
			&& SolidSquadBuilder::HasAnythingToArrange(Squad, Mode.GetMaxSquadSize()))
		{
			HUD->OpenSquadBuilder();
		}
		return false;
	}

	const int32 Companion = Company->FindCompanionIndexByName(Step.CompanionName);
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
