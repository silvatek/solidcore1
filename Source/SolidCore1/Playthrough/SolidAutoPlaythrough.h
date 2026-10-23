#pragma once

#include "CoreMinimal.h"

class USolidGameState;
class ASolidGameMode;
class ASolidCharacter;

/**
 * Plays the opening from a script. The game instance keeps this object,
 * so a level load keeps the same script.
 */
class FSolidAutoPlaythrough
{
public:
	/** One line of the script. Stage 1 ends when he wakes in the cave. */
	enum class EStep : uint8
	{
		WalkToIglinRamp,
		CloseMonsterMessage,
		WakeInCave,
	};

	/** The F10 row and the HUD line. */
	static const TCHAR* Label() { return TEXT("Auto playthrough"); }

	/** No progress toward the current point for this long stops the script. */
	static constexpr float StallSeconds = 4.f;

	/** Close enough to a road point to take the next one, in centimetres. */
	static constexpr float ArriveCm = 200.f;

	/** Stage 1's list, in order. A later stage edits this list. */
	static void CollectSteps(TArray<EStep>& OutSteps);
	static const TCHAR* NameOf(EStep Step);

	/** True while the row can be chosen. The dim flag on the play state clears it. */
	static bool CanStart(const USolidGameState& State);

	/** Sets the dim flag and starts at the first step. */
	void Start(USolidGameState& State);
	/** The HUD line goes. The dim flag stays set. */
	void Stop();
	bool IsRunning() const { return bRunning; }

	/**
	 * One frame. The captain's tick calls this on the live game mode.
	 * Closing the monster message can load the cave before this returns.
	 */
	void Tick(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);

private:
	EStep CurrentStep() const;
	void Advance();
	void ResetWalk();
	void NoteDistance(float DeltaSeconds, float DistanceCm);
	bool BuildWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation);
	void TickWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	void TickMessage(ASolidCharacter& Captain);
	void TickWake(const ASolidGameMode& Mode);

	bool bRunning = false;
	int32 StepIndex = 0;
	bool bWalkReady = false;
	int32 PointIndex = 0;
	TArray<FVector2D> Points;
	float BestDistanceCm = 0.f;
	bool bHasBestDistance = false;
	float StallAccumulated = 0.f;
};
