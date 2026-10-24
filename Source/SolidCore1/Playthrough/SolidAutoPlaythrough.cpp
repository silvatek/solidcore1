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
	FStep::CloseMessage(TEXT("Close the driven-out message"), SolidMessages::IdDrivenFromVillage).Pause(1.f),
	FStep::WalkTo(TEXT("Walk to the Iglin tunnels"), EWalk::LairEntrance),
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
	ResetWalk();
}

void FSolidAutoPlaythrough::Stop()
{
	bRunning = false;
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
		return TickEquipItem(Mode, Captain, Step);
	case EStepType::DisableEncounter:
		TickDisableEncounter(Mode, Step);
		return false;
	case EStepType::ExitStronghold:
		return TickExit(Mode, Captain);
	case EStepType::WakeInStronghold:
		TickWakeInStronghold(Mode);
		return false;
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
	const bool bAvoidRamps)
{
	const FVector Location = Captain.GetActorLocation();
	const FVector2D Here(Location.X, Location.Y);
	if (Captain.IsTrueSight())
	{
		const float SteerCm = bLast ? 1.f : ArriveCm;
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
	APlayerController* PlayerController = Cast<APlayerController>(Captain.GetController());
	FVector2D Screen = FVector2D::ZeroVector;
	if (PlayerController
		&& PlayerController->ProjectWorldLocationToScreen(FVector(Target.X, Target.Y, Location.Z), Screen))
	{
		Captain.SimulateLeftClick(Screen.X, Screen.Y);
	}
	return false;
}

bool FSolidAutoPlaythrough::FollowPoints(
	const float DeltaSeconds,
	const FVector2D& Here,
	FVector2D& OutTarget,
	bool& bOutFinished)
{
	bOutFinished = false;
	if (!Points.IsValidIndex(PointIndex))
	{
		bOutFinished = bWalkReady;
		return false;
	}

	const FVector2D Target = Points[PointIndex];
	const bool bLast = PointIndex + 1 >= Points.Num();
	if (SolidClickMove::HasArrived(Here, Target, ArriveCm))
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
	const SolidLair::FEntrance* Mouth = FindLairEntrance(Entrances, USolidWorldChart::XMarker);
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
	if (Step.bClosePage)
	{
		if (ASolidHUD* HUD = HudOf(Captain))
		{
			if (HUD->IsInventoryVisible())
			{
				HUD->ToggleInventory();
			}
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
	Captain.ReleaseRightMouse();
	UWorld* World = Mode.GetWorld();
	ASolidStrongholdInterior* Interior = FindInterior(World);
	UStaticMeshComponent* Mouth = Interior ? Interior->GetPassageMesh() : nullptr;
	APlayerController* PlayerController = Cast<APlayerController>(Captain.GetController());
	if (!Mouth || !PlayerController)
	{
		return false;
	}

	FVector2D Screen = FVector2D::ZeroVector;
	if (!PlayerController->ProjectWorldLocationToScreen(Mouth->GetComponentLocation(), Screen))
	{
		return false;
	}
	Captain.SimulateLeftClick(Screen.X, Screen.Y);
	const USolidGameInstance* Instance = World ? World->GetGameInstance<USolidGameInstance>() : nullptr;
	return Instance && Instance->WantsStrongholdExit();
}

bool FSolidAutoPlaythrough::TickExit(ASolidGameMode& Mode, ASolidCharacter& Captain)
{
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
