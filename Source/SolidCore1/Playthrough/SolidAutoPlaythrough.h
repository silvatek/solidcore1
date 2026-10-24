#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidInventory.h"
#include "Terrain/SolidRoad.h"

class USolidGameState;
class USolidSquad;
class ASolidGameMode;
class ASolidCharacter;

namespace SolidStorage
{
	struct FContainer;
}

namespace SolidStrongholdGrid
{
	struct FCell;
}

/**
 * Plays the opening from a script. The game instance keeps this object,
 * so a level load keeps the same script.
 */
class FSolidAutoPlaythrough
{
public:
	/** What a script row does. The fields on that row are the ones this type reads. */
	enum class EStepType : uint8
	{
		WalkTo,
		CloseMessage,
		CraftItem,
		EquipItem,
		DisableEncounter,
		ExitStronghold,
		EnterLair,
		ExitLair,
		WakeInStronghold,
		SelectBattlePlan,
		AimRaven,
		Custom,
	};

	/** The route a WalkTo row follows. */
	enum class EWalk : uint8
	{
		MonsterEncounter,
		Hexes,
		TrailToIglin,
		LairEntrance,
		Lair,
		Town,
	};

	/** A CloseMessage row waits this long after the message is showing. */
	static constexpr float MessagePauseSeconds = 1.f;

	/**
	 * One row of the opening script.
	 * WalkTo, CloseMessage, CraftItem, EquipItem, DisableEncounter, ExitStronghold,
	 * EnterLair, ExitLair, WakeInStronghold, SelectBattlePlan, AimRaven, and Custom build one.
	 * A later helper fills the one field that row also reads.
	 */
	struct FStep
	{
		EStepType Type = EStepType::Custom;
		const TCHAR* Name = TEXT("");
		EWalk Walk = EWalk::Hexes;
		const SolidStrongholdGrid::FCell* Cells = nullptr;
		int32 CellCount = 0;
		const TCHAR* ItemId = nullptr;
		SolidInventory::ESlot Slot = SolidInventory::ESlot::MainHand;
		const TCHAR* EncounterName = nullptr;
		/** Eight hex digits. EnterLair, ExitLair, and a lair walk read this. */
		const TCHAR* LairId = nullptr;
		/** The Iglin Tunnels Entrance or Exit. */
		TCHAR EndMarker = 0;
		/** Location id. A Town walk ends when OccupiedTown is this id. */
		int32 TownId = INDEX_NONE;
		/** True when this row closes the inventory page. */
		bool bClosePage = false;
		const TCHAR* MessageId = nullptr;
		/** Seconds to wait after the matching message is showing, then close. */
		float PauseSeconds = 0.f;
		/** Assigned battle plan name. SelectBattlePlan reads this. */
		const TCHAR* PlanName = nullptr;
		/** Raven look, degrees. Negative looks down. */
		float PitchDeg = 0.f;
		/** Raven boom length, centimetres. */
		float ArmCm = 0.f;
		/** Added to the captain's yaw. Zero is behind him. */
		float YawOffsetDeg = 0.f;
		/** Which custom row this is. The custom switch matches it. */
		int32 CustomId = INDEX_NONE;

		static constexpr FStep WalkTo(const TCHAR* InName, const EWalk InWalk)
		{
			FStep Step;
			Step.Type = EStepType::WalkTo;
			Step.Name = InName;
			Step.Walk = InWalk;
			return Step;
		}

		/** A town walk. InTown is the location id. */
		static constexpr FStep WalkTo(const TCHAR* InName, const EWalk InWalk, const int32 InTown)
		{
			FStep Step;
			Step.Type = EStepType::WalkTo;
			Step.Name = InName;
			Step.Walk = InWalk;
			Step.TownId = InTown;
			return Step;
		}

		/** A lair walk. InEnd is the Iglin Tunnels Entrance or Exit. */
		static constexpr FStep WalkTo(
			const TCHAR* InName,
			const EWalk InWalk,
			const TCHAR* InLairId,
			const TCHAR InEnd)
		{
			FStep Step;
			Step.Type = EStepType::WalkTo;
			Step.Name = InName;
			Step.Walk = InWalk;
			Step.LairId = InLairId;
			Step.EndMarker = InEnd;
			return Step;
		}

		static constexpr FStep WalkTo(
			const TCHAR* InName,
			const SolidStrongholdGrid::FCell* InCells,
			const int32 InCount)
		{
			FStep Step;
			Step.Type = EStepType::WalkTo;
			Step.Name = InName;
			Step.Walk = EWalk::Hexes;
			Step.Cells = InCells;
			Step.CellCount = InCount;
			return Step;
		}

