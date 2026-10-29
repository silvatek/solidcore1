#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Math/RotationMatrix.h"
#include "SolidTerrainFog.h"

/** Raven sight is the third-person squad camera. True sight is first person. */
enum class ESolidSight : uint8
{
	Raven,
	True,
	Falcon,
	Eagle,
};

/** F9 view toggle. Pure helpers — no actor spawning (safe for automation). */
namespace SolidSight
{
	/** Eye height above the capsule center (cm). */
	inline constexpr float TrueSightEyeHeightCm = 64.f;

	/** Falcon camera height above the ground under the camera (cm). */
	inline constexpr float FalconMinHeightCm = 2000.f;
	inline constexpr float FalconMaxHeightCm = 20000.f;

	/** Share of the inner clear radius Falcon may stand back from the captain. */
	inline constexpr float FalconClearFraction = 0.8f;

	/** 80% of the clear band. A 25 m clear radius is 20 m. */
	inline constexpr float FalconMaxOffsetCm =
		SolidTerrainFog::ClearRadiusMeters * FalconClearFraction * 100.f;

	/** Camera sits this far along the look direction so it clears the near clip (cm). */
	inline constexpr float TrueSightForwardCm = 12.f;

	/** Right stick click. The same toggle as F9. */
	inline FKey ControllerToggleKey()
	{
		return EKeys::Gamepad_RightThumbstick;
	}

	inline bool IsTrueSight(ESolidSight Sight)
	{
		return Sight == ESolidSight::True;
	}

	inline bool IsFalconSight(ESolidSight Sight)
	{
		return Sight == ESolidSight::Falcon;
	}

	inline bool IsEagleSight(ESolidSight Sight)
	{
		return Sight == ESolidSight::Eagle;
	}

	/** Straight down, with north (+Y) at the top of the view and east at the right. */
	inline FRotator EagleLook()
	{
		return FRotator(-90.f, 90.f, 0.f);
	}

	inline FVector2D WorldCentre(const FVector2D& WorldMin, const FVector2D& WorldMax)
	{
		return (WorldMin + WorldMax) * 0.5f;
	}

	/**
	 * The camera is over Camera, and FogAtCamera is that terrain point.
	 * Clear fog keeps the point and steps Next toward Centre.
	 * Fog sends Next back to the last kept point and stops the move.
	 */
	struct FEagleCameraGround
	{
		FVector2D Kept = FVector2D::ZeroVector;
		FVector2D Next = FVector2D::ZeroVector;
		bool bSettled = false;
	};

	inline FEagleCameraGround EagleCameraGround(
		const FVector2D& Camera,
		const FVector2D& Kept,
		const FVector2D& Centre,
		const float StepCm,
		const float FogAtCamera)
	{
		FEagleCameraGround Ground;
		Ground.Kept = Kept;
		Ground.Next = Kept;
		if (FogAtCamera > SolidTerrainFog::ClearFogEpsilon)
		{
			Ground.bSettled = true;
			return Ground;
		}

		Ground.Kept = Camera;
		const FVector2D Delta = Centre - Camera;
		const float Length = Delta.Size();
		if (Length <= KINDA_SMALL_NUMBER)
		{
			Ground.Next = Camera;
			Ground.bSettled = true;
			return Ground;
		}

		const float Step = FMath::Max(StepCm, 1.f);
		const float Move = FMath::Min(Step, Length);
		Ground.Next = Camera + Delta / Length * Move;
		return Ground;
	}

