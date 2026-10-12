#pragma once

#include "CoreMinimal.h"
#include "Misc/Parse.h"
#include "SolidEvents.h"
#include "SolidSight.h"

/**
 * The Meagre Dungeon interior, traveled to from the open world.
 * F10 enters and leaves. Clicking the mouth is later.
 * Only the captain makes the trip. Companions are removed with the
 * open world and spawned again when he comes back.
 */
namespace SolidCaveLevel
{
	inline const TCHAR* WorldMapPath = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");
	inline const TCHAR* OptionKey = TEXT("SolidLevel");
	inline const TCHAR* CaveValue = TEXT("Cave");

	inline const TCHAR* EnterLabel = TEXT("Enter cave");
	inline const TCHAR* LeaveLabel = TEXT("Leave cave");

	/** Open interior, cm. The mouth sits on the X=0 wall. */
	inline constexpr float RoomDepthCm = 1800.f;
	inline constexpr float RoomWidthCm = 1400.f;
	inline constexpr float RoomHeightCm = 560.f;
	inline constexpr float ShellCm = 80.f;

	/** The captain stands inside, facing the mouth. */
	inline FVector CaptainSpawnLocal()
	{
		return FVector(RoomDepthCm * 0.45f, 0.f, 0.f);
	}

	/** Local +X of the interior runs away from the mouth, so yaw 180 looks back at it. */
	inline constexpr float CaptainYawDeg = 180.f;

	/** Mouth yaw faces the grass. The captain on that grass looks back at the cliff. */
	inline float CaptainReturnYaw(const float MouthYawDeg)
	{
		return FRotator::NormalizeAxis(MouthYawDeg + 180.f);
	}

	inline bool ContainsLocal(const FVector& Point)
	{
		return Point.X > 0.f && Point.X < RoomDepthCm
			&& FMath::Abs(Point.Y) < RoomWidthCm * 0.5f
			&& Point.Z >= 0.f && Point.Z < RoomHeightCm;
	}

	/** Companions stay out of the cave. */
	inline bool BringsCompanions()
	{
		return false;
	}

	inline const TCHAR* MenuLabel(const bool bInCave)
	{
		return bInCave ? LeaveLabel : EnterLabel;
	}

	inline FString EnterOptions()
	{
		return FString::Printf(TEXT("?%s=%s"), OptionKey, CaveValue);
	}

	inline FString LeaveOptions()
	{
		return FString();
	}

	/** True when a travel URL asked for the cave interior. */
	inline bool RequestsCave(const FString& Options)
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
		return Value.Equals(CaveValue, ESearchCase::IgnoreCase);
	}
}

/** What survives the trip between the open world and the cave. */
struct FSolidTravelSession
{
	FSolidEventConfiguration Configuration;
	TArray<int32> SelectedCompanionIndices;
	int32 ActiveBattlePlanSlot = 0;
	ESolidSight Sight = ESolidSight::True;
	/** Next open-world spawn stands on the grass in front of the mouth. */
	bool bReturnToCaveMouth = false;
	/** Next loaded world is the cave interior. */
	bool bInCave = false;
};