		static constexpr FStep CloseMessage(const TCHAR* InName, const TCHAR* InMessageId)
		{
			FStep Step;
			Step.Type = EStepType::CloseMessage;
			Step.Name = InName;
			Step.MessageId = InMessageId;
			Step.PauseSeconds = MessagePauseSeconds;
			return Step;
		}

		static constexpr FStep CraftItem(const TCHAR* InName, const TCHAR* InItemId)
		{
			FStep Step;
			Step.Type = EStepType::CraftItem;
			Step.Name = InName;
			Step.ItemId = InItemId;
			return Step;
		}

		static constexpr FStep EquipItem(
			const TCHAR* InName,
			const TCHAR* InItemId,
			const SolidInventory::ESlot InSlot)
		{
			FStep Step;
			Step.Type = EStepType::EquipItem;
			Step.Name = InName;
			Step.ItemId = InItemId;
			Step.Slot = InSlot;
			return Step;
		}

		static constexpr FStep DisableEncounter(const TCHAR* InName, const TCHAR* InEncounterName)
		{
			FStep Step;
			Step.Type = EStepType::DisableEncounter;
			Step.Name = InName;
			Step.EncounterName = InEncounterName;
			return Step;
		}

		static constexpr FStep ExitStronghold(const TCHAR* InName)
		{
			FStep Step;
			Step.Type = EStepType::ExitStronghold;
			Step.Name = InName;
			return Step;
		}

		/** Click that lair's mouth on the grass. InEnd is the Iglin Tunnels Entrance or Exit. */
		static constexpr FStep EnterLair(const TCHAR* InName, const TCHAR* InLairId, const TCHAR InEnd)
		{
			FStep Step;
			Step.Type = EStepType::EnterLair;
			Step.Name = InName;
			Step.LairId = InLairId;
			Step.EndMarker = InEnd;
			return Step;
		}

		/** Click that mouth inside the lair. InEnd is the Iglin Tunnels Entrance or Exit. */
		static constexpr FStep ExitLair(const TCHAR* InName, const TCHAR* InLairId, const TCHAR InEnd)
		{
			FStep Step;
			Step.Type = EStepType::ExitLair;
			Step.Name = InName;
			Step.LairId = InLairId;
			Step.EndMarker = InEnd;
			return Step;
		}

		static constexpr FStep WakeInStronghold(const TCHAR* InName)
		{
			FStep Step;
			Step.Type = EStepType::WakeInStronghold;
			Step.Name = InName;
			return Step;
		}

		static constexpr FStep SelectBattlePlan(const TCHAR* InName, const TCHAR* InPlanName)
		{
			FStep Step;
			Step.Type = EStepType::SelectBattlePlan;
			Step.Name = InName;
			Step.PlanName = InPlanName;
			return Step;
		}

		/** Pitch and boom for raven sight. Yaw stays behind the captain. */
		static constexpr FStep AimRaven(const TCHAR* InName, const float InPitchDeg, const float InArmCm)
		{
			FStep Step;
			Step.Type = EStepType::AimRaven;
			Step.Name = InName;
			Step.PitchDeg = InPitchDeg;
			Step.ArmCm = InArmCm;
			return Step;
		}

		static constexpr FStep Custom(const TCHAR* InName, const int32 InCustomId)
		{
			FStep Step;
			Step.Type = EStepType::Custom;
			Step.Name = InName;
			Step.CustomId = InCustomId;
			return Step;
		}

		constexpr FStep ClosePage() const
		{
			FStep Step = *this;
			Step.bClosePage = true;
			return Step;
		}

		constexpr FStep Pause(const float Seconds) const
		{
			FStep Step = *this;
			Step.PauseSeconds = Seconds;
			return Step;
		}
	};

	/** The opening script, in order. A later story edits this list. */
	static const FStep Script[];
	static constexpr int32 ScriptCount = 23;

	/** High behind the squad. AimRaven at Kanfold uses these. */
	static constexpr float HighTrailPitchDeg = -48.f;
	static constexpr float HighTrailArmCm = 2200.f;

	/** The F10 row and the HUD line. */
	static const TCHAR* Label() { return TEXT("Auto playthrough"); }

	/** No progress toward the current point for this long stops the script. */
	static constexpr float StallSeconds = 4.f;

	/** Close enough to a road point to take the next one, in centimetres. */
	static constexpr float ArriveCm = 200.f;

	/**
	 * One stick and one pitch become a torch in the pack. The leftovers go back to the chest.
	 * The sword, the boots, the coins, and the body are left alone.
	 */
	static bool CraftTorch(SolidInventory::FLoadout& Loadout, SolidStorage::FContainer& Chest);
	/**
	 * Open cells from one cell to another on this lair's plan, including both.
	 * The shipped plan is the Iglin tunnels. An unknown id has no path.
	 */
	static bool LairCells(const TCHAR* LairId, FIntPoint From, FIntPoint To, TArray<FIntPoint>& OutCells);
	/** The same path between the Iglin Tunnels Entrance and Exit. */
	static bool LairCells(const TCHAR* LairId, TCHAR FromEnd, TCHAR ToEnd, TArray<FIntPoint>& OutCells);

