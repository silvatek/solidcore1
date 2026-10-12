#pragma once

#include "CoreMinimal.h"
#include "SolidDungeonEntrance.h"
#include "SolidDungeonInterior.h"
#include "SolidDungeonLevel.h"

class UPrimitiveComponent;

/**
 * Left click on the dark cave opening. Outside, that click enters.
 * Inside, it leaves. The stone frame is not the opening.
 * Either sight can click. Click-to-move stays raven sight only.
 * The menu, the formation drill, and right-button look hide the pointer and ignore the click.
 */
namespace SolidDungeonPassage
{
	enum class EHit : uint8
	{
		None,
		Enter,
		Leave,
	};

	/** The pointer is up, and no menu or drill is using it. */
	inline bool AcceptsPointer(const bool bPointerShown, const bool bMenuOrDrill)
	{
		return bPointerShown && !bMenuOrDrill;
	}

	/**
	 * A visibility trace misses this engine-cube mouth, including a ray that
	 * starts inside it. The click uses the mouth component's bounds instead.
	 */
	inline bool RayHitsBox(
		const FVector& Origin,
		const FVector& Direction,
		const FBox& Box,
		const float MaxDistanceCm)
	{
		const bool bHasExtent = Box.Min.X <= Box.Max.X && Box.Min.Y <= Box.Max.Y && Box.Min.Z <= Box.Max.Z;
		if (!bHasExtent)
		{
			return false;
		}
		if (Box.IsInsideOrOn(Origin))
		{
			return true;
		}

		const FVector Dir = Direction.GetSafeNormal();
		if (Dir.IsNearlyZero() || MaxDistanceCm <= 0.f)
		{
			return false;
		}

		const float OriginAxis[3] = { Origin.X, Origin.Y, Origin.Z };
		const float DirAxis[3] = { Dir.X, Dir.Y, Dir.Z };
		const float MinAxis[3] = { Box.Min.X, Box.Min.Y, Box.Min.Z };
		const float MaxAxis[3] = { Box.Max.X, Box.Max.Y, Box.Max.Z };

		float TMin = 0.f;
		float TMax = MaxDistanceCm;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			if (FMath::Abs(DirAxis[Axis]) < KINDA_SMALL_NUMBER)
			{
				if (OriginAxis[Axis] < MinAxis[Axis] || OriginAxis[Axis] > MaxAxis[Axis])
				{
					return false;
				}
				continue;
			}

			float Near = (MinAxis[Axis] - OriginAxis[Axis]) / DirAxis[Axis];
			float Far = (MaxAxis[Axis] - OriginAxis[Axis]) / DirAxis[Axis];
			if (Near > Far)
			{
				const float SwapNear = Near;
				Near = Far;
				Far = SwapNear;
			}
			TMin = FMath::Max(TMin, Near);
			TMax = FMath::Min(TMax, Far);
			if (TMin > TMax)
			{
				return false;
			}
		}
		return TMax >= 0.f;
	}

	inline const TCHAR* TipText(const EHit Hit)
	{
		switch (Hit)
		{
		case EHit::Enter:
			return SolidDungeonLevel::EnterLabel;
		case EHit::Leave:
			return SolidDungeonLevel::LeaveLabel;
		default:
			return TEXT("");
		}
	}

	/** Dark opening only. Jambs, the lintel, and the den floor are not a passage. */
	inline EHit Classify(const AActor* Actor, const UPrimitiveComponent* Component)
	{
		if (!Actor || !Component)
		{
			return EHit::None;
		}
		if (const ASolidDungeonEntrance* Entrance = Cast<ASolidDungeonEntrance>(Actor))
		{
			return Component == Entrance->GetMouthMesh() ? EHit::Enter : EHit::None;
		}
		if (const ASolidDungeonInterior* Interior = Cast<ASolidDungeonInterior>(Actor))
		{
			return Component == Interior->GetMouthMesh() ? EHit::Leave : EHit::None;
		}
		return EHit::None;
	}

	struct FTipBox
	{
		float X = 0.f;
		float Y = 0.f;
		float W = 0.f;
		float H = 0.f;
	};

	/** Gap so the label sits clear of the arrow, which extends down and to the right. */
	inline constexpr float PointerClearance = 24.f;
	inline constexpr float ViewMargin = 4.f;

	inline bool Contains(const FTipBox& Box, const FVector2D& Point)
	{
		return Point.X >= Box.X && Point.X < Box.X + Box.W
			&& Point.Y >= Box.Y && Point.Y < Box.Y + Box.H;
	}

	/** Beside the pointer. Flips toward the view when the label would leave the screen. */
	inline FTipBox PlaceTip(
		const FVector2D& Hotspot,
		const float BoxW,
		const float BoxH,
		const float ViewW,
		const float ViewH)
	{
		FTipBox Box;
		Box.W = BoxW;
		Box.H = BoxH;
		Box.X = Hotspot.X + PointerClearance;
		Box.Y = Hotspot.Y + 18.f;
		if (Box.X + Box.W > ViewW - ViewMargin)
		{
			Box.X = Hotspot.X - PointerClearance - Box.W;
		}
		if (Box.Y + Box.H > ViewH - ViewMargin)
		{
			Box.Y = Hotspot.Y - ViewMargin - Box.H;
		}
		const float MaxX = FMath::Max(ViewMargin, ViewW - Box.W - ViewMargin);
		const float MaxY = FMath::Max(ViewMargin, ViewH - Box.H - ViewMargin);
		Box.X = FMath::Clamp(Box.X, ViewMargin, MaxX);
		Box.Y = FMath::Clamp(Box.Y, ViewMargin, MaxY);
		return Box;
	}
}
