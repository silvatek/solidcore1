#pragma once

#include "CoreMinimal.h"
#include "Math/RotationMatrix.h"

namespace SolidSight
{
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
		if (FogAtCamera > 0.f)
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
}
