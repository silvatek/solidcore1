#pragma once

#include "CoreMinimal.h"

/**
 * In-game mouse pointer. The hotspot is the tip, at local (0, 0).
 * The arrow extends down and to the right. Mouse look is the right button;
 * while that button is held the pointer is hidden.
 */
namespace SolidPointer
{
	struct FTriangle
	{
		FVector2D A;
		FVector2D B;
		FVector2D C;
	};

	inline constexpr int32 TriangleCount = 3;
	/** Dark copies of the arrow, stamped around the tip (px). */
	inline constexpr float OutlinePad = 2.5f;
	/** Bronze edge between the rim and the bright core (px). */
	inline constexpr float EdgePad = 1.25f;
	inline constexpr int32 OutlineOffsetCount = 8;

	/** 0 and 1 are the head. 2 is the tail. */
	inline FTriangle Triangle(const int32 Index)
	{
		switch (Index)
		{
		case 0:
			return { { 0.f, 0.f }, { 0.f, 24.f }, { 7.f, 16.f } };
		case 1:
			return { { 0.f, 0.f }, { 7.f, 16.f }, { 20.f, 14.f } };
		default:
			return { { 6.f, 15.f }, { 4.f, 34.f }, { 13.f, 24.f } };
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
}
