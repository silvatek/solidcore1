#include "SolidRoad.h"
#include "SolidCore1.h"
#include "SolidRoads.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWallgate.h"
#include "SolidWorldChart.h"

namespace SolidRoad
{
	namespace
	{
		bool RoadParseEndpoint(const FString& Text, FEndpoint& Out)
		{
			FString Token = Text.TrimStartAndEnd();
			if (Token.Len() >= 2 && Token[0] == TEXT('(') && Token[Token.Len() - 1] == TEXT(')'))
			{
				Token = Token.Mid(1, Token.Len() - 2).TrimStartAndEnd();
			}
			Out = FEndpoint();
			if (Token.IsEmpty())
			{
				return false;
			}

			if (const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Token))
			{
				Out.LocationId = Town->Id;
				return true;
			}

			ESolidStrongholdType HoldType = ESolidStrongholdType::Dungeon;
			ESolidStrongholdSize HoldSize = ESolidStrongholdSize::Meagre;
			if (SolidStronghold::FromToken(Token, HoldType, HoldSize))
			{
				Out.bStronghold = true;
				Out.StrongholdName = Token;
				return true;
			}
			return false;
		}

		bool RoadParseType(const FString& Name, EType& Out)
		{
			if (Name.Equals(TEXT("trail"), ESearchCase::IgnoreCase))
			{
				Out = EType::Trail;
			}
			else if (Name.Equals(TEXT("track"), ESearchCase::IgnoreCase))
			{
				Out = EType::Track;
			}
			else if (Name.Equals(TEXT("road"), ESearchCase::IgnoreCase))
			{
				Out = EType::Road;
			}
			else if (Name.Equals(TEXT("highway"), ESearchCase::IgnoreCase))
			{
				Out = EType::Highway;
			}
			else
			{
				return false;
			}
			return true;
		}

		bool PointIsRiver(const USolidWorldChart& Map, const int32 X, const int32 Y)
		{
			if (X < 0 || Y < 0 || X >= USolidWorldChart::MapWidth || Y >= USolidWorldChart::MapHeight)
			{
				return false;
			}
			return Map.GetPointBiomeAtCell(X, Y) == ESolidBiome::River;
		}

		/**
		 * A river step is allowed when it runs across the channel, not along it.
		 * A cell with no river beside it is a pond, so the step crosses it.
		 */
		bool StepIsSquareToRiver(const USolidWorldChart& Map, const FIntPoint& From, const FIntPoint& To)
		{
			const bool bAlongY = PointIsRiver(Map, To.X, To.Y - 1) || PointIsRiver(Map, To.X, To.Y + 1);
			const bool bAlongX = PointIsRiver(Map, To.X + 1, To.Y) || PointIsRiver(Map, To.X - 1, To.Y);
			if (!bAlongY && !bAlongX)
			{
				return true;
			}
			if (bAlongY && bAlongX)
			{
				return false;
			}
			const FIntPoint Step(To.X - From.X, To.Y - From.Y);
			if (bAlongY)
			{
				return Step.X != 0 && Step.Y == 0;
			}
			return Step.Y != 0 && Step.X == 0;
		}

		bool CellBesideRiver(const USolidWorldChart& Map, const int32 X, const int32 Y)
		{
			for (int32 OffsetY = -RiverBendReach; OffsetY <= RiverBendReach; ++OffsetY)
			{
				for (int32 OffsetX = -RiverBendReach; OffsetX <= RiverBendReach; ++OffsetX)
				{
					const int32 NextX = X + OffsetX;
					const int32 NextY = Y + OffsetY;
					if (NextX < 0 || NextY < 0 || NextX >= USolidWorldChart::MapWidth || NextY >= USolidWorldChart::MapHeight)
					{
						continue;
					}
					if (Map.GetMarkerAtCell(NextX, NextY) == USolidWorldChart::RiverMarker)
					{
						return true;
					}
				}
			}
			return false;
		}

		/** How far a corner may sit past a river cell and still be pulled back toward it. */
		constexpr int32 RiverSpurWindow = 6;

		int32 ChebyshevToRiver(const TArray<FIntPoint>& Cells, const TArray<int32>& RiverIndices, const FIntPoint& Cell)
		{
			int32 Best = MAX_int32;
			for (const int32 RiverIndex : RiverIndices)
			{
				const FIntPoint& River = Cells[RiverIndex];
				Best = FMath::Min(Best, FMath::Max(FMath::Abs(Cell.X - River.X), FMath::Abs(Cell.Y - River.Y)));
			}
			return Best;
		}

