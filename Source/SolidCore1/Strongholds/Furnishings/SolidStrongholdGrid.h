#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"

/**
 * Pointy-top hexes on a stronghold floor, for placing furniture.
 * Dungeons, fortresses, and towers share this grid.
 * "Across" is flat side to flat side (the horizontal width), not point to point.
 * Which cells a stronghold keeps is that type's floor, not this grid.
 */
namespace SolidStrongholdGrid
{
	/** 1.2 m, edge to edge. */
	inline constexpr float FlatToFlatCm = 120.f;

	inline constexpr float LineThicknessCm = 5.f;
	inline constexpr float LineHeightCm = 2.f;
	/** Bottom of each line sits this far above the floor so it does not z-fight the stone. */
	inline constexpr float LineLiftCm = 1.f;

	inline FLinearColor LineColor()
	{
		return FLinearColor(1.f, 0.86f, 0.08f);
	}

	/** Point to point. Longer than the flat-to-flat width. */
	inline float VertexToVertexCm()
	{
		return 2.f * FlatToFlatCm / FMath::Sqrt(3.f);
	}

	/** Center to a vertex, and the length of one side. */
	inline float SideCm()
	{
		return VertexToVertexCm() * 0.5f;
	}

	/** Center spacing from one row to the next. */
	inline float RowStepCm()
	{
		return SideCm() * 1.5f;
	}

	struct FCell
	{
		int32 Col = 0;
		int32 Row = 0;
	};

	struct FHex
	{
		FVector2D Center = FVector2D::ZeroVector;
		int32 Col = 0;
		int32 Row = 0;
	};

	struct FSegment
	{
		FVector2D A = FVector2D::ZeroVector;
		FVector2D B = FVector2D::ZeroVector;
	};

	inline FVector2D HexCenter(const int32 Col, const int32 Row, const FVector2D& Origin)
	{
		const float OddShift = (Row & 1) != 0 ? FlatToFlatCm * 0.5f : 0.f;
		return Origin + FVector2D(Col * FlatToFlatCm + OddShift, Row * RowStepCm());
	}

	inline void HexVertices(const FVector2D& Center, FVector2D OutVertices[6])
	{
		const float Radius = SideCm();
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(60.f * static_cast<float>(Index) + 30.f);
			OutVertices[Index] = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		}
	}

	/** True when Point is inside the hex, including the boundary. */
	inline bool PointInHex(const FVector2D& Point, const FVector2D& Center)
	{
		FVector2D Vertices[6];
		HexVertices(Center, Vertices);
		constexpr float Slack = 0.05f;
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector2D Edge = Vertices[(Index + 1) % 6] - Vertices[Index];
			const FVector2D ToPoint = Point - Vertices[Index];
			const float Cross = Edge.X * ToPoint.Y - Edge.Y * ToPoint.X;
			if (Cross < -Slack)
			{
				return false;
			}
		}
		return true;
	}

	struct FEdgeKey
	{
		int32 AX = 0;
		int32 AY = 0;
		int32 BX = 0;
		int32 BY = 0;

		bool operator==(const FEdgeKey& Other) const
		{
			return AX == Other.AX && AY == Other.AY && BX == Other.BX && BY == Other.BY;
		}
	};

	inline uint32 GetTypeHash(const FEdgeKey& Key)
	{
		return HashCombine(
			HashCombine(::GetTypeHash(Key.AX), ::GetTypeHash(Key.AY)),
			HashCombine(::GetTypeHash(Key.BX), ::GetTypeHash(Key.BY)));
	}

	inline void CollectSegments(const TArray<FHex>& Hexes, TArray<FSegment>& OutSegments)
	{
		OutSegments.Reset();
		TSet<FEdgeKey> Seen;
		for (const FHex& Hex : Hexes)
		{
			FVector2D Vertices[6];
			HexVertices(Hex.Center, Vertices);
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const FVector2D& P = Vertices[Index];
				const FVector2D& Q = Vertices[(Index + 1) % 6];
				auto Quantize = [](const FVector2D& Point) -> TPair<int32, int32>
				{
					return TPair<int32, int32>(
						FMath::RoundToInt(Point.X * 10.f),
						FMath::RoundToInt(Point.Y * 10.f));
				};
				const TPair<int32, int32> KeyP = Quantize(P);
				const TPair<int32, int32> KeyQ = Quantize(Q);
				const bool bPFirst = KeyP.Key < KeyQ.Key || (KeyP.Key == KeyQ.Key && KeyP.Value < KeyQ.Value);
				FEdgeKey Key;
				Key.AX = bPFirst ? KeyP.Key : KeyQ.Key;
				Key.AY = bPFirst ? KeyP.Value : KeyQ.Value;
				Key.BX = bPFirst ? KeyQ.Key : KeyP.Key;
				Key.BY = bPFirst ? KeyQ.Value : KeyP.Value;
				if (Seen.Contains(Key))
				{
					continue;
				}
				Seen.Add(Key);
				FSegment Segment;
				Segment.A = P;
				Segment.B = Q;
				OutSegments.Add(Segment);
			}
		}
	}

	/** Edges that belong to only one cell. */
	inline void CollectBoundarySegments(const TArray<FHex>& Hexes, TArray<FSegment>& OutSegments)
	{
		OutSegments.Reset();
		TMap<FEdgeKey, int32> Counts;
		TMap<FEdgeKey, FSegment> Geometry;
		for (const FHex& Hex : Hexes)
		{
			FVector2D Vertices[6];
			HexVertices(Hex.Center, Vertices);
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const FVector2D& P = Vertices[Index];
				const FVector2D& Q = Vertices[(Index + 1) % 6];
				auto Quantize = [](const FVector2D& Point) -> TPair<int32, int32>
				{
					return TPair<int32, int32>(
						FMath::RoundToInt(Point.X * 10.f),
						FMath::RoundToInt(Point.Y * 10.f));
				};
				const TPair<int32, int32> KeyP = Quantize(P);
				const TPair<int32, int32> KeyQ = Quantize(Q);
				const bool bPFirst = KeyP.Key < KeyQ.Key || (KeyP.Key == KeyQ.Key && KeyP.Value < KeyQ.Value);
				FEdgeKey Key;
				Key.AX = bPFirst ? KeyP.Key : KeyQ.Key;
				Key.AY = bPFirst ? KeyP.Value : KeyQ.Value;
				Key.BX = bPFirst ? KeyQ.Key : KeyP.Key;
				Key.BY = bPFirst ? KeyQ.Value : KeyP.Value;
				int32& Count = Counts.FindOrAdd(Key);
				if (Count == 0)
				{
					FSegment Segment;
					Segment.A = P;
					Segment.B = Q;
					Geometry.Add(Key, Segment);
				}
				++Count;
			}
		}
		for (const TPair<FEdgeKey, int32>& Pair : Counts)
		{
			if (Pair.Value == 1)
			{
				if (const FSegment* Segment = Geometry.Find(Pair.Key))
				{
					OutSegments.Add(*Segment);
				}
			}
		}
	}
}
