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
	/** Companions abreast in front of the Captain, facing him. */
	Parade UMETA(DisplayName = "Parade"),
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
 * AlongForward is negative behind the Captain and positive in front (Parade).
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
		case ESolidBattleFormation::Parade: return TEXT("Parade");
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

	/** True when companions in slot should turn to look at the Captain. */
	inline bool FacesCaptain(ESolidBattleFormation Formation)
	{
		return Formation == ESolidBattleFormation::Parade;
	}

	/**
	 * Face the Captain only after arriving in a formation that looks at him.
	 * While walking to the slot, companions still face their travel direction.
	 */
	inline bool ShouldFaceCaptain(
		ESolidBattleFormation Formation,
		float PlanarDistanceCm,
		float AcceptanceRadiusCm)
	{
		return FacesCaptain(Formation) && PlanarDistanceCm <= FMath::Max(AcceptanceRadiusCm, 0.f);
	}

	/** Yaw in degrees so someone at FromXY looks at ToXY. Unchanged direction returns 0. */
	inline float YawFacingPoint(FVector2D FromXY, FVector2D ToXY)
	{
		const FVector2D Delta = ToXY - FromXY;
		if (Delta.IsNearlyZero())
		{
			return 0.f;
		}
		return FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	}

	/** How quickly a companion turns to face the Captain (RInterpTo speed). */
	inline constexpr float FaceTurnInterpSpeed = 8.f;

	/** Half of the captain's horizontal view (true sight uses a 90° camera). */
	inline constexpr float CaptainHalfFovDeg = 45.f;

	/** Keep bodies inside this fraction of the view so they are not cropped at the edge. */
	inline constexpr float ParadeViewFill = 0.65f;

	/** Half-width of a companion body, past the slot center (cm). */
	inline constexpr float ParadeBodyHalfWidthCm = 90.f;

	/** Feet sit this far below the captain's eyes (capsule half-height + eye offset, cm). */
	inline constexpr float ParadeFeetBelowEyeCm = 160.f;

	/** Companions may stop this far short of the slot (matches the follow acceptance radius, cm). */
	inline constexpr float ParadeArrivalSlackCm = 90.f;

	/** Do not march the rank out toward the fog curtain (cm). */
	inline constexpr float ParadeMaxFrontCm = 1400.f;

	/** Lateral half-span of a two-companion parade rank before spacing scale (cm). */
	inline constexpr float ParadeSideCm = 160.f;

	inline float CaptainHalfVerticalFovDeg()
	{
		const float HalfH = FMath::DegreesToRadians(CaptainHalfFovDeg);
		return FMath::RadiansToDegrees(FMath::Atan(FMath::Tan(HalfH) / (16.f / 9.f)));
	}

	/**
	 * How far in front of the captain a parade rank must stand so the bodies fit
	 * in his view, including companions who stop short of the slot.
	 * A very wide rank is capped at ParadeMaxFrontCm.
	 */
	inline float ParadeStandingFrontCm(float HalfSpanCm)
	{
		const float LimitH = FMath::DegreesToRadians(CaptainHalfFovDeg * ParadeViewFill);
		const float LimitV = FMath::DegreesToRadians(CaptainHalfVerticalFovDeg() * ParadeViewFill);
		const float Outer = FMath::Max(0.f, HalfSpanCm) + ParadeBodyHalfWidthCm;
		const float HorizontalFit = Outer / FMath::Max(FMath::Tan(LimitH), 0.05f);
		const float VerticalFit = ParadeFeetBelowEyeCm / FMath::Max(FMath::Tan(LimitV), 0.05f);
		return FMath::Min(FMath::Max(HorizontalFit, VerticalFit) + ParadeArrivalSlackCm, ParadeMaxFrontCm);
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
		case ESolidBattleFormation::Parade:
		{
			float HalfSpan = (Count <= 1) ? 0.f : ParadeSideCm * Scale;
			const float LimitH = FMath::DegreesToRadians(CaptainHalfFovDeg * ParadeViewFill);
			const float UsableHalfSpan = FMath::Max(
				0.f,
				(ParadeMaxFrontCm - ParadeArrivalSlackCm) * FMath::Tan(LimitH) - ParadeBodyHalfWidthCm);
			HalfSpan = FMath::Min(HalfSpan, UsableHalfSpan);
			const float Front = ParadeStandingFrontCm(HalfSpan);
			if (Count <= 1)
			{
				return FVector2D(Front, 0.f);
			}
			const float T = (static_cast<float>(Index) / static_cast<float>(Count - 1)) * 2.f - 1.f;
			return FVector2D(Front, T * HalfSpan);
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