		/**
		 * On the far side of a crossing, tie-breaks leave a one-cell spur past the bend.
		 * Slide that bend back toward the river when the ground beside it is open.
		 */
		void SlideSpursAfterRiver(const USolidWorldChart& Map, const EType Type, TArray<FIntPoint>& Cells)
		{
			TArray<int32> Rivers;
			for (int32 Index = 0; Index < Cells.Num(); ++Index)
			{
				if (Map.GetMarkerAtCell(Cells[Index].X, Cells[Index].Y) == USolidWorldChart::RiverMarker)
				{
					Rivers.Add(Index);
				}
			}
			if (Rivers.Num() == 0)
			{
				return;
			}

			for (int32 Guard = 0; Guard < Cells.Num(); ++Guard)
			{
				bool bChanged = false;
				for (int32 Index = 1; Index + 1 < Cells.Num(); ++Index)
				{
					const FIntPoint In = Cells[Index] - Cells[Index - 1];
					const FIntPoint Out = Cells[Index + 1] - Cells[Index];
					if (In == Out
						|| FMath::Abs(In.X) + FMath::Abs(In.Y) != 1
						|| FMath::Abs(Out.X) + FMath::Abs(Out.Y) != 1)
					{
						continue;
					}

					bool bAfterRiver = false;
					for (const int32 RiverIndex : Rivers)
					{
						if (Index > RiverIndex && Index - RiverIndex <= RiverSpurWindow)
						{
							bAfterRiver = true;
							break;
						}
					}
					if (!bAfterRiver)
					{
						continue;
					}

					const FIntPoint Side = Cells[Index - 1] + Out;
					if (Side + In != Cells[Index + 1])
					{
						continue;
					}
					if (ChebyshevToRiver(Cells, Rivers, Side) >= ChebyshevToRiver(Cells, Rivers, Cells[Index]))
					{
						continue;
					}
					if (Side.X < 0 || Side.Y < 0 || Side.X >= USolidWorldChart::MapWidth || Side.Y >= USolidWorldChart::MapHeight)
					{
						continue;
					}
					if (Map.GetMarkerAtCell(Side.X, Side.Y) == USolidWorldChart::RiverMarker)
					{
						continue;
					}
					if (EnterCost(Type, Map.GetPointBiomeAtCell(Side.X, Side.Y)) < 0)
					{
						continue;
					}
					if (Cells.Contains(Side))
					{
						continue;
					}

					Cells[Index] = Side;
					bChanged = true;
					break;
				}
				if (!bChanged)
				{
					break;
				}
			}
		}

		void RoadCollectEndpoint(const USolidWorldChart& Map, const FEndpoint& End, TArray<FIntPoint>& OutCells)
		{
			OutCells.Reset();
			if (End.bStronghold)
			{
				if (End.bLocated)
				{
					OutCells.Add(End.Cell);
				}
				return;
			}

			FSolidWorldLocation Location;
			if (Map.FindLocation(End.LocationId, Location))
			{
				FIntPoint Marker = FIntPoint::ZeroValue;
				if (SolidWallgate::MarkerCell(&Map, End.LocationId, Marker))
				{
					OutCells.Add(Marker);
				}
				else
				{
					OutCells = Location.Cells;
				}
			}
		}

		void TerrainLine(const FIntPoint& From, const FIntPoint& To, TArray<FIntPoint>& OutPoints)
		{
			OutPoints.Reset();
			int32 X0 = From.X;
			int32 Y0 = From.Y;
			const int32 X1 = To.X;
			const int32 Y1 = To.Y;
			const int32 Dx = FMath::Abs(X1 - X0);
			const int32 Dy = FMath::Abs(Y1 - Y0);
			const int32 Sx = X0 < X1 ? 1 : -1;
			const int32 Sy = Y0 < Y1 ? 1 : -1;
			int32 Err = Dx - Dy;
			while (true)
			{
				OutPoints.Add(FIntPoint(X0, Y0));
				if (X0 == X1 && Y0 == Y1)
				{
					break;
				}
				const int32 Twice = Err * 2;
				if (Twice > -Dy)
				{
					Err -= Dy;
					X0 += Sx;
				}
				if (Twice < Dx)
				{
					Err += Dx;
					Y0 += Sy;
				}
			}
		}

