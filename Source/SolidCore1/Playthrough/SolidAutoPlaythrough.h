#pragma once

#include "CoreMinimal.h"

class USolidGameState;
class ASolidGameMode;
class ASolidCharacter;

namespace SolidInventory
{
	struct FLoadout;
}

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
		ExitStronghold,
		WakeInStronghold,
		Custom,
	};

	/** The route a WalkTo row follows. */
	enum class EWalk : uint8
	{
		MonsterEncounter,
		Hexes,
		TrailToIglin,
	};

	/** A CloseMessage row waits this long after the message is showing. */
	static constexpr float MessagePauseSeconds = 1.f;

	/**
	 * One row of the opening script.
	 * WalkTo, CloseMessage, CraftItem, ExitStronghold, WakeInStronghold, and Custom build one.
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
		/** True when this row moves the crafted item into the main hand. */
		bool bEquipMainHand = false;
		/** True when this row closes the inventory page. */
		bool bClosePage = false;
		const TCHAR* MessageId = nullptr;
		/** Seconds to wait after the matching message is showing, then close. */
		float PauseSeconds = 0.f;
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

		static constexpr FStep ExitStronghold(const TCHAR* InName)
		{
			FStep Step;
			Step.Type = EStepType::ExitStronghold;
			Step.Name = InName;
			return Step;
		}

		static constexpr FStep WakeInStronghold(const TCHAR* InName)
		{
			FStep Step;
			Step.Type = EStepType::WakeInStronghold;
			Step.Name = InName;
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

		constexpr FStep EquipMainHand() const
		{
			FStep Step = *this;
			Step.bEquipMainHand = true;
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
	static constexpr int32 ScriptCount = 9;

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
	/** The torch in pack cell (0, 0) moves to the main hand. */
	static bool PutTorchInMainHand(SolidInventory::FLoadout& Loadout);

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
	/**
	 * Steps along Points. True when OutTarget is the point to steer toward.
	 * bOutFinished is set once the last point is within arrival and the stall clock is clear.
	 */
	bool FollowPoints(float DeltaSeconds, const FVector2D& Here, FVector2D& OutTarget, bool& bOutFinished);
	/** True while true sight is dragging the view and holding W. Raven sight clicks the point. */
	bool Steer(
		float DeltaSeconds,
		const ASolidGameMode& Mode,
		ASolidCharacter& Captain,
		const FVector2D& Target,
		bool bLast,
		bool bAvoidRamps);
	bool TickCurrent(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickWalkTo(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	/** True while true sight is dragging the view and holding W. */
	bool TickWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickHexWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickWalkToIglin(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	void TickCustom(const FStep& Step);
	void TickWakeInStronghold(const ASolidGameMode& Mode);
	bool TickCraftItem(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickPutTorch(ASolidGameMode& Mode, ASolidCharacter& Captain, bool bClosePage);
	bool TickExit(ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickCloseMessage(float DeltaSeconds, ASolidCharacter& Captain, const FStep& Step);
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
