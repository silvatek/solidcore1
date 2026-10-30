#pragma once

#include "CoreMinimal.h"
#include "SolidBiome.h"
#include "SolidWorldChartMark.h"
#include "Towns/SolidTowns.h"

class USolidWorldChart;

/**
 * A road line in WorldMap.txt names a start, an end, and a type.
 * The route is a path of map cells. A terrain quad takes the biome of its
 * northeast corner, so the painted strip lies half a step southwest of those centres.
 * Trails are 1 m, tracks 2 m, roads 3 m, highways 4 m.
 * A step costs 8. Town and road cost 0. Forest, desert, and swamp cost 16 for a
 * road and 32 for a highway. A river costs twice that when the step is square
 * to the river. Any other river step is banned, and so are mountain and sea.
 * A turn beside a river costs extra, and a spur just past the far bank is pulled
 * back, so the road runs straight across the water.
 * The strip uses a road texture, a brown close to the town colour.
 * River water keeps the river texture.
 * Local offsets are centimetres. +X is west, +Y is north.
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

	struct FDefinition;

	/** One shipped road. It joins two marks on the world chart. */
	class FSolidRoad
	{
	public:
		FSolidRoad(FSolidWorldChartMark InFrom, FSolidWorldChartMark InTo, EType InType)
			: From(MoveTemp(InFrom))
			, To(MoveTemp(InTo))
			, Type(InType)
		{
		}

		/**
		 * Copy the chart position onto each end when the chart has that mark.
		 * False when either end is missing.
		 */
		bool Locate(const TArray<FSolidWorldChartMark>& ChartMarks);

		/**
		 * Legacy support only. Type, town id, and stronghold name.
		 * A stronghold cell is still empty. An unknown town leaves that end blank.
		 */
		FDefinition ToDefinition() const;

		FSolidWorldChartMark From;
		FSolidWorldChartMark To;
		EType Type = EType::Trail;
	};

	/** A town mark. The chart places it. */
	inline FSolidWorldChartMark TownMark(const int32 TownId)
	{
		FSolidWorldChartMark Mark;
		Mark.Kind = ESolidWorldChartMarkKind::Town;
		Mark.Name = SolidTowns::Definitions[TownId].Name;
		return Mark;
	}

	/** A stronghold mark. The chart places it. */
	inline FSolidWorldChartMark StrongholdMark(const ESolidStrongholdSize Size, const ESolidStrongholdType Type)
	{
		FSolidWorldChartMark Mark;
		Mark.Kind = ESolidWorldChartMarkKind::Stronghold;
		Mark.StrongholdSize = Size;
		Mark.StrongholdType = Type;
		Mark.Name = SolidStronghold::Label(Size, Type);
		return Mark;
	}

	/** Copy the chart mark's position onto this end when the chart has that mark. */
	inline bool LocateMark(const TArray<FSolidWorldChartMark>& ChartMarks, FSolidWorldChartMark& End)
	{
		for (const FSolidWorldChartMark& ChartMark : ChartMarks)
		{
			if (ChartMark.Kind != End.Kind)
			{
				continue;
			}
			if (End.Kind == ESolidWorldChartMarkKind::Stronghold)
			{
				if (ChartMark.StrongholdSize != End.StrongholdSize
					|| ChartMark.StrongholdType != End.StrongholdType)
				{
					continue;
				}
			}
			else
			{
				FString EndName = End.Name;
				FString ChartName = ChartMark.Name;
				EndName.ReplaceInline(TEXT(" "), TEXT(""));
				ChartName.ReplaceInline(TEXT(" "), TEXT(""));
				if (!EndName.Equals(ChartName, ESearchCase::IgnoreCase))
				{
					continue;
				}
			}
			End.PositionCm = ChartMark.PositionCm;
			return true;
		}
		return false;
	}

	inline bool FSolidRoad::Locate(const TArray<FSolidWorldChartMark>& ChartMarks)
	{
		const bool bFrom = LocateMark(ChartMarks, From);
		const bool bTo = LocateMark(ChartMarks, To);
		return bFrom && bTo;
	}

	struct FEndpoint
	{
		/** Town location id. INDEX_NONE when this end is a stronghold. */
		int32 LocationId = INDEX_NONE;
		/** True when the end is a stronghold name, such as MeagreDungeon. */
		bool bStronghold = false;
		/** True once that name has been found on a Stronghold line in the same file. */
		bool bLocated = false;
		/** Map cell of that stronghold. X is the column, Y is the row, counting from 0. */
		FIntPoint Cell = FIntPoint::ZeroValue;
		/** The name from the road line. MeagreDungeon and Meagre Dungeon are the same name. */
		FString StrongholdName;
	};

	/** Town name, or the stronghold name, for this end. */
	FString ChartRoadEndName(const FEndpoint& End);

	struct FDefinition
	{
		FEndpoint From;
		FEndpoint To;
		EType Type = EType::Trail;
	};

	inline FDefinition FSolidRoad::ToDefinition() const
	{
		auto FillEnd = [](const FSolidWorldChartMark& Mark, FEndpoint& Out)
		{
			Out = FEndpoint();
			if (Mark.Kind == ESolidWorldChartMarkKind::Town)
			{
				const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Mark.Name);
				if (Town == nullptr)
				{
					return;
				}
				Out.LocationId = Town->Id;
				return;
			}
			if (Mark.Kind == ESolidWorldChartMarkKind::Stronghold)
			{
				Out.bStronghold = true;
				Out.StrongholdName = Mark.Name;
			}
		};

		FDefinition Result;
		Result.Type = Type;
		FillEnd(From, Result.From);
		FillEnd(To, Result.To);
		return Result;
	}

	struct FRoute
	{
		FDefinition Definition;
		/** Cell centres the strip follows, including both ends. Empty when an end is missing. */
		TArray<FIntPoint> Cells;
		/** Terrain points on the straight line. Empty when the route is still the cell search. */
		TArray<FIntPoint> Points;
	};

	/** One painted segment, in map cells. A point span uses the same cell twice. */
	struct FSpan
	{
		FIntPoint From = FIntPoint::ZeroValue;
		FIntPoint To = FIntPoint::ZeroValue;
		float HalfWidthCm = 0.f;
	};

	/** One ordinary step. Town and road cost nothing. A banned step is BannedStep. */
	inline constexpr int32 StepCost = 8;
	/** The route goes around this ground. */
	inline constexpr int32 BannedStep = -1;
	/** Cells around a river, diagonals included, where a turn pays RiverBendCost. */
	inline constexpr int32 RiverBendReach = 2;
	/** Less than a step, so the route does not lengthen. A straight crossing wins the tie. */
	inline constexpr int32 RiverBendCost = 4;

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

	/**
	 * Cost of entering this ground. BannedStep when the route goes around it.
	 * A river step costs its crossing only when bSquareToRiver is set.
	 * Any other step on the river is banned.
	 */
	inline int32 EnterCost(const EType Type, const ESolidBiome Biome, const bool bSquareToRiver = false)
	{
		const bool bRoad = Type == EType::Road;
		const bool bHighway = Type == EType::Highway;
		switch (Biome)
		{
		case ESolidBiome::Grassland:
			return StepCost;
		case ESolidBiome::Forest:
		case ESolidBiome::Desert:
		case ESolidBiome::Swamp:
			if (bHighway)
			{
				return 32;
			}
			if (bRoad)
			{
				return 16;
			}
			return StepCost;
		case ESolidBiome::Town:
		case ESolidBiome::Road:
			return 0;
		case ESolidBiome::River:
			if (!bSquareToRiver)
			{
				return BannedStep;
			}
			if (bHighway)
			{
				return 64;
			}
			if (bRoad)
			{
				return 32;
			}
			return 16;
		case ESolidBiome::Mountain:
		case ESolidBiome::Sea:
		default:
			return BannedStep;
		}
	}

	/**
	 * "Road: Iglin -> Relion, track" or "Road: MeagreDungeon -> Iglin, trail".
	 * An end is a town name or a stronghold name.
	 * A stronghold name is resolved to the cell of the Stronghold line with that name.
	 * Brackets around a name are optional. False when the line is not a road,
	 * or an end is not one of those.
	 */
	bool Parse(const FString& Line, FDefinition& Out);

	/**
	 * Route from one end to the other.
	 * Road supplies the ends and the type when it is set. Definition and OutCells remain for a line that has no road.
	 * OutPoints is the terrain points of that line, including one detour around a single banned patch. Empty when the cell search runs.
	 * @return False when an end is missing, or no path exists.
	 */
	bool Solve(
		const USolidWorldChart& Map,
		const FDefinition& Definition,
		TArray<FIntPoint>& OutCells,
		const FSolidRoad* Road,
		TArray<FIntPoint>& OutPoints);

	float DistanceToSegmentCm(const FVector2D& Point, const FVector2D& A, const FVector2D& B);
}