		bool RiverPoint(const USolidWorldChart& Map, const int32 X, const int32 Y)
		{
			if (X < 0 || Y < 0 || X >= USolidWorldChart::PointAxis || Y >= USolidWorldChart::PointAxis)
			{
				return false;
			}
			return Map.GetPointBiome(FIntPoint(X, Y)) == ESolidBiome::River;
		}

		bool LineStepAllowed(const USolidWorldChart& Map, const EType Type, const FIntPoint& From, const FIntPoint& To)
		{
			const ESolidBiome Biome = Map.GetPointBiome(To);
			if (Biome != ESolidBiome::River)
			{
				return EnterCost(Type, Biome, false) >= 0;
			}
			const int32 AlongY = (RiverPoint(Map, To.X, To.Y + 1) ? 1 : 0) + (RiverPoint(Map, To.X, To.Y - 1) ? 1 : 0);
			const int32 AlongX = (RiverPoint(Map, To.X + 1, To.Y) ? 1 : 0) + (RiverPoint(Map, To.X - 1, To.Y) ? 1 : 0);
			const FIntPoint Step(To.X - From.X, To.Y - From.Y);
			bool bSquare = true;
			if (AlongY == 0 && AlongX == 0)
			{
				bSquare = true;
			}
			else if (AlongY > AlongX)
			{
				bSquare = Step.X != 0 && Step.Y == 0;
			}
			else if (AlongX > AlongY)
			{
				bSquare = Step.Y != 0 && Step.X == 0;
			}
			else
			{
				bSquare = false;
			}
			return EnterCost(Type, Biome, bSquare) >= 0;
		}

		bool CellsForPoints(
			const USolidWorldChart& Map,
			const TArray<FIntPoint>& Points,
			TArray<FIntPoint>& OutCells)
		{
			OutCells.Reset();
			FVector2D Min = FVector2D::ZeroVector;
			FVector2D Max = FVector2D::ZeroVector;
			USolidWorldChart::DefaultWorldBounds(Min, Max);
			for (const FIntPoint& Point : Points)
			{
				FVector2D World = FVector2D::ZeroVector;
				if (!Map.TerrainPointToWorld(Point, Min, Max, World))
				{
					OutCells.Reset();
					return false;
				}
				FIntPoint Cell = FIntPoint::ZeroValue;
				if (!Map.CellAtWorld(World.X, World.Y, Min, Max, Cell))
				{
					OutCells.Reset();
					return false;
				}
				if (OutCells.Num() == 0 || OutCells.Last() != Cell)
				{
					OutCells.Add(Cell);
				}
			}
			return true;
		}

