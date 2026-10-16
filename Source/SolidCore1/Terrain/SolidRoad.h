#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.h"

class USolidWorldMap;

/**
 * A road line in WorldMap.txt names a start, an end, and a type.
 * The route is a path of map cells. A terrain quad takes the biome of its
 * northeast corner, so the painted strip lies half a step southwest of those centres.
 * Trails are 1 m, tracks 2 m, roads 3 m, highways 4 m.
 * Every route stays off mountains and the sea. Roads and highways also stay off
 * forest and desert, and they pay extra for every river cell so a crossing is short.
 * A turn beside a river costs extra, and a spur just past the far bank is pulled
 * back, so the road runs straight across the water.
 * The strip uses a road texture, a brown close to the town colour.
 * River water keeps the river texture.
 * Local offsets are centimetres. +X is east, +Y is north.
 */
namespace SolidRoad
{
	enum class EType : uint8
	{
		Trail,
		Track,
		Road,
		Highway,
	};

	struct FEndpoint
	{
		/** True when the end is the cave (marker C) rather than a numbered town. */
		bool bCave = false;
		int32 LocationId = INDEX_NONE;
	};

	struct FDefinition
	{
		FEndpoint From;
		FEndpoint To;
		EType Type = EType::Trail;
	};

	struct FRoute
	{
		FDefinition Definition;
		/** Cell centres the strip follows, including both ends. Empty when an end is missing. */
		TArray<FIntPoint> Cells;
	};

	/** One painted segment, in map cells. A point span uses the same cell twice. */
	struct FSpan
	{
		FIntPoint From = FIntPoint::ZeroValue;
		FIntPoint To = FIntPoint::ZeroValue;
		float HalfWidthCm = 0.f;
	};

	/**
	 * One step, in the same units as the other costs. A surface preference is
	 * smaller than this, so it only breaks a tie between equal-length routes.
	 */
	inline constexpr int32 StepCost = 16;
	/** Roads and highways pay this to enter a river cell. One crossing, not a trip along the water. */
	inline constexpr int32 RiverCost = StepCost * 40;
	/** Cells around a river, diagonals included, where a turn pays RiverBendCost. */
	inline constexpr int32 RiverBendReach = 2;
	/** Less than a step, so the route does not lengthen. A straight crossing wins the tie. */
	inline constexpr int32 RiverBendCost = 4;
	/** Closed ground. Mountains and the sea cost this for every type. Forest and desert cost it for a road or a highway. */
	inline constexpr int32 AvoidCost = StepCost * 800;

	/**
	 * Terrain quads are this wide. Rounding picks the northeast corner, so the
	 * painted strip lies half a step toward -X and -Y. A bridge uses that shift
	 * and sits on the strip.
	 */
	inline constexpr float TerrainQuadCm = 200.f;
	inline constexpr float StripPaintOffsetCm = -0.5f * TerrainQuadCm;

	inline FVector2D StripPaintOffset()
	{
		return FVector2D(StripPaintOffsetCm, StripPaintOffsetCm);
	}

	inline float WidthCm(const EType Type)
	{
		switch (Type)
		{
		case EType::Track: return 200.f;
		case EType::Road: return 300.f;
		case EType::Highway: return 400.f;
		case EType::Trail:
		default: return 100.f;
		}
	}

	inline bool AvoidsHardGround(const EType Type)
	{
		return Type == EType::Road || Type == EType::Highway;
	}

	/** Lower is better. Never as large as a whole extra step. */
	inline int32 SurfacePreference(const ESolidBiome Biome)
	{
		switch (Biome)
		{
		case ESolidBiome::Grassland:
		case ESolidBiome::Town:
		case ESolidBiome::Road:
		case ESolidBiome::Swamp:
			return 0;
		case ESolidBiome::Forest:
		case ESolidBiome::Desert:
			return 2;
		case ESolidBiome::Mountain:
			return 4;
		case ESolidBiome::River:
			return 6;
		case ESolidBiome::Sea:
			return 8;
		default:
			return 1;
		}
	}

	inline int32 EnterCost(const EType Type, const ESolidBiome Biome)
	{
		int32 Cost = StepCost;
		if (Biome == ESolidBiome::Sea || Biome == ESolidBiome::Mountain)
		{
			Cost = AvoidCost;
		}
		else if (AvoidsHardGround(Type))
		{
			if (Biome == ESolidBiome::Forest || Biome == ESolidBiome::Desert)
			{
				Cost = AvoidCost;
			}
			else if (Biome == ESolidBiome::River)
			{
				Cost = RiverCost;
			}
		}
		return Cost + SurfacePreference(Biome);
	}

	/** "Road: C -> 0, trail". False when the line is not a road, or the road is incomplete. */
	bool Parse(const FString& Line, FDefinition& Out);

	/**
	 * Orthogonal route from any cell of From to any cell of To.
	 * @return False when an end has no cells, or no path exists.
	 */
	bool Solve(const USolidWorldMap& Map, const FDefinition& Definition, TArray<FIntPoint>& OutCells);

	float DistanceToSegmentCm(const FVector2D& Point, const FVector2D& A, const FVector2D& B);
}
