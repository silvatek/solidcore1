#pragma once

#include "CoreMinimal.h"
#include "SolidWorldMap.h"

/**
 * A WorldMap R marks a river through that square, not a square of water.
 * The channel is 4 m wide. An R with no river or sea beside it is a 4 m pond.
 * Orthogonal neighbors that are R or the sea marker are joined at that width, however long the gap is.
 * Local coordinates are centimetres from the cell centre: +X east, +Y north.
 */
namespace SolidRiver
{
	/** Channel width, and the diameter of an isolated pond. */
	inline constexpr float WidthCm = 400.f;

	struct FLinks
	{
		bool bNorth = false;
		bool bEast = false;
		bool bSouth = false;
		bool bWest = false;

		bool HasAny() const
		{
			return bNorth || bEast || bSouth || bWest;
		}
	};

	/** Which way the dry part of the square faces, away from the water. */
	enum class ESide : uint8
	{
		North,
		East,
		South,
		West,
	};

	/** R joins another river. The sea marker joins the sea. Other markers do not. */
	inline bool Joins(const TCHAR Marker)
	{
		const auto Matches = [](const TCHAR Raw, const TCHAR Expected)
		{
			return Raw == Expected || FChar::ToUpper(Raw) == Expected;
		};
		return Matches(Marker, USolidWorldMap::RiverMarker)
			|| Matches(Marker, USolidWorldMap::SeaMarker);
	}

	inline float RadiusCm()
	{
		return WidthCm * 0.5f;
	}

	inline FVector2D ClosestOnSegment(const FVector2D& Point, const FVector2D& End)
	{
		const float LenSq = End.SizeSquared();
		if (LenSq <= KINDA_SMALL_NUMBER)
		{
			return FVector2D::ZeroVector;
		}
		const float T = FMath::Clamp(FVector2D::DotProduct(Point, End) / LenSq, 0.f, 1.f);
		return End * T;
	}

	/** Nearest point on the channel centre-lines. The origin, when the cell is only a pond. */
	inline FVector2D ClosestWater(const FVector2D& LocalCm, const float CellW, const float CellH, const FLinks& Links)
	{
		if (!Links.HasAny())
		{
			return FVector2D::ZeroVector;
		}

		const float HalfW = CellW * 0.5f;
		const float HalfH = CellH * 0.5f;
		FVector2D Best = FVector2D::ZeroVector;
		float BestDistSq = TNumericLimits<float>::Max();
		auto Consider = [&Best, &BestDistSq, &LocalCm](const FVector2D& End)
		{
			const FVector2D Closest = ClosestOnSegment(LocalCm, End);
			const float DistSq = FVector2D::DistSquared(LocalCm, Closest);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Closest;
			}
		};
		if (Links.bNorth)
		{
			Consider(FVector2D(0.f, HalfH));
		}
		if (Links.bEast)
		{
			Consider(FVector2D(HalfW, 0.f));
		}
		if (Links.bSouth)
		{
			Consider(FVector2D(0.f, -HalfH));
		}
		if (Links.bWest)
		{
			Consider(FVector2D(-HalfW, 0.f));
		}
		return Best;
	}

	inline bool IsWater(const FVector2D& LocalCm, const float CellW, const float CellH, const FLinks& Links)
	{
		const float Radius = RadiusCm();
		if (!Links.HasAny())
		{
			return LocalCm.SizeSquared() <= Radius * Radius;
		}
		const FVector2D Closest = ClosestWater(LocalCm, CellW, CellH, Links);
		return FVector2D::DistSquared(LocalCm, Closest) <= Radius * Radius;
	}

	inline ESide Side(const FVector2D& LocalCm, const float CellW, const float CellH, const FLinks& Links)
	{
		FVector2D Dir = LocalCm;
		if (Links.HasAny())
		{
			Dir = LocalCm - ClosestWater(LocalCm, CellW, CellH, Links);
		}
		if (Dir.SizeSquared() < 1.f)
		{
			return ESide::North;
		}
		if (FMath::Abs(Dir.X) > FMath::Abs(Dir.Y))
		{
			return Dir.X > 0.f ? ESide::East : ESide::West;
		}
		return Dir.Y > 0.f ? ESide::North : ESide::South;
	}
}
