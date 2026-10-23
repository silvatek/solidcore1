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
	/** One line of the script. Stage 2 ends when the driven-out message closes. */
	enum class EStep : uint8
	{
		WalkToMonsterEncounter,
		WakeInCave,
		CraftTorch,
		PutTorchInMainHand,
		LeaveCave,
		WalkToIglin,
		CloseDrivenOutMessage,
	};

	/** The F10 row and the HUD line. */
	static const TCHAR* Label() { return TEXT("Auto playthrough"); }

	/** No progress toward the current point for this long stops the script. */
	static constexpr float StallSeconds = 4.f;

	/** Close enough to a road point to take the next one, in centimetres. */
	static constexpr float ArriveCm = 200.f;

	/** The list in order. A later stage edits this list. */
	static void CollectSteps(TArray<EStep>& OutSteps);
	static const TCHAR* NameOf(EStep Step);

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
	EStep CurrentStep() const;
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
	/** True while true sight is dragging the view and holding W. */
	bool TickWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	void TickWake(const ASolidGameMode& Mode);
	bool TickCraft(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickPutTorch(ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickLeave(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickWalkToIglin(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickCloseMessage(ASolidCharacter& Captain);
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
};
