#pragma once

#include "CoreMinimal.h"

namespace SolidStronghold
{
	/**
	 * A handrail: a bar about waist height, and the posts that hold it.
	 * Where a tower stands this rail is that tower's own placement.
	 */
	class FSolidHandrail
	{
	public:
		/** One straight bar or one post. */
		struct FPiece
		{
			FVector Location = FVector::ZeroVector;
			FRotator Rotation = FRotator::ZeroRotator;
			FVector SizeCm = FVector::OneVector;
			/** A post stands on the walking surface. A bar is the handrail. */
			bool bPost = false;
			/** Set when this piece closes an end the rail would otherwise leave open. */
			bool bHoleEnd = false;
		};

		/**
		 * Height of the bar above the local walking surface.
		 * The captain's capsule is 192 cm, so this is about waist height.
		 */
		static constexpr float HeightCm = 100.f;
		/** Square section of the bar and the posts. */
		static constexpr float BarCm = 10.f;
		/** Longest straight run of the bar along an arc, before the next post. */
		static constexpr float SpanCm = 50.f;

		/** A point on the rail at this angle, radius, and height. */
		static FVector Point(float Angle, float Radius, float Z)
		{
			return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z);
		}

		void Reset() { Stored.Reset(); }
		const TArray<FPiece>& Pieces() const { return Stored; }

		/** A bar from A to B. Skipped when the run is shorter than 1 cm. */
		void AddBar(const FVector& A, const FVector& B, bool bHoleEnd);

		/** A post whose base sits on the walking surface. */
		void AddPost(const FVector& BaseOnSurface, float YawDeg, bool bHoleEnd);

		/**
		 * Bars then posts along an arc.
		 * SurfaceZ(Along) is the walking surface, Along from 0 to 1.
		 */
		template<typename SurfaceFn>
		void CollectArc(float StartRad, float SweepRad, float Radius, SurfaceFn SurfaceZ);

	private:
		static FRotator Rotation(const FVector& Direction);

		TArray<FPiece> Stored;
	};

	template<typename SurfaceFn>
	void FSolidHandrail::CollectArc(
		const float StartRad,
		const float SweepRad,
		const float Radius,
		SurfaceFn SurfaceZ)
	{
		if (SweepRad <= 0.f || Radius <= 1.f)
		{
			return;
		}
		const float Arc = SweepRad * Radius;
		const int32 Count = FMath::Max(1, FMath::CeilToInt(Arc / SpanCm));
		auto AtRail = [&](const float Along)
		{
			const float Angle = StartRad + Along * SweepRad;
			return Point(Angle, Radius, SurfaceZ(Along) + HeightCm);
		};
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float From = static_cast<float>(Index) / static_cast<float>(Count);
			const float To = static_cast<float>(Index + 1) / static_cast<float>(Count);
			AddBar(AtRail(From), AtRail(To), false);
		}
		for (int32 Index = 0; Index <= Count; ++Index)
		{
			const float Along = static_cast<float>(Index) / static_cast<float>(Count);
			const float Angle = StartRad + Along * SweepRad;
			AddPost(Point(Angle, Radius, SurfaceZ(Along)), FMath::RadiansToDegrees(Angle), false);
		}
	}
}
