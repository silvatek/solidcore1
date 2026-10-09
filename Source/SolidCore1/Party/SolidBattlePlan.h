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

/** How far Companions keep from the Captain and each other. */
UENUM(BlueprintType)
enum class ESolidBattleSpacing : uint8
{
	Narrow UMETA(DisplayName = "Narrow"),
	Standard UMETA(DisplayName = "Standard"),
	Wide UMETA(DisplayName = "Wide"),
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BattlePlan")
	ESolidBattleSpacing Spacing = ESolidBattleSpacing::Standard;
};

/**
 * Follow-slot offsets for Companions under a formation + spacing.
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

	inline const TCHAR* SpacingName(ESolidBattleSpacing Spacing)
	{
		switch (Spacing)
		{
		case ESolidBattleSpacing::Narrow: return TEXT("Narrow");
		case ESolidBattleSpacing::Standard: return TEXT("Standard");
		case ESolidBattleSpacing::Wide: return TEXT("Wide");
		default: return TEXT("?");
		}
	}

	/** Multiplier applied to base formation offsets. */
	inline float SpacingScale(ESolidBattleSpacing Spacing)
	{
		switch (Spacing)
		{
		case ESolidBattleSpacing::Narrow: return 0.65f;
		case ESolidBattleSpacing::Wide: return 1.45f;
		case ESolidBattleSpacing::Standard:
		default: return 1.f;
		}
	}

	inline FVector2D SlotOffset(
		ESolidBattleFormation Formation,
		int32 SlotIndex,
		int32 CompanionCount,
		ESolidBattleSpacing Spacing = ESolidBattleSpacing::Standard)
	{
		const int32 Count = FMath::Max(CompanionCount, 1);
		const int32 Index = FMath::Clamp(SlotIndex, 0, Count - 1);
		const float Scale = SpacingScale(Spacing);

		FVector2D Base = FVector2D::ZeroVector;
		switch (Formation)
		{
		case ESolidBattleFormation::Line:
		{
			// Slightly behind, spread across the Captain's flanks.
			constexpr float BackCm = 40.f;
			constexpr float SideCm = 220.f;
			if (Count == 1)
			{
				Base = FVector2D(-BackCm, 0.f);
			}
			else
			{
				const float T = (static_cast<float>(Index) / static_cast<float>(Count - 1)) * 2.f - 1.f;
				Base = FVector2D(-BackCm, T * SideCm);
			}
			break;
		}
		case ESolidBattleFormation::Column:
		{
			constexpr float FirstBackCm = 280.f;
			constexpr float RankSpacingCm = 240.f;
			Base = FVector2D(-(FirstBackCm + RankSpacingCm * Index), 0.f);
			break;
		}
		case ESolidBattleFormation::Mob:
		default:
		{
			// Triangle base behind the Captain for two Companions.
			constexpr float BackCm = 240.f;
			constexpr float SideCm = 140.f;
			if (Count == 1)
			{
				Base = FVector2D(-BackCm, 0.f);
			}
			else
			{
				const float T = (static_cast<float>(Index) / static_cast<float>(Count - 1)) * 2.f - 1.f;
				Base = FVector2D(-BackCm, T * SideCm);
			}
			break;
		}
		}

		return Base * Scale;
	}
}
