#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidInventory.h"
#include "Sight/SolidSight.h"
#include "Strongholds/SolidStronghold.h"
#include "Terrain/SolidWorldChartMark.h"

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
		MoveTo,
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
		ChooseStronghold,
		BuildSquad,
		PortalToMarker,
		Pause,
		EnableSight,
		FaceDirection,
		TakePlayshot,
		Custom,
	};

	/** The route a WalkTo or MoveTo row follows. */
	enum class EWalk : uint8
	{
		/** The row names a chart mark. The chart's copy of that mark is the place. */
		Marker,
		Hexes,
		Lair,
	};

	/** A hold on a message, the inventory page, a camera aim, or a battle plan. */
	static constexpr float DefaultPauseSeconds = 1.f;
	/** A CloseMessage row waits this long after the message is showing. */
	static constexpr float MessagePauseSeconds = DefaultPauseSeconds;

	/**
	 * One row of the opening script.
	 * WalkTo, MoveTo, CloseMessage, CraftItem, EquipItem, DisableEncounter, ExitStronghold,
	 * EnterLair, ExitLair, WakeInStronghold, SelectBattlePlan, AimRaven,
	 * ChooseStronghold, BuildSquad, PortalToMarker, Pause, EnableSight,
	 * FaceDirection, TakePlayshot, and Custom build one.
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
		/** True when this row closes the inventory page. */
		bool bClosePage = false;
		const TCHAR* MessageId = nullptr;
		/** Seconds to wait before this row advances. ClosePage waits, then closes. */
		float PauseSeconds = 0.f;
		/** Assigned battle plan name. SelectBattlePlan reads this. */
		const TCHAR* PlanName = nullptr;
		/** Raven look, degrees. Negative looks down. */
		float PitchDeg = 0.f;
		/** Raven boom length, centimetres. */
		float ArmCm = 0.f;
		/** Added to the walk yaw. Zero looks along the movement. */
		float YawOffsetDeg = 0.f;
		/** Which custom row this is. The custom switch matches it. */
		int32 CustomId = INDEX_NONE;
		/** Site size. ChooseStronghold reads this. */
		ESolidStrongholdSize StrongholdSize = ESolidStrongholdSize::Meagre;
		/** Built type. ChooseStronghold reads this. */
		ESolidStrongholdType StrongholdType = ESolidStrongholdType::Dungeon;
		/** Company companion name. BuildSquad reads this. */
		const TCHAR* CompanionName = nullptr;
		/** Kind of the chart mark this walk goes to. WalkToMarker sets it. */
		ESolidWorldChartMarkKind MarkKind = ESolidWorldChartMarkKind::Town;
		/** Name of that mark. The chart's mark of this kind and name supplies the place. */
		const TCHAR* MarkName = nullptr;
		/** Added to the mark, in centimetres. South is negative Y. PortalToMarker reads these. */
		float OffsetXCm = 0.f;
		float OffsetYCm = 0.f;
		/** Sight mode. EnableSight reads this. */
		ESolidSight Sight = ESolidSight::True;
		/** Absolute facing, degrees. FaceDirection reads this. */
		float FacingYawDeg = 0.f;

		/** A walk to the chart mark of this kind and name. Summary is the row label. */
		static constexpr FStep WalkToMarker(
			const TCHAR* Summary,
			const ESolidWorldChartMarkKind Kind,
			const TCHAR* Name)
		{
			FStep Step;
			Step.Type = EStepType::WalkTo;
			Step.Name = Summary;
			Step.Walk = EWalk::Marker;
			Step.MarkKind = Kind;
			Step.MarkName = Name;
			return Step;
		}

		/** An interior move along stronghold hexes. */
		static constexpr FStep MoveTo(
			const TCHAR* InName,
			const SolidStrongholdGrid::FCell* InCells,
			const int32 InCount)
		{
			FStep Step;
			Step.Type = EStepType::MoveTo;
			Step.Name = InName;
			Step.Walk = EWalk::Hexes;
			Step.Cells = InCells;
			Step.CellCount = InCount;
			return Step;
		}

		/** An interior lair move. */
		static constexpr FStep MoveTo(
			const TCHAR* InName,
			const EWalk InWalk,
			const TCHAR* InLairId,
			const TCHAR InEnd)
		{
			FStep Step;
			Step.Type = EStepType::MoveTo;
			Step.Name = InName;
			Step.Walk = InWalk;
			Step.LairId = InLairId;
			Step.EndMarker = InEnd;
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
			Step.PauseSeconds = DefaultPauseSeconds;
			return Step;
		}

		/** Pitch and boom for raven sight. Yaw stays behind the walk. */
		static constexpr FStep AimRaven(const TCHAR* InName, const float InPitchDeg, const float InArmCm)
		{
			FStep Step;
			Step.Type = EStepType::AimRaven;
			Step.Name = InName;
			Step.PitchDeg = InPitchDeg;
			Step.ArmCm = InArmCm;
			Step.PauseSeconds = DefaultPauseSeconds;
			return Step;
		}

		static constexpr FStep ChooseStronghold(
			const TCHAR* InName,
			const ESolidStrongholdSize InSize,
			const ESolidStrongholdType InType)
		{
			FStep Step;
			Step.Type = EStepType::ChooseStronghold;
			Step.Name = InName;
			Step.StrongholdSize = InSize;
			Step.StrongholdType = InType;
			Step.PauseSeconds = DefaultPauseSeconds;
			return Step;
		}

		static constexpr FStep BuildSquad(const TCHAR* InName, const TCHAR* InCompanionName)
		{
			FStep Step;
			Step.Type = EStepType::BuildSquad;
			Step.Name = InName;
			Step.CompanionName = InCompanionName;
			Step.PauseSeconds = DefaultPauseSeconds;
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
			if (Step.PauseSeconds <= 0.f)
			{
				Step.PauseSeconds = DefaultPauseSeconds;
			}
			return Step;
		}

		constexpr FStep Pause(const float Seconds) const
		{
			FStep Step = *this;
			Step.PauseSeconds = Seconds;
			return Step;
		}

		/** A portal to the chart mark of this kind and name, then the X and Y offsets. */
		static constexpr FStep PortalToMarker(
			const TCHAR* Summary,
			const ESolidWorldChartMarkKind Kind,
			const TCHAR* Name,
			const float OffsetXCm,
			const float OffsetYCm)
		{
			FStep Step;
			Step.Type = EStepType::PortalToMarker;
			Step.Name = Summary;
			Step.MarkKind = Kind;
			Step.MarkName = Name;
			Step.OffsetXCm = OffsetXCm;
			Step.OffsetYCm = OffsetYCm;
			return Step;
		}

		/** A wait, then the next row. Name is the row label. */
		static constexpr FStep Pause(const float Seconds, const TCHAR* Name)
		{
			FStep Step;
			Step.Type = EStepType::Pause;
			Step.Name = Name;
			Step.PauseSeconds = Seconds;
			return Step;
		}

		static constexpr FStep EnableSight(const TCHAR* Summary, const ESolidSight Sight)
		{
			FStep Step;
			Step.Type = EStepType::EnableSight;
			Step.Name = Summary;
			Step.Sight = Sight;
			return Step;
		}

		static constexpr FStep FaceDirection(const TCHAR* Summary, const float YawDeg)
		{
			FStep Step;
			Step.Type = EStepType::FaceDirection;
			Step.Name = Summary;
			Step.FacingYawDeg = YawDeg;
			return Step;
		}

		static constexpr FStep TakePlayshot(const TCHAR* Summary)
		{
			FStep Step;
			Step.Type = EStepType::TakePlayshot;
			Step.Name = Summary;
			return Step;
		}
	};

	/**
	 * One named script. Steps is that list.
	 * bOnMenu puts the name on the F10 menu. bDisableAfterMove refuses a later start once the captain has moved.
	 */
	struct FScript
	{
		const TCHAR* Name = TEXT("");
		const FStep* Steps = nullptr;
		int32 StepCount = 0;
		bool bOnMenu = false;
		bool bDisableAfterMove = false;
	};

	/** Named scripts. The runner still walks Script, which is Full-playthrough. */
	static const FScript Scripts[];
	static constexpr int32 ScriptsCount = 2;

	/** Full-playthrough's steps, in order. */
	static const FStep Script[];
	static constexpr int32 ScriptCount = 30;

	/** Companion parade's steps, in order. */
	static const FStep Parade[];
	static constexpr int32 ParadeCount = 7;

	/** High behind the squad. AimRaven at Kanfold uses these. */
	static constexpr float HighTrailPitchDeg = -48.f;
	static constexpr float HighTrailArmCm = 2200.f;

	/** The F10 row and the HUD line. */
	static const TCHAR* Label() { return TEXT("Auto playthrough"); }

	/** No progress toward the current point for this long stops the script. */
	static constexpr float StallSeconds = 4.f;

	/** Close enough to a point to take the next one, in centimetres. */
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
	 * Yaw that looks along Movement, then adds the AimRaven offset.
	 * False when Movement is zero; OutYaw stays.
	 */
	static bool TrailYawDeg(const FVector2D& Movement, float YawOffsetDeg, float& OutYaw);

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
	/** The HUD line goes. The dim flag stays set. Reason is written to the log. */
	void Stop(const TCHAR* Reason = nullptr);
	bool IsRunning() const { return bRunning; }

	/**
	 * One frame. The captain's tick calls this on the live game mode.
	 * The battle's death can load the cave before a later frame.
	 */
	void Tick(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);

