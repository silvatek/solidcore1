#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"
#include "SolidDungeonLevel.h"

/**
 * Pointy-top hexes on a stronghold floor, for placing furniture.
 * Dungeons, fortresses, and towers share this grid. The Meagre Dungeon uses it first.
 * "Across" is flat side to flat side (the horizontal width), not point to point.
 * Only hexes that sit entirely on the floor are kept, so a piece on a cell stays inside the room.
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

	inline FBox2D MeagreFloorBounds()
	{
		const float HalfY = SolidDungeonLevel::RoomWidthCm * 0.5f;
		return FBox2D(
			FVector2D(0.f, -HalfY),
			FVector2D(SolidDungeonLevel::RoomDepthCm, HalfY));
	}

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

	inline bool HexFitsOnFloor(const FBox2D& Floor, const FVector2D& Center)
	{
		const float HalfFlat = FlatToFlatCm * 0.5f;
		const float HalfVertex = SideCm();
		constexpr float Slack = 0.05f;
		return Center.X - HalfFlat >= Floor.Min.X - Slack
			&& Center.X + HalfFlat <= Floor.Max.X + Slack
			&& Center.Y - HalfVertex >= Floor.Min.Y - Slack
			&& Center.Y + HalfVertex <= Floor.Max.Y + Slack;
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

	inline void CollectHexes(const FBox2D& Floor, TArray<FHex>& OutHexes)
	{
		OutHexes.Reset();
		const FVector2D Origin = Floor.GetCenter();
		const int32 MaxCol = FMath::CeilToInt(Floor.GetSize().X / FlatToFlatCm) + 2;
		const int32 MaxRow = FMath::CeilToInt(Floor.GetSize().Y / FMath::Max(RowStepCm(), 1.f)) + 2;
		for (int32 Row = -MaxRow; Row <= MaxRow; ++Row)
		{
			for (int32 Col = -MaxCol; Col <= MaxCol; ++Col)
			{
				const FVector2D Center = HexCenter(Col, Row, Origin);
				if (!HexFitsOnFloor(Floor, Center))
				{
					continue;
				}
				FHex Hex;
				Hex.Center = Center;
				Hex.Col = Col;
				Hex.Row = Row;
				OutHexes.Add(Hex);
			}
		}
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
}