		bool OpenStraightCells(
			const USolidWorldChart& Map,
			const FSolidRoad* Road,
			TArray<FIntPoint>& OutCells,
			TArray<FIntPoint>& OutPoints)
		{
			OutPoints.Reset();
			if (Road == nullptr)
			{
				return false;
			}
			FVector2D Min = FVector2D::ZeroVector;
			FVector2D Max = FVector2D::ZeroVector;
			USolidWorldChart::DefaultWorldBounds(Min, Max);
			FIntPoint FromPoint = FIntPoint::ZeroValue;
			FIntPoint ToPoint = FIntPoint::ZeroValue;
			if (!Map.TerrainPointAt(Road->From.PositionCm.X, Road->From.PositionCm.Y, Min, Max, FromPoint)
				|| !Map.TerrainPointAt(Road->To.PositionCm.X, Road->To.PositionCm.Y, Min, Max, ToPoint))
			{
				return false;
			}

			TerrainLine(FromPoint, ToPoint, OutPoints);
			for (int32 Index = 1; Index < OutPoints.Num(); ++Index)
			{
				if (!LineStepAllowed(Map, Road->Type, OutPoints[Index - 1], OutPoints[Index]))
				{
					OutPoints.Reset();
					return false;
				}
			}

			if (!CellsForPoints(Map, OutPoints, OutCells))
			{
				OutPoints.Reset();
				return false;
			}
			return OutCells.Num() > 0;
		}
	}

	bool Parse(const FString& Line, FDefinition& Out)
	{
		const FString Trimmed = Line.TrimStartAndEnd();
		if (!Trimmed.StartsWith(TEXT("Road:"), ESearchCase::IgnoreCase))
		{
			return false;
		}

		const FString Rest = Trimmed.Mid(5).TrimStartAndEnd();
		FString Ends;
		FString TypeName;
		if (!Rest.Split(TEXT(","), &Ends, &TypeName))
		{
			return false;
		}
		TypeName = TypeName.TrimStartAndEnd();

		FString FromText;
		FString ToText;
		if (!Ends.Split(TEXT("->"), &FromText, &ToText))
		{
			return false;
		}
		if (!RoadParseEndpoint(FromText, Out.From) || !RoadParseEndpoint(ToText, Out.To))
		{
			return false;
		}
		return RoadParseType(TypeName, Out.Type);
	}

	bool Solve(
		const USolidWorldChart& Map,
		const FDefinition& Definition,
		TArray<FIntPoint>& OutCells,
		const FSolidRoad* Road,
		TArray<FIntPoint>& OutPoints)
	{
		OutCells.Reset();
		OutPoints.Reset();
		const EType Type = Road != nullptr ? Road->Type : Definition.Type;

		TArray<FIntPoint> Starts;
		TArray<FIntPoint> Goals;
		RoadCollectEndpoint(Map, Definition.From, Starts);
		RoadCollectEndpoint(Map, Definition.To, Goals);
		if (OpenStraightCells(Map, Road, OutCells, OutPoints))
		{
			return true;
		}
		OutPoints.Reset();
		if (Starts.Num() == 0 || Goals.Num() == 0)
		{
			return false;
		}

		const int32 Width = USolidWorldChart::MapWidth;
		const int32 Height = USolidWorldChart::MapHeight;
		const int32 Count = Width * Height;
		const int32 Infinite = MAX_int32 / 4;

		TArray<int32> Cost;
		TArray<int32> Previous;
		TArray<uint8> Done;
		Cost.Init(Infinite, Count);
		Previous.Init(INDEX_NONE, Count);
		Done.Init(0, Count);

		TArray<uint8> Goal;
		Goal.Init(0, Count);
		for (const FIntPoint& Cell : Goals)
		{
			if (Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Width && Cell.Y < Height)
			{
				Goal[Cell.Y * Width + Cell.X] = 1;
			}
		}
		for (const FIntPoint& Cell : Starts)
		{
			if (Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Width && Cell.Y < Height)
			{
				Cost[Cell.Y * Width + Cell.X] = 0;
			}
		}

		int32 GoalIndex = INDEX_NONE;
		for (int32 Step = 0; Step < Count; ++Step)
		{
			int32 Best = INDEX_NONE;
			int32 BestCost = Infinite;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				if (Done[Index] == 0 && Cost[Index] < BestCost)
				{
					Best = Index;
					BestCost = Cost[Index];
				}
			}
			if (Best == INDEX_NONE || BestCost >= Infinite)
			{
				break;
			}

			Done[Best] = 1;
			if (Goal[Best] != 0)
			{
				GoalIndex = Best;
				break;
			}

			const int32 X = Best % Width;
			const int32 Y = Best / Width;
			const bool bHasIn = Previous[Best] != INDEX_NONE;
			const int32 PrevX = bHasIn ? Previous[Best] % Width : X;
			const int32 PrevY = bHasIn ? Previous[Best] / Width : Y;
			const FIntPoint In(X - PrevX, Y - PrevY);
			const FIntPoint Neighbors[] = {
				FIntPoint(X, Y - 1),
				FIntPoint(X + 1, Y),
				FIntPoint(X, Y + 1),
				FIntPoint(X - 1, Y),
			};
			for (const FIntPoint& Next : Neighbors)
			{
				if (Next.X < 0 || Next.Y < 0 || Next.X >= Width || Next.Y >= Height)
				{
					continue;
				}
				const int32 NextIndex = Next.Y * Width + Next.X;
				if (Done[NextIndex] != 0)
				{
					continue;
				}
				const FIntPoint Out(Next.X - X, Next.Y - Y);
				int32 Bend = 0;
				if (bHasIn && In != Out && CellBesideRiver(Map, X, Y))
				{
					Bend = RiverBendCost;
				}
				const ESolidBiome NextBiome = Map.GetPointBiomeAtCell(Next.X, Next.Y);
				const bool bSquareToRiver = NextBiome == ESolidBiome::River
					&& StepIsSquareToRiver(Map, FIntPoint(X, Y), Next);
				const int32 Enter = EnterCost(Type, NextBiome, bSquareToRiver);
				if (Enter < 0)
				{
					continue;
				}
				const int32 NextCost = BestCost + Enter + Bend;
				if (NextCost < Cost[NextIndex])
				{
					Cost[NextIndex] = NextCost;
					Previous[NextIndex] = Best;
				}
			}
		}

		if (GoalIndex == INDEX_NONE)
		{
			return false;
		}

		TArray<FIntPoint> Reversed;
		for (int32 Index = GoalIndex; Index != INDEX_NONE; Index = Previous[Index])
		{
			Reversed.Add(FIntPoint(Index % Width, Index / Width));
			if (Reversed.Num() > Count)
			{
				OutCells.Reset();
				return false;
			}
		}
		for (int32 Index = Reversed.Num() - 1; Index >= 0; --Index)
		{
			OutCells.Add(Reversed[Index]);
		}
		SlideSpursAfterRiver(Map, Type, OutCells);
		return OutCells.Num() > 0;
	}

	float DistanceToSegmentCm(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D Span = B - A;
		const float LengthSq = Span.SizeSquared();
		if (LengthSq <= KINDA_SMALL_NUMBER)
		{
			return FVector2D::Distance(Point, A);
		}
		const float Along = FMath::Clamp(FVector2D::DotProduct(Point - A, Span) / LengthSq, 0.f, 1.f);
		return FVector2D::Distance(Point, A + Span * Along);
	}

	FString ChartRoadEndName(const FEndpoint& End)
	{
		if (End.bStronghold)
		{
			return End.StrongholdName;
		}
		return SolidTowns::NameFor(End.LocationId);
	}

	void LocateStrongholdEnd(const TMap<int32, FSolidWorldBang>& Marks, FEndpoint& End)
	{
		if (!End.bStronghold)
		{
			return;
		}
		FString Want = End.StrongholdName;
		Want.ReplaceInline(TEXT(" "), TEXT(""));
		for (const TPair<int32, FSolidWorldBang>& Pair : Marks)
		{
			const FSolidWorldBang& Mark = Pair.Value;
			if (Mark.Kind != ESolidWorldBang::Stronghold)
			{
				continue;
			}
			FString Have = Mark.Name;
			Have.ReplaceInline(TEXT(" "), TEXT(""));
			if (!Have.Equals(Want, ESearchCase::IgnoreCase))
			{
				continue;
			}
			End.Cell = Mark.Cell;
			End.bLocated = true;
			return;
		}
	}
}