	/**
	 * Height above the ground so the world square fits in view, looking straight down.
	 * HorizontalFovDeg is the camera's horizontal field of view. Aspect is width / height.
	 */
	inline float EagleHeightCm(
		const FVector2D& Ground,
		const FVector2D& WorldMin,
		const FVector2D& WorldMax,
		const float HorizontalFovDeg,
		const float Aspect)
	{
		const float HalfH = FMath::DegreesToRadians(FMath::Clamp(HorizontalFovDeg, 1.f, 179.f) * 0.5f);
		const float TanH = FMath::Max(FMath::Tan(HalfH), 0.01f);
		const float TanV = FMath::Max(TanH / FMath::Max(Aspect, 0.1f), 0.01f);
		const FRotationMatrix Axes(EagleLook());
		const FVector Right = Axes.GetUnitAxis(EAxis::Y);
		const FVector Up = Axes.GetUnitAxis(EAxis::Z);
		const FVector2D Corners[4] = {
			WorldMin,
			FVector2D(WorldMax.X, WorldMin.Y),
			WorldMax,
			FVector2D(WorldMin.X, WorldMax.Y),
		};
		float MaxRight = 0.f;
		float MaxUp = 0.f;
		for (const FVector2D& Corner : Corners)
		{
			const FVector Delta(Corner.X - Ground.X, Corner.Y - Ground.Y, 0.f);
			MaxRight = FMath::Max(MaxRight, FMath::Abs(FVector::DotProduct(Delta, Right)));
			MaxUp = FMath::Max(MaxUp, FMath::Abs(FVector::DotProduct(Delta, Up)));
		}
		return FMath::Max(MaxRight / TanH, MaxUp / TanV);
	}

	/** Where Eagle Sight stands, and how high, so the world square fills the shorter side of the window. */
	struct FEagleFill
	{
		FVector2D Ground = FVector2D::ZeroVector;
		float HeightCm = 0.f;
	};

	/**
	 * The square fills the shorter side of the viewport.
	 * A wider window meets the top and the bottom, with the camera at the centre.
	 * A taller window meets the left and the right, and the south edge stays on the bottom.
	 */
	inline FEagleFill EagleFill(
		const FVector2D& WorldMin,
		const FVector2D& WorldMax,
		const float HorizontalFovDeg,
		const float ViewportWidth,
		const float ViewportHeight)
	{
		const float HalfH = FMath::DegreesToRadians(FMath::Clamp(HorizontalFovDeg, 1.f, 179.f) * 0.5f);
		const float TanH = FMath::Max(FMath::Tan(HalfH), 0.01f);
		const float ViewW = FMath::Max(ViewportWidth, 1.f);
		const float ViewH = FMath::Max(ViewportHeight, 1.f);
		const float TanV = FMath::Max(TanH / (ViewW / ViewH), 0.01f);
		const float MapW = FMath::Max(WorldMax.X - WorldMin.X, 1.f);
		const float MapH = FMath::Max(WorldMax.Y - WorldMin.Y, 1.f);
		const FVector2D Centre = WorldCentre(WorldMin, WorldMax);

		FEagleFill Fill;
		if (ViewW >= ViewH)
		{
			Fill.HeightCm = (MapH * 0.5f) / TanV;
			Fill.Ground = Centre;
		}
		else
		{
			Fill.HeightCm = (MapW * 0.5f) / TanH;
			Fill.Ground = FVector2D(Centre.X, WorldMin.Y + Fill.HeightCm * TanV);
		}
		return Fill;
	}

	/**
	 * Height above the ground that puts the map across the span the window sees now.
	 * The view is straight down, so that span grows in proportion to the height.
	 */
	inline float EagleSpanHeightCm(const float ViewHeightCm, const float SeenSpanCm, const float MapSpanCm)
	{
		if (ViewHeightCm <= 1.f || SeenSpanCm <= 1.f || MapSpanCm <= 1.f)
		{
			return ViewHeightCm;
		}
		return ViewHeightCm * (MapSpanCm / SeenSpanCm);
	}

	/**
	 * Height that puts every fog-0 point in view. The spans are the ground the window
	 * covers now, and the reach from the camera to the far side of the clear bounds.
	 */
	inline float EagleClearHeightCm(
		const float ViewHeightCm,
		const float SeenSpanX,
		const float SeenSpanY,
		const FVector2D& Camera,
		const FVector2D& ClearMin,
		const FVector2D& ClearMax)
	{
		const float NeedX = 2.f * FMath::Max(Camera.X - ClearMin.X, ClearMax.X - Camera.X);
		const float NeedY = 2.f * FMath::Max(Camera.Y - ClearMin.Y, ClearMax.Y - Camera.Y);
		return FMath::Max(
			EagleSpanHeightCm(ViewHeightCm, SeenSpanX, NeedX),
			EagleSpanHeightCm(ViewHeightCm, SeenSpanY, NeedY));
	}

	/** Eagle Sight will not rise above this (cm). It is the tallest camera any sight reaches. */
	inline constexpr float EagleMaxHeightCm = 500000.f;

