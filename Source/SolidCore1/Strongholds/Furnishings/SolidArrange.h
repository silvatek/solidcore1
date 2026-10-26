#pragma once

#include "CoreMinimal.h"
#include "SolidStrongholdFurniture.h"
#include "SolidStrongholdGrid.h"

/**
 * Arrange mode inside a company stronghold.
 * The mode lifts one piece on the captain's floor, turns it, and puts it down.
 * A legal put-down writes that piece's cell and facing onto the stronghold's list.
 */
class FSolidArrange
{
public:
	struct FContext
	{
		/** The company stronghold. The open world and the tunnels are not. */
		bool bInStronghold = false;
		/** A menu, the inventory, the journal, a message, or the squad builder. */
		bool bOverlay = false;
	};

	/** One facing. Six of these return a piece to the facing it started with. */
	static constexpr float FacingDeg = 60.f;

	/** Where a held piece is drawn, in the interior's local space. */
	struct FDraw
	{
		FVector Local = FVector::ZeroVector;
		float Yaw = 0.f;
		bool bGhost = false;
	};

	/** The floor plane under a pointer ray, in the interior's local space. */
	struct FTrace
	{
		bool bPlane = false;
		bool bCell = false;
		int32 X = 0;
		int32 Y = 0;
		FVector Point = FVector::ZeroVector;
	};

	enum class EConfirm : uint8
	{
		None,
		Lifted,
		Placed,
		Refused,
	};

	bool IsOn() const { return bOn; }
	bool IsHolding() const { return Held != INDEX_NONE; }
	int32 HeldIndex() const { return Held; }
	float FloorZ() const { return FloorHeight; }
	int32 PieceCount() const { return Pieces.Num(); }

	/**
	 * `\` or the sign.
	 * Turning on needs the stronghold and a clear screen.
	 * Turning off puts a held piece back.
	 * An overlay leaves the mode as it is.
	 */
	bool Toggle(const FContext& Context);

	/** Esc with nothing held, or leaving the stronghold. A held piece goes back first. */
	void Leave();

	/**
	 * The captain's floor for this visit.
	 * Cells already omit stairs, landings, and gaps.
	 * The entrance and the captain's cell are blocked when the flags say so.
	 */
	void Open(
		const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& InPieces,
		const TArray<SolidStrongholdGrid::FHex>& InCells,
		const SolidStrongholdGrid::FCell& InEntrance,
		bool bInHasEntrance,
		const SolidStrongholdGrid::FCell& InCaptain,
		bool bInHasCaptain,
		float InFloorZ);

	void ClearHover();

	/** A cell on this floor. Any other cell clears the hover. */
	void SetHover(int32 X, int32 Y);

	/** Catalog name of the piece on the hover cell. Empty when the cell is bare. */
	const TCHAR* HoverName() const;

	/** Hexes the pointer marks. Empty when the pointer is off the floor. */
	void HighlightHexes(TArray<SolidStrongholdGrid::FHex>& Out) const;

	/**
	 * Left click or Enter.
	 * A piece lifts. A legal ghost cell puts it down. An illegal cell keeps it held.
	 * An empty cell with nothing held does nothing.
	 */
	EConfirm Confirm();

	/**
	 * After a legal put-down, write that piece's cell and facing onto the stronghold list.
	 * The entry it lifted is the one that changes. A put-back is not written.
	 */
	bool WritePlaced(TArray<SolidStrongholdFurniture::FPlacedFurnishing>& Furnishings) const;

	/** Q and wheel up are positive. E and wheel down are negative. Nothing held does nothing. */
	void Turn(int32 Steps);

	/** One facing per wheel notch. Wheel up is positive, the same way as Q. */
	static int32 WheelSteps(float Axis);

	/** Esc while a piece is held. The mode stays on. */
	bool PutBack();

	const SolidStrongholdFurniture::FPlacedFurnishing& PieceAt(int32 Index) const;

	/** Local pose of one piece. A held piece uses the ghost, so its old cells stay empty. */
	bool DrawOf(int32 Index, FDraw& Out) const;

	/**
	 * True when both cells are on this floor, clear of the entrance, the captain, and other pieces.
	 * A bed fails when either cell fails. The ghost still exists when this is false.
	 */
	bool CanPlace(const SolidStrongholdFurniture::FPlacedFurnishing& Candidate) const;

	/** Intersect the floor plane. A miss of every cell is off the floor, a stair, or a landing. */
	bool Trace(const FVector& LocalOrigin, const FVector& LocalDirection, float MaxDistanceCm, FTrace& Out) const;

private:
	bool Blocked(int32 X, int32 Y) const;
	int32 PieceOnCell(int32 X, int32 Y) const;

	bool bOn = false;
	TArray<SolidStrongholdFurniture::FPlacedFurnishing> Pieces;
	TArray<SolidStrongholdGrid::FHex> Cells;
	SolidStrongholdGrid::FCell Entrance;
	bool bHasEntrance = false;
	SolidStrongholdGrid::FCell Captain;
	bool bHasCaptain = false;
	float FloorHeight = 0.f;
	bool bHover = false;
	int32 HoverX = 0;
	int32 HoverY = 0;
	int32 Held = INDEX_NONE;
	/** The piece Confirm last put down. INDEX_NONE after a lift, a refusal, or a put-back. */
	int32 LastPlaced = INDEX_NONE;
	SolidStrongholdFurniture::FPlacedFurnishing Home;
	SolidStrongholdFurniture::FPlacedFurnishing GhostUsed;
};