namespace
{
	FString CompactName(FString Name)
	{
		Name.ReplaceInline(TEXT(" "), TEXT(""));
		return Name;
	}

	/** Town, stronghold, site, or neither. A site is reported and is not an end. */
	enum class EChartRoadEnd : uint8
	{
		Missing,
		Town,
		Stronghold,
		Site,
	};

	EChartRoadEnd RoadEndKind(const TArray<FSolidWorldChartMark>& Marks, const FString& Name)
	{
		const FString Compact = CompactName(Name);
		if (Compact.IsEmpty())
		{
			return EChartRoadEnd::Missing;
		}

		EChartRoadEnd Found = EChartRoadEnd::Missing;
		for (const FSolidWorldChartMark& Mark : Marks)
		{
			if (Mark.Kind == ESolidWorldChartMarkKind::Town
				&& Mark.Name.Equals(Name.TrimStartAndEnd(), ESearchCase::IgnoreCase))
			{
				return EChartRoadEnd::Town;
			}
			if (Mark.Kind == ESolidWorldChartMarkKind::Stronghold
				&& CompactName(Mark.Name).Equals(Compact, ESearchCase::IgnoreCase))
			{
				Found = EChartRoadEnd::Stronghold;
			}
			else if (Found != EChartRoadEnd::Stronghold
				&& Mark.Kind == ESolidWorldChartMarkKind::StrongholdSite
				&& Mark.Name.Equals(Name.TrimStartAndEnd(), ESearchCase::IgnoreCase))
			{
				Found = EChartRoadEnd::Site;
			}
		}
		return Found;
	}

