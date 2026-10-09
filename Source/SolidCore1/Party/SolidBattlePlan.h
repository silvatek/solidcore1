#pragma once

#include "CoreMinimal.h"
#include "SolidBattlePlan.generated.h"

/** How Companions arrange relative to the Captain while a Battle Plan is active. */
UENUM(BlueprintType)
enum class ESolidBattleFormation : uint8
{
	/** Companions abreast to the Captain's sides. */
	Line UMETA(DisplayName = "Line"),
	/** Companions in a file behind the Captain. */
	Column UMETA(DisplayName = "Column"),
	/** Companions clustered behind the Captain (triangle for two). */
	Mob UMETA(DisplayName = "Mob"),
};

/** One entry in the Company's catalog of battle plans. */
USTRUCT(BlueprintType)
struct FSolidBattlePlan
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattlePlan")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattlePlan")
	ESolidBattleFormation Formation = ESolidBattleFormation::Column;
};

/**
 * Follow-slot offsets for Companions under a formation.
 * Returns (AlongForward, AlongRight) in cm relative to the Captain.
 * AlongForward is typically negative (behind).
 */
namespace SolidBattleFormationSlots
{
	inline constexpr int32 MaxAssignedBattlePlans = 8;

	inline const TCHAR* FormationName(ESolidBattleFormation Formation)
	{
		switch (Formation)
		{
		case ESolidBattleFormation::Line: return TEXT("Line");
		case ESolidBattleFormation::Column: return TEXT("Column");
		case ESolidBattleFormation::Mob: return TEXT("Mob");
		default: return TEXT("?");
		}
	}

	inline FVector2D SlotOffset(
		ESolidBattleFormation Formation,
		int32 SlotIndex,
		int32 CompanionCount)
	{
		const int32 Count = FMath::Max(CompanionCount, 1);
		const int32 Index = FMath::Clamp(SlotIndex, 0, Count - 1);

		switch (Formation)
		{
		case ESolidBattleFormation::Line:
		{
			// Slightly behind, spread across the Captain's flanks.
			constexpr float BackCm = 40.f;
			constexpr float SideCm = 220.f;
			if (Count == 1)
			{
				return FVector2D(-BackCm, 0.f);
			}
			const float T = (static_cast<float>(Index) / static_cast<float>(Count - 1)) * 2.f - 1.f;
			return FVector2D(-BackCm, T * SideCm);
		}
		case ESolidBattleFormation::Column:
		{
			constexpr float FirstBackCm = 280.f;
			constexpr float RankSpacingCm = 240.f;
			return FVector2D(-(FirstBackCm + RankSpacingCm * Index), 0.f);
		}
		case ESolidBattleFormation::Mob:
		default:
		{
			// Triangle base behind the Captain for two Companions.
			constexpr float BackCm = 240.f;
			constexpr float SideCm = 140.f;
			if (Count == 1)
			{
				return FVector2D(-BackCm, 0.f);
			}
			const float T = (static_cast<float>(Index) / static_cast<float>(Count - 1)) * 2.f - 1.f;
			return FVector2D(-BackCm, T * SideCm);
		}
		}
	}
}
