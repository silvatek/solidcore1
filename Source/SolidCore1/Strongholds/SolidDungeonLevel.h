#pragma once

#include "CoreMinimal.h"
#include "Misc/Parse.h"
#include "SolidGameState.h"
#include "SolidSight.h"
#include "SolidStrongholdGrid.h"

/**
 * Dungeon interior, traveled to from the open world.
 * Every dungeon size uses this trip. The Meagre size is the cave.
 * F10 enters and leaves. A left click on the dark opening does the same.
 * Only the captain makes the trip. Companions are removed with the
 * open world and spawned again when he comes back.
 */
namespace SolidDungeonLevel
{
	inline const TCHAR* WorldMapPath = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");
	inline const TCHAR* OptionKey = TEXT("SolidLevel");
	inline const TCHAR* DungeonValue = TEXT("Dungeon");

	inline const TCHAR* EnterLabel = TEXT("Enter cave");
	inline const TCHAR* LeaveLabel = TEXT("Leave cave");

	/** Meagre den ceiling. The floor plan is the fourteen-hex outline in SolidStrongholdGrid. */
	inline constexpr float RoomHeightCm = 320.f;
	inline constexpr float ShellCm = 48.f;
	/** Dark opening on the entrance hex. The cliff mouth outside is larger. */
	inline constexpr float MouthHeightCm = 240.f;
	/**
	 * Clear width of the cave door. Wider than the captain, and wider than one hex edge,
	 * so the vestibule is not pinched shut by the wall ends.
	 */
	inline constexpr float DoorwayWidthCm = 140.f;

	/** Feet on the cell inside the entrance. The visit view turns him into the den. */
	inline FVector CaptainSpawnLocal()
	{
		const SolidStrongholdGrid::FCell Cell = SolidStrongholdGrid::MeagreCaptainCell();
		const FVector2D Center = SolidStrongholdGrid::HexCenter(
			Cell.Col, Cell.Row, SolidStrongholdGrid::MeagreOrigin());
		return FVector(Center.X, Center.Y, 0.f);
	}

	/** Local +X runs away from the mouth, into the den. Yaw 180 would look back at the mouth. */

	inline bool ContainsLocal(const FVector& Point)
	{
		if (Point.Z < 0.f || Point.Z >= RoomHeightCm)
		{
			return false;
		}
		TArray<SolidStrongholdGrid::FHex> Hexes;
		SolidStrongholdGrid::CollectMeagreHexes(Hexes);
		const FVector2D Plan(Point.X, Point.Y);
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			if (SolidStrongholdGrid::PointInHex(Plan, Hex.Center))
			{
				return true;
			}
		}
		return false;
	}

	/** A step west of the mouth, outside the den. */
	inline FVector PastMouthLocal()
	{
		const FVector2D Mouth = SolidStrongholdGrid::MeagreMouthMidpoint();
		return FVector(Mouth.X - SolidStrongholdGrid::FlatToFlatCm * 0.5f, Mouth.Y, 100.f);
	}

	inline const TCHAR* MenuLabel(const bool bInDungeon)
	{
		return bInDungeon ? LeaveLabel : EnterLabel;
	}

	inline FString EnterOptions()
	{
		return FString::Printf(TEXT("?%s=%s"), OptionKey, DungeonValue);
	}

	inline FString LeaveOptions()
	{
		return FString();
	}

	/** True when a travel URL asked for the dungeon interior. */
	inline bool RequestsDungeon(const FString& Options)
	{
		FString Value;
		if (!FParse::Value(*Options, *FString::Printf(TEXT("%s="), OptionKey), Value))
		{
			return false;
		}
		int32 Query = INDEX_NONE;
		if (Value.FindChar(TEXT('?'), Query))
		{
			Value.LeftInline(Query);
		}
		return Value.Equals(DungeonValue, ESearchCase::IgnoreCase);
	}
}

/** What survives the trip between the open world and a closed visit. */
struct FSolidTravelSession
{
	FGameState Configuration;
	TArray<int32> SelectedCompanionIndices;
	int32 ActiveBattlePlanSlot = 0;
	ESolidSight Sight = ESolidSight::True;
	/** Next open-world spawn stands on the grass in front of the mouth. */
	bool bReturnToDungeonMouth = false;
	/** Next loaded world is the dungeon interior. */
	bool bInDungeon = false;
	/** Next loaded world is the Iglin tunnels. */
	bool bInLair = false;
	/** Next open-world spawn stands on the grass in front of one tunnel mouth. */
	bool bReturnToLairMouth = false;
	/** X or Y. The end to enter, or the end to stand at after leaving. */
	TCHAR LairEnd = 0;
};