	FString RoadEndSentence(const EChartRoadEnd Kind, const FString& Name)
	{
		const FString Trimmed = Name.TrimStartAndEnd();
		if (Kind == EChartRoadEnd::Site)
		{
			return FString::Printf(TEXT("%s is a stronghold site, not a road end."), *Trimmed);
		}
		if (Kind == EChartRoadEnd::Missing)
		{
			return FString::Printf(TEXT("%s is not a town or stronghold on the chart."), *Trimmed);
		}
		return FString();
	}
}

const TCHAR* USolidWorldChart::RoadTypeName(const ESolidWorldChartRoadType Type)
{
	switch (Type)
	{
	case ESolidWorldChartRoadType::Track:
		return TEXT("track");
	case ESolidWorldChartRoadType::Road:
		return TEXT("road");
	case ESolidWorldChartRoadType::Highway:
		return TEXT("highway");
	case ESolidWorldChartRoadType::Trail:
	default:
		return TEXT("trail");
	}
}

ESolidWorldChartRoadType USolidWorldChart::ChartRoadType(const SolidRoad::EType Type)
{
	switch (Type)
	{
	case SolidRoad::EType::Track:
		return ESolidWorldChartRoadType::Track;
	case SolidRoad::EType::Road:
		return ESolidWorldChartRoadType::Road;
	case SolidRoad::EType::Highway:
		return ESolidWorldChartRoadType::Highway;
	case SolidRoad::EType::Trail:
	default:
		return ESolidWorldChartRoadType::Trail;
	}
}

FString USolidWorldChart::RoadEndError(const FString& From, const FString& To) const
{
	TArray<FString> Sentences;
	const EChartRoadEnd FromKind = RoadEndKind(Marks, From);
	const EChartRoadEnd ToKind = RoadEndKind(Marks, To);
	if (FromKind == EChartRoadEnd::Site || FromKind == EChartRoadEnd::Missing)
	{
		Sentences.Add(RoadEndSentence(FromKind, From));
	}
	if (ToKind == EChartRoadEnd::Site || ToKind == EChartRoadEnd::Missing)
	{
		Sentences.Add(RoadEndSentence(ToKind, To));
	}
	return FString::Join(Sentences, TEXT(" "));
}

