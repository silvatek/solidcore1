#pragma once

#include "CoreMinimal.h"

/** Screen-space pick for the character-editor skeleton. A bone is the segment that ends at its joint. */
namespace SolidSkeletonHover
{
	/** How close the pointer must be to a bone, in pixels. */
	inline constexpr float PickRadiusPx = 22.f;

	/**
	 * Closest distance from a ray to a segment.
	 * RayDir is a unit vector. OutRayT is the distance along the ray, never negative.
	 * OutSegU is 0 at A and 1 at B.
	 */
	inline float RaySegmentDistance(
		const FVector& RayOrigin,
		const FVector& RayDir,
		const FVector& SegA,
		const FVector& SegB,
		float& OutRayT,
		float& OutSegU)
	{
		const FVector D1 = RayDir;
		const FVector D2 = SegB - SegA;
		const FVector R = RayOrigin - SegA;
		const float A = 1.f;
		const float E = D2.SizeSquared();
		const float F = FVector::DotProduct(D2, R);
		const float C = FVector::DotProduct(D1, R);
		const float B = FVector::DotProduct(D1, D2);
		const float Denom = A * E - B * B;

		float S = 0.f;
		float T = 0.f;
		if (E < KINDA_SMALL_NUMBER)
		{
			T = 0.f;
			S = FMath::Max(0.f, -C);
		}
		else if (Denom < 1.e-4f)
		{
			T = FMath::Clamp(F / E, 0.f, 1.f);
			const FVector ClosestSeg = SegA + D2 * T;
			S = FMath::Max(0.f, static_cast<float>(FVector::DotProduct(ClosestSeg - RayOrigin, D1)));
		}
		else
		{
			S = (B * F - E * C) / Denom;
			T = (A * F - B * C) / Denom;
			if (T < 0.f)
			{
				T = 0.f;
				S = FMath::Max(0.f, -C);
			}
			else if (T > 1.f)
			{
				T = 1.f;
				S = FMath::Max(0.f, B - C);
			}
			else if (S < 0.f)
			{
				S = 0.f;
				T = FMath::Clamp(F / E, 0.f, 1.f);
			}
		}

		OutRayT = S;
		OutSegU = T;
		return static_cast<float>(FVector::Dist(RayOrigin + D1 * S, SegA + D2 * T));
	}

	/**
	 * Bones left off the skeleton view. IK helpers, and clothing or weapon chains
	 * whose names start with axe_, sheath_, cloack_, beard_, shield_, or skirt_.
	 */
	inline bool IsOmittedBoneName(const FString& Name)
	{
		if (Name.Contains(TEXT("ik"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		const TCHAR* Prefixes[] = {
			TEXT("axe_"),
			TEXT("sheath_"),
			TEXT("cloack_"),
			TEXT("beard_"),
			TEXT("shield_"),
			TEXT("skirt_"),
		};
		for (const TCHAR* Prefix : Prefixes)
		{
			if (Name.StartsWith(Prefix, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

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