	/** How fast Eagle Sight rises or drops while it matches the window to the square (cm/s). */
	inline constexpr float EagleRiseCmPerSecond = 40000.f;

	/** One frame never spends more than this, so a hitch cannot jump the flight. */
	inline constexpr float EagleRiseMaxSeconds = 1.f / 30.f;

	/** Step the current height toward the measured height. The next look can correct the step. */
	inline float EagleRiseCm(const float CurrentHeightCm, const float TargetHeightCm, const float DeltaSeconds)
	{
		const float StepSeconds = FMath::Clamp(DeltaSeconds, 0.f, EagleRiseMaxSeconds);
		const float MaxStep = EagleRiseCmPerSecond * StepSeconds;
		return CurrentHeightCm + FMath::Clamp(TargetHeightCm - CurrentHeightCm, -MaxStep, MaxStep);
	}

	/** Where a straight-down ray meets the ground plane. A level ray does not meet it. */
	inline bool EagleGroundHit(
		const FVector& RayOrigin,
		const FVector& RayDirection,
		const float GroundZ,
		FVector& OutHit)
	{
		if (FMath::Abs(RayDirection.Z) <= 0.001f)
		{
			return false;
		}
		const float T = (GroundZ - RayOrigin.Z) / RayDirection.Z;
		if (T <= 0.f)
		{
			return false;
		}
		OutHit = RayOrigin + RayDirection * T;
		return true;
	}

	/** The engine plane mesh is 100 cm on a side. */
	inline constexpr float EagleSeaPlaneCm = 100.f;

	/** The sea sheet sits this far under the noise base, below the land. */
	inline constexpr float EagleSeaDepthCm = 20000.f;

	/** The sheet is this many world squares across, so the side gaps stay on the sea. */
	inline constexpr float EagleSeaSpans = 4.f;

	/** A flat sheet of sea under the square. The side gaps of the eagle view land on it. */
	struct FEagleSea
	{
		FVector Location = FVector::ZeroVector;
		float Scale = 1.f;
	};

	inline FEagleSea EagleSeaSheet(const FVector2D& WorldMin, const FVector2D& WorldMax)
	{
		const FVector2D Centre = WorldCentre(WorldMin, WorldMax);
		const float Span = FMath::Max(WorldMax.X - WorldMin.X, WorldMax.Y - WorldMin.Y);
		FEagleSea Sea;
		Sea.Location = FVector(Centre.X, Centre.Y, -EagleSeaDepthCm);
		Sea.Scale = FMath::Max(Span, 1.f) * EagleSeaSpans / EagleSeaPlaneCm;
		return Sea;
	}

	/** Wheel position in the raven zoom range, as 0 at the near end and 1 at the far end. */
	inline float FalconZoomAlpha(const float ArmCm, const float ZoomMin, const float ZoomMax)
	{
		const float Span = ZoomMax - ZoomMin;
		if (Span <= KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}
		return FMath::Clamp((ArmCm - ZoomMin) / Span, 0.f, 1.f);
	}

	/** 20 m at alpha 0, 200 m at alpha 1, above the ground under the camera. */
	inline float FalconHeightCm(const float ZoomAlpha)
	{
		return FMath::Lerp(FalconMinHeightCm, FalconMaxHeightCm, FMath::Clamp(ZoomAlpha, 0.f, 1.f));
	}

	/**
	 * Horizontal offset from the captain, opposite the look's yaw, 20 m away.
	 * Pitch is ignored, so a steep look still stands off to the side.
	 */
	inline FVector2D FalconHorizontalOffset(const FRotator& Look)
	{
		const FVector Back = -FRotator(0.f, Look.Yaw, 0.f).Vector();
		return FVector2D(Back.X * FalconMaxOffsetCm, Back.Y * FalconMaxOffsetCm);
	}

	/** Look from the falcon camera toward the captain. Level roll. */
	inline FRotator FalconLookAtCaptain(const FVector& Camera, const FVector& Captain)
	{
		const FVector ToCaptain = Captain - Camera;
		if (ToCaptain.IsNearlyZero())
		{
			return FRotator::ZeroRotator;
		}
		const FRotator Look = ToCaptain.Rotation();
		return FRotator(Look.Pitch, Look.Yaw, 0.f);
	}