bool USolidWorldChart::AddRoad(
	const FString& From,
	const FString& To,
	const ESolidWorldChartRoadType Type,
	FString& OutError)
{
	OutError = RoadEndError(From, To);
	if (From.TrimStartAndEnd().IsEmpty() || To.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("A road needs two names.");
		return false;
	}

	PushUndo();
	FSolidWorldChartRoad Road;
	Road.FromName = From.TrimStartAndEnd();
	Road.ToName = To.TrimStartAndEnd();
	Road.Type = Type;
	Roads.Add(Road);
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::RouteForRoad(const int32 RoadIndex, TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	if (!Roads.IsValidIndex(RoadIndex))
	{
		return false;
	}
	if (!EnsureQuery())
	{
		return false;
	}

	// A name that is not a town or a stronghold does not parse, so the strip is empty.
	auto LineFor = [](const FSolidWorldChartRoad& Road)
	{
		return FString::Printf(
			TEXT("Road: %s -> %s, %s"),
			*Road.FromName,
			*Road.ToName,
			RoadTypeName(Road.Type));
	};

	SolidRoad::FDefinition ThisRoad;
	if (!SolidRoad::Parse(LineFor(Roads[RoadIndex]), ThisRoad))
	{
		return true;
	}

	int32 ParsedIndex = 0;
	for (int32 Index = 0; Index < RoadIndex; ++Index)
	{
		SolidRoad::FDefinition Earlier;
		if (SolidRoad::Parse(LineFor(Roads[Index]), Earlier))
		{
			++ParsedIndex;
		}
	}

	if (GetRoads().IsValidIndex(ParsedIndex))
	{
		OutCells = GetRoads()[ParsedIndex].Cells;
	}
	return true;
}

void USolidWorldChart::BuildListedRoads(
	const TArray<FSolidWorldChartMark>& ChartMarks,
	const TMap<int32, FSolidWorldBang>& ChartBangMarks) const
{
	auto EndReady = [](const SolidRoad::FEndpoint& End)
	{
		return End.bStronghold || End.LocationId != INDEX_NONE;
	};

	ResetRoads();
	for (SolidRoad::FSolidRoad Road : SolidRoad::Roads)
	{
		Road.Locate(ChartMarks);
		SolidRoad::FDefinition Definition = Road.ToDefinition();
		if (!EndReady(Definition.From) || !EndReady(Definition.To))
		{
			UE_LOG(LogSolid, Warning, TEXT("WorldChart: ignored road %s -> %s."), *Road.From.Name, *Road.To.Name);
			continue;
		}
		if (Road.From.Kind == ESolidWorldChartMarkKind::Stronghold)
		{
			SolidRoad::LocateStrongholdEnd(ChartBangMarks, Definition.From);
		}
		if (Road.To.Kind == ESolidWorldChartMarkKind::Stronghold)
		{
			SolidRoad::LocateStrongholdEnd(ChartBangMarks, Definition.To);
		}
		BuildRoad(Definition, &Road);
	}
}

void USolidWorldChart::BuildRoads(const TArray<SolidRoad::FDefinition>& Definitions) const
{
	ResetRoads();

	for (const SolidRoad::FDefinition& Definition : Definitions)
	{
		BuildRoad(Definition, nullptr);
	}
}

void USolidWorldChart::ResetRoads() const
{
	SolvedRoads.Reset();
	RoadSpans.Reset();
	RoadSpansByCell.Reset();
	RoadSpansByCell.SetNum(MapWidth * MapHeight);
}

void USolidWorldChart::BuildRoad(const SolidRoad::FDefinition& Definition, const SolidRoad::FSolidRoad* Road) const
{
	// Prefer Road to Definition for every later change here. The aim is to remove Definition.
	if (RoadSpansByCell.Num() != MapWidth * MapHeight)
	{
		RoadSpansByCell.SetNum(MapWidth * MapHeight);
	}

	SolidRoad::FRoute Route;
	Route.Definition = Definition;
	if (!SolidRoad::Solve(*this, Definition, Route.Cells, Road, Route.Points))
	{
		const FString From = SolidRoad::ChartRoadEndName(Definition.From);
		const FString To = SolidRoad::ChartRoadEndName(Definition.To);
		UE_LOG(LogSolid, Warning, TEXT("WorldMap: no route for road %s -> %s."), *From, *To);
	}
	SolvedRoads.Add(Route);

	if (Route.Cells.Num() == 0)
	{
		return;
	}

	const float HalfWidth = SolidRoad::WidthCm(Road != nullptr ? Road->Type : Definition.Type) * 0.5f;
	const int32 SpanCount = Route.Cells.Num() == 1 ? 1 : Route.Cells.Num() - 1;
	for (int32 SpanIndex = 0; SpanIndex < SpanCount; ++SpanIndex)
	{
		SolidRoad::FSpan Span;
		Span.From = Route.Cells[SpanIndex];
		Span.To = Route.Cells[FMath::Min(SpanIndex + 1, Route.Cells.Num() - 1)];
		Span.HalfWidthCm = HalfWidth;
		const int32 Stored = RoadSpans.Add(Span);

		auto Touch = [this, Stored](const FIntPoint& Cell)
		{
			for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
			{
				for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
				{
					const int32 X = Cell.X + OffsetX;
					const int32 Y = Cell.Y + OffsetY;
					if (X < 0 || Y < 0 || X >= MapWidth || Y >= MapHeight)
					{
						continue;
					}
					RoadSpansByCell[Y * MapWidth + X].AddUnique(Stored);
				}
			}
		};
		Touch(Span.From);
		Touch(Span.To);
	}
}

bool USolidWorldChart::IsOnRoad(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	if (!EnsureQuery() || RoadSpans.Num() == 0 || RoadSpansByCell.Num() != MapWidth * MapHeight)
	{
		return false;
	}

	int32 MapX = 0;
	int32 MapY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, MapX, MapY))
	{
		return false;
	}

	const FVector2D World(WorldX, WorldY);
	for (const int32 SpanIndex : RoadSpansByCell[MapY * MapWidth + MapX])
	{
		const SolidRoad::FSpan& Span = RoadSpans[SpanIndex];
		FVector2D From = FVector2D::ZeroVector;
		FVector2D To = FVector2D::ZeroVector;
		if (!CellToWorldXY(Span.From, WorldMinXY, WorldMaxXY, From)
			|| !CellToWorldXY(Span.To, WorldMinXY, WorldMaxXY, To))
		{
			continue;
		}
		if (SolidRoad::DistanceToSegmentCm(World, From, To) <= Span.HalfWidthCm)
		{
			return true;
		}
	}
	return false;
}