private:
	/** Moves to the next row of Script. The list ending stops the script. Reason is written to the log. */
	void Advance(const TCHAR* Reason = nullptr);
	/** Logs the current row once. A later row logs again. */
	void NoteStepStart();
	/** Logs a wait reason once, and again when the reason changes. */
	void NoteWait(const TCHAR* Reason);
	/** Logs the row NoteStepStart logged. A second call logs nothing. */
	void NoteStepEnd();
	void ResetWalk();
	void NoteDistance(float DeltaSeconds, float DistanceCm);
	bool BuildWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation, const FStep& Step);
	bool BuildHexWalk(const ASolidGameMode& Mode, const SolidStrongholdGrid::FCell* Cells, int32 Count);
	bool BuildLairWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation, const FStep& Step);
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
		ASolidCharacter& Captain,
		const FVector2D& Target,
		bool bLast,
		float ArriveRadiusCm = ArriveCm);
	bool TickCurrent(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickWalkTo(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	/** True while true sight is dragging the view and holding W. */
	bool TickWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickHexWalk(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickWalkLair(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickEnterLair(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickExitLair(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	void TickCustom(const FStep& Step);
	void TickWakeInStronghold(const ASolidGameMode& Mode);
	bool TickCraftItem(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickEquipItem(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	void TickDisableEncounter(ASolidGameMode& Mode, const FStep& Step);
	bool TickExit(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain);
	bool TickCloseMessage(float DeltaSeconds, ASolidCharacter& Captain, const FStep& Step);
	bool TickSelectBattlePlan(float DeltaSeconds, ASolidGameMode& Mode, const FStep& Step);
	bool TickAimRaven(float DeltaSeconds, const FStep& Step);
	void ApplyAimRaven(float DeltaSeconds, ASolidCharacter& Captain);
	bool TickChooseStronghold(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickBuildSquad(float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickPortal(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickPause(float DeltaSeconds, const FStep& Step);
	bool TickEnableSight(ASolidCharacter& Captain, const FStep& Step);
	bool TickFaceDirection(ASolidGameMode& Mode, ASolidCharacter& Captain, const FStep& Step);
	bool TickTakePlayshot(ASolidCharacter& Captain);
	bool ClickMouth(ASolidGameMode& Mode, ASolidCharacter& Captain);

	bool bRunning = false;
	int32 StepIndex = 0;
	/** Row whose start has been logged. INDEX_NONE after its end is logged. */
	int32 LoggedStep = INDEX_NONE;
	/** Last wait reason logged for the current walk. */
	const TCHAR* WaitReason = nullptr;
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
	/** AimRaven set these. Later ticks keep that look behind the walk. */
	bool bAimRavenHeld = false;
	float RavenPitchDeg = 0.f;
	float RavenArmCm = 0.f;
	float RavenYawOffsetDeg = 0.f;
};