	inline ESolidSight Toggle(ESolidSight Sight)
	{
		return IsTrueSight(Sight) ? ESolidSight::Raven : ESolidSight::True;
	}

	/** Which sights F9 may land on. True sight starts available. */
	struct FAvailable
	{
		bool bTrue = true;
		bool bRaven = false;
		bool bFalcon = false;
		bool bEagle = false;
	};

	inline bool IsAvailable(const ESolidSight Sight, const FAvailable& Available)
	{
		switch (Sight)
		{
		case ESolidSight::True: return Available.bTrue;
		case ESolidSight::Raven: return Available.bRaven;
		case ESolidSight::Falcon: return Available.bFalcon;
		case ESolidSight::Eagle: return Available.bEagle;
		default: return false;
		}
	}

	/** The next mode in F9 order: true, raven, falcon, eagle. */
	inline ESolidSight Next(const ESolidSight Sight)
	{
		switch (Sight)
		{
		case ESolidSight::True: return ESolidSight::Raven;
		case ESolidSight::Raven: return ESolidSight::Falcon;
		case ESolidSight::Falcon: return ESolidSight::Eagle;
		case ESolidSight::Eagle: return ESolidSight::True;
		default: return ESolidSight::True;
		}
	}

	/**
	 * F9. Walks that order and skips a mode that is locked.
	 * Stays on the current mode when no other mode is available.
	 */
	inline ESolidSight Cycle(const ESolidSight Sight, const FAvailable& Available)
	{
		ESolidSight Candidate = Sight;
		for (int32 Step = 0; Step < 4; ++Step)
		{
			Candidate = Next(Candidate);
			if (IsAvailable(Candidate, Available))
			{
				return Candidate;
			}
		}
		return Sight;
	}

	/**
	 * Look used after a sight change. True sight is horizontal: pitch and roll are 0.
	 * Yaw stays, so the captain still faces the same way. Raven sight keeps the tilt.
	 */
	inline FRotator ViewForSight(const ESolidSight Sight, const FRotator& View)
	{
		if (!IsTrueSight(Sight))
		{
			return View;
		}
		return FRotator(0.f, View.Yaw, 0.f);
	}

	inline const TCHAR* Label(ESolidSight Sight)
	{
		switch (Sight)
		{
		case ESolidSight::True: return TEXT("True sight");
		case ESolidSight::Falcon: return TEXT("Falcon sight");
		case ESolidSight::Eagle: return TEXT("Eagle sight");
		case ESolidSight::Raven:
		default: return TEXT("Raven sight");
		}
	}

	/** Top-right bird or eye. Falcon and eagle use the same eagle as raven. */
	inline const TCHAR* Marker(ESolidSight Sight)
	{
		return IsTrueSight(Sight) ? TEXT("\U0001F441") : TEXT("\U0001F985");
	}

	/** Glyph behind the bird. Empty for true sight and raven sight. */
	inline const TCHAR* Mark(ESolidSight Sight)
	{
		switch (Sight)
		{
		case ESolidSight::Falcon: return TEXT("+");
		case ESolidSight::Eagle: return TEXT("\U0001F310");
		default: return TEXT("");
		}
	}

	inline bool HasMark(const ESolidSight Sight)
	{
		const TCHAR* Glyph = Mark(Sight);
		return Glyph != nullptr && Glyph[0] != 0;
	}

	/** Oval width in pixels. Wider when a mark sits behind the bird. 72 matches the raven chip. */
	inline float ChipWidth(const ESolidSight Sight)
	{
		return HasMark(Sight) ? 112.f : 72.f;
	}

	/** The globe is larger than the plus. The bird stays at 28. */
	inline float MarkFontSize(const ESolidSight Sight)
	{
		return Sight == ESolidSight::Eagle ? 28.f : 20.f;
	}

	/**
	 * Top-right oval. Hidden until raven sight unlocks, and while the character editor is open.
	 * The editor hides it even when raven sight is unlocked.
	 */
	inline bool ShowsIndicator(const bool bRavenSightEnabled, const bool bCharacterEditorOpen)
	{
		return bRavenSightEnabled && !bCharacterEditorOpen;
	}
}