	/**
	 * Map cells along the chart routes from one town to another, including both ends.
	 * The chain is the fewest routes. A stronghold end is left out. False when no chain exists.
	 */
	static bool TownRoads(
		const TArray<SolidRoad::FRoute>& Routes,
		int32 FromTown,
		int32 ToTown,
		TArray<FIntPoint>& OutCells);
	/** The town a road walk starts from. A grass cell uses OccupiedTown. */
	static int32 TownWalkFrom(const int32 CellTown, const int32 OccupiedTown)
	{
		return CellTown != INDEX_NONE ? CellTown : OccupiedTown;
	}

	/** The assigned key for that plan name. INDEX_NONE when it is not on a key. */
	static int32 BattlePlanSlot(const USolidSquad& Squad, const TCHAR* PlanName);

	/** Move that item from a worn box, the pack, or the chest into the worn slot. */
	static bool EquipItem(
		SolidInventory::FLoadout& Loadout,
		SolidStorage::FContainer* Chest,
		const TCHAR* ItemId,
		SolidInventory::ESlot Slot);

	/** True while the row can be chosen. The dim flag on the play state clears it. */
	static bool CanStart(const USolidGameState& State);

	/** Sets the dim flag and starts at the first step. */
	void Start(USolidGameState& State);
	/** The HUD line goes. The dim flag stays set. */
	void Stop();
	bool IsRunning() const { return bRunning; }

	/**
	 * One frame. The captain's tick calls this on the live game mode.
	 * The battle's death can load the cave before a later frame.
	 */
	void Tick(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);

private:
	/** Moves to the next row of Script. The list ending stops the script. */
	void Advance();
	void ResetWalk();
	void NoteDistance(float DeltaSeconds, float DistanceCm);
	bool BuildWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation);
	bool BuildHexWalk(const ASolidGameMode& Mode, const SolidStrongholdGrid::FCell* Cells, int32 Count);
	bool BuildIglinTrail(const ASolidGameMode& Mode, const FVector& CaptainLocation);
	bool BuildLairEntranceWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation);
	bool BuildLairWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation, const FStep& Step);
	bool BuildTownWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation, const FStep& Step);
	/**
	 * Steps along Points. True when OutTarget is the point to steer toward.
	 * bOutFinished is set once the last point is within arrival and the stall clock is clear.
	 */
	bool FollowPoints(
		float DeltaSeconds,
		const FVector2D& Here,
		FVector2D& OutTarget,
		bool& bOutFinished,
		float ArriveRadiusCm = ArriveCm);
	/** True while true sight is dragging the view and holding W. Raven sight walks toward the point. */
	bool Steer(
		float DeltaSeconds,
		const ASolidGameMode& Mode,
		ASolidCharacter& Captain,
		const FVector2D& Target,
		bool bLast,
		bool bAvoidRamps,
		float ArriveRadiusCm = ArriveCm);
	bool TickCurrent(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickWalkTo(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	/** True while true sight is dragging the view and holding W. */
	bool TickWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickHexWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickWalkToIglin(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickWalkToLairEntrance(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickWalkLair(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickWalkTown(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickEnterLair(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickExitLair(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	void TickCustom(const FStep& Step);
	void TickWakeInStronghold(const ASolidGameMode& Mode);
	bool TickCraftItem(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickEquipItem(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	void TickDisableEncounter(ASolidGameMode& Mode, const FStep& Step);
	bool TickExit(ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickCloseMessage(float DeltaSeconds, ASolidCharacter& Captain, const FStep& Step);
	bool TickSelectBattlePlan(float DeltaSeconds, ASolidGameMode& Mode, const FStep& Step);
	bool TickAimRaven(float DeltaSeconds, ASolidCharacter& Captain, const FStep& Step);
	bool ClickMouth(ASolidGameMode& Mode, ASolidCharacter& Captain);

	bool bRunning = false;
	int32 StepIndex = 0;
	bool bWalkReady = false;
	/** The inventory page is open, so the next tick crafts. */
	bool bPageOpen = false;
	int32 PointIndex = 0;
	TArray<FVector2D> Points;
	float BestDistanceCm = 0.f;
	bool bHasBestDistance = false;
	float StallAccumulated = 0.f;
	/** Time the matching message has been showing on a CloseMessage row. */
	float WaitAccumulated = 0.f;
};
