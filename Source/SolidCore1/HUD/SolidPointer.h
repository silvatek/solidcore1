#pragma once

#include "CoreMinimal.h"

/**
 * In-game mouse pointer. The hotspot is the tip, at local (0, 0).
 * The arrow is the two head triangles. It extends down and to the right.
 * Mouse look is the right button; while that button is held the pointer is hidden.
 */
namespace SolidPointer
{
	struct FTriangle
	{
		FVector2D A;
		FVector2D B;
		FVector2D C;
	};

	inline constexpr int32 TriangleCount = 2;
	/** Dark copies of the arrow, stamped around the tip (px). */
	inline constexpr float OutlinePad = 2.5f;
	/** Bronze edge between the rim and the bright core (px). */
	inline constexpr float EdgePad = 1.25f;
	inline constexpr int32 OutlineOffsetCount = 8;

	/** Both triangles are the head. They meet at the tip. There is no tail. */
	inline FTriangle Triangle(const int32 Index)
	{
		switch (Index)
		{
		case 1:
			return { { 0.f, 0.f }, { 7.f, 16.f }, { 20.f, 14.f } };
		default:
			return { { 0.f, 0.f }, { 0.f, 24.f }, { 7.f, 16.f } };
		}
	}

	/** Right mouse button turns the camera. The pointer stays put otherwise. */
	inline bool AllowsMouseLook(const bool bRightMouseDown)
	{
		return bRightMouseDown;
	}

	/** Player input or Slate may be the one that still sees the button. */
	inline bool IsRightMouseHeld(const bool bPlayerInputDown, const bool bSlateDown)
	{
		return bPlayerInputDown || bSlateDown;
	}

	/**
	 * Raw MouseX/MouseY to yaw/pitch. MouseY is negated, matching the mapping
	 * that used to feed Look (negate, then swizzle into the axis).
	 */
	inline FVector2D MouseLookDelta(const float DeltaX, const float DeltaY)
	{
		return FVector2D(DeltaX, -DeltaY);
	}

	inline bool ShouldDraw(const bool bHasMousePosition, const bool bRightMouseDown)
	{
		return bHasMousePosition && !AllowsMouseLook(bRightMouseDown);
	}

	/**
	 * Horizontal slice through the triangle at ScanY (top-inclusive, bottom-exclusive).
	 * Used to paint the arrow with rects; the game module cannot link GWhiteTexture.
	 */
	inline bool SpanAtY(const FTriangle& Tri, const float ScanY, float& OutLeft, float& OutRight)
	{
		const FVector2D Vertices[3] = { Tri.A, Tri.B, Tri.C };
		float Xs[3] = { 0.f, 0.f, 0.f };
		int32 Found = 0;
		for (int32 Edge = 0; Edge < 3; ++Edge)
		{
			const FVector2D& A = Vertices[Edge];
			const FVector2D& B = Vertices[(Edge + 1) % 3];
			const float YMin = FMath::Min(A.Y, B.Y);
			const float YMax = FMath::Max(A.Y, B.Y);
			if (ScanY < YMin || ScanY >= YMax)
			{
				continue;
			}
			const float T = (ScanY - A.Y) / (B.Y - A.Y);
			Xs[Found++] = FMath::Lerp(A.X, B.X, T);
		}
		if (Found < 2)
		{
			return false;
		}
		OutLeft = FMath::Min(Xs[0], Xs[1]);
		OutRight = FMath::Max(Xs[0], Xs[1]);
		if (Found > 2)
		{
			OutLeft = FMath::Min(OutLeft, Xs[2]);
			OutRight = FMath::Max(OutRight, Xs[2]);
		}
		return OutRight > OutLeft;
	}

	inline FTriangle Place(const FTriangle& Local, const FVector2D& HotspotScreen)
	{
		return {
			Local.A + HotspotScreen,
			Local.B + HotspotScreen,
			Local.C + HotspotScreen
		};
	}

	/** Eight compass stamps. Empty when Pad is 0; the core arrow is drawn at the hotspot. */
	inline void OutlineOffsets(const float Pad, FVector2D (&OutOffsets)[OutlineOffsetCount])
	{
		const FVector2D Steps[OutlineOffsetCount] = {
			{ -1.f, -1.f }, { 0.f, -1.f }, { 1.f, -1.f },
			{ -1.f, 0.f },                 { 1.f, 0.f },
			{ -1.f, 1.f },  { 0.f, 1.f },  { 1.f, 1.f }
		};
		for (int32 Index = 0; Index < OutlineOffsetCount; ++Index)
		{
			OutOffsets[Index] = Steps[Index].GetSafeNormal() * Pad;
		}
	}

	/** The pointer is up, and no menu or drill is using it. */
	inline bool AcceptsPointer(const bool bPointerShown, const bool bMenuOrDrill)
	{
		return bPointerShown && !bMenuOrDrill;
	}

	/**
	 * A visibility trace misses an engine cube, including a ray that starts inside it.
	 * A click uses the component bounds instead.
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
