#pragma once

#include "CoreMinimal.h"

/** Screen-space pick for the character-editor skeleton. A bone is the segment that ends at its joint. */
namespace SolidSkeletonHover
{
	/** How close the pointer must be to a bone, in pixels. */
	inline constexpr float PickRadiusPx = 22.f;

	/** Distance from Point to the segment. A joint with no parent is a zero-length segment. */
	inline float SegmentDistance(const FVector2D& From, const FVector2D& To, const FVector2D& Point)
	{
		const FVector2D Edge = To - From;
		const float LengthSq = Edge.SizeSquared();
		if (LengthSq <= KINDA_SMALL_NUMBER)
		{
			return FVector2D::Distance(Point, To);
		}
		const float Along = FMath::Clamp(FVector2D::DotProduct(Point - From, Edge) / LengthSq, 0.f, 1.f);
		return FVector2D::Distance(Point, From + Edge * Along);
	}

	/**
	 * The bone closest to Point, when that distance is within RadiusPx.
	 * Two bones that meet at a joint yield the one whose joint is nearer the pointer.
	 * Empty when the pointer misses every bone.
	 */
	inline FString BoneUnderPoint(
		const FVector2D* From,
		const FVector2D* To,
		const FString* Names,
		const int32 Count,
		const FVector2D& Point,
		const float RadiusPx)
	{
		FString Best;
		float BestDist = RadiusPx;
		float BestJoint = TNumericLimits<float>::Max();
		if (!From || !To || !Names || Count <= 0)
		{
			return Best;
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Names[Index].IsEmpty())
			{
				continue;
			}
			const float Dist = SegmentDistance(From[Index], To[Index], Point);
			if (Dist > RadiusPx)
			{
				continue;
			}
			const float Joint = FVector2D::Distance(Point, To[Index]);
			const bool bCloser = Best.IsEmpty() || Dist < BestDist - 0.01f;
			const bool bSameBone = !bCloser && Dist <= BestDist + 0.01f && Joint < BestJoint;
			if (bCloser || bSameBone)
			{
				BestDist = Dist;
				BestJoint = Joint;
				Best = Names[Index];
			}
		}
		return Best;
	}
}
