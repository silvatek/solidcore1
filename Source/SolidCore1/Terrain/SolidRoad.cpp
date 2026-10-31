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
				const ESolidBiome CellBiome = Map.GetBiomeAtCell(Cell.X, Cell.Y);
				if (CellBiome == ESolidBiome::Mountain || CellBiome == ESolidBiome::Sea)
				{
					continue;
				}
				if (OutCells.Num() == 0 || OutCells.Last() != Cell)
				{
					OutCells.Add(Cell);
				}
			}
			return true;
		}

		bool OnTerrainPoint(const FIntPoint& Point)
		{
			return Point.X >= 0 && Point.Y >= 0
				&& Point.X < USolidWorldChart::PointAxis
				&& Point.Y < USolidWorldChart::PointAxis;
		}

		int32 PointKey(const FIntPoint& Point)
		{
			return Point.Y * USolidWorldChart::PointAxis + Point.X;
		}

		const FIntPoint PointSteps[] = {
			FIntPoint(1, 0),
			FIntPoint(-1, 0),
			FIntPoint(0, 1),
			FIntPoint(0, -1),
			FIntPoint(1, 1),
			FIntPoint(1, -1),
			FIntPoint(-1, 1),
			FIntPoint(-1, -1),
		};

		/** Rings around one banned run. Another ring would leave that patch. */
		constexpr int32 DetourReach = 8;

		bool BesideMountainOrSea(const USolidWorldChart& Map, const FIntPoint& Point)
		{
			for (const FIntPoint& Step : PointSteps)
			{
				const FIntPoint Side = Point + Step;
				if (!OnTerrainPoint(Side))
				{
					continue;
				}
				const ESolidBiome Biome = Map.GetPointBiome(Side);
				if (Biome == ESolidBiome::Mountain || Biome == ESolidBiome::Sea)
				{
					return true;
				}
			}
			return false;
		}

		bool DetourAroundPatch(
			const USolidWorldChart& Map,
			const EType Type,
			const TArray<FIntPoint>& Line,
			const int32 First,
			const int32 Last,
			TArray<FIntPoint>& OutPoints)
		{
			if (First < 1 || Last + 1 >= Line.Num())
			{
				return false;
			}

			const FIntPoint Before = Line[First - 1];
			const FIntPoint After = Line[Last + 1];
			TSet<int32> Banned;
			TArray<FIntPoint> Frontier;
			bool bClearMountain = false;
			for (int32 Index = First; Index <= Last; ++Index)
			{
				Banned.Add(PointKey(Line[Index]));
				Frontier.Add(Line[Index]);
				const ESolidBiome Biome = Map.GetPointBiome(Line[Index]);
				if (Biome == ESolidBiome::Mountain || Biome == ESolidBiome::Sea)
				{
					bClearMountain = true;
				}
			}

			TSet<int32> Beside;
			for (int32 Ring = 0; Ring < DetourReach; ++Ring)
			{
				TArray<FIntPoint> Grown;
				for (const FIntPoint& Point : Frontier)
				{
					for (const FIntPoint& Step : PointSteps)
					{
						const FIntPoint Side = Point + Step;
						if (!OnTerrainPoint(Side))
						{
							continue;
						}
						const int32 Key = PointKey(Side);
						if (Banned.Contains(Key) || Beside.Contains(Key))
						{
							continue;
						}
						Beside.Add(Key);
						Grown.Add(Side);
					}
				}
				if (Grown.Num() == 0)
				{
					return false;
				}
				Frontier = Grown;
				Beside.Add(PointKey(Before));
				Beside.Add(PointKey(After));

				TArray<FIntPoint> Queue;
				TMap<int32, int32> CameFrom;
				Queue.Add(Before);
				CameFrom.Add(PointKey(Before), INDEX_NONE);
				bool bFound = false;
				for (int32 Index = 0; Index < Queue.Num(); ++Index)
				{
					const FIntPoint Here = Queue[Index];
					if (Here == After)
					{
						bFound = true;
						break;
					}
					for (const FIntPoint& Step : PointSteps)
					{
						const FIntPoint Next = Here + Step;
						if (!OnTerrainPoint(Next))
						{
							continue;
						}
						const int32 Key = PointKey(Next);
						if (!Beside.Contains(Key) || CameFrom.Contains(Key) || !LineStepAllowed(Map, Type, Here, Next))
						{
							continue;
						}
						if (bClearMountain && Next != Before && Next != After && BesideMountainOrSea(Map, Next))
						{
							continue;
						}
						CameFrom.Add(Key, PointKey(Here));
						Queue.Add(Next);
					}
				}
				if (!bFound)
				{
					continue;
				}

				TArray<FIntPoint> Reversed;
				for (int32 Key = PointKey(After); Key != INDEX_NONE; Key = CameFrom[Key])
				{
					Reversed.Add(FIntPoint(Key % USolidWorldChart::PointAxis, Key / USolidWorldChart::PointAxis));
					if (Reversed.Num() > Beside.Num())
					{
						return false;
					}
				}

				OutPoints.Reset();
				for (int32 Index = 0; Index < First; ++Index)
				{
					OutPoints.Add(Line[Index]);
				}
				for (int32 Index = Reversed.Num() - 2; Index >= 0; --Index)
				{
					OutPoints.Add(Reversed[Index]);
				}
				for (int32 Index = Last + 2; Index < Line.Num(); ++Index)
				{
					OutPoints.Add(Line[Index]);
				}
				return OutPoints.Num() > 1;
			}
			return false;
		}

		bool StraightRunAllowed(
			const USolidWorldChart& Map,
			const EType Type,
			const FIntPoint& From,
			const FIntPoint& To)
		{
			TArray<FIntPoint> Line;
			TerrainLine(From, To, Line);
			for (int32 Index = 1; Index < Line.Num(); ++Index)
			{
				if (!LineStepAllowed(Map, Type, Line[Index - 1], Line[Index]))
				{
					return false;
				}
				if (Index + 1 < Line.Num() && BesideMountainOrSea(Map, Line[Index]))
				{
					return false;
				}
			}
			return Line.Num() > 1;
		}

		void PullStraight(
			const USolidWorldChart& Map,
			const EType Type,
			TArray<FIntPoint>& Points)
		{
			if (Points.Num() < 3)
			{
				return;
			}

			bool bChanged = true;
			while (bChanged)
			{
				bChanged = false;
				for (int32 Index = 0; Index + 2 < Points.Num();)
				{
					if (!StraightRunAllowed(Map, Type, Points[Index], Points[Index + 2]))
					{
						++Index;
						continue;
					}
					Points.RemoveAt(Index + 1);
					bChanged = true;
				}
			}

			TArray<FIntPoint> Expanded;
			Expanded.Add(Points[0]);
			for (int32 Index = 1; Index < Points.Num(); ++Index)
			{
				TArray<FIntPoint> Segment;
				TerrainLine(Points[Index - 1], Points[Index], Segment);
				for (int32 Step = 1; Step < Segment.Num(); ++Step)
				{
					Expanded.Add(Segment[Step]);
				}
			}
			Points = MoveTemp(Expanded);
		}
	}

	FString FSolidRoad::toString() const
	{
		const TCHAR* TypeName = TEXT("trail");
		switch (Type)
		{
		case EType::Track:
			TypeName = TEXT("track");
			break;
		case EType::Road:
			TypeName = TEXT("road");
			break;
		case EType::Highway:
			TypeName = TEXT("highway");
			break;
		case EType::Trail:
		default:
			break;
		}
		return FString::Printf(TEXT("%s -> %s, %s"), *From.Name, *To.Name, TypeName);
	}

	bool FSolidRoad::FindRoute(const USolidWorldChart& Map)
	{
		Points.Reset();
		FVector2D Min = FVector2D::ZeroVector;
		FVector2D Max = FVector2D::ZeroVector;
		USolidWorldChart::DefaultWorldBounds(Min, Max);
		FIntPoint FromPoint = FIntPoint::ZeroValue;
		FIntPoint ToPoint = FIntPoint::ZeroValue;
		if (!Map.TerrainPointAt(From.PositionCm.X, From.PositionCm.Y, Min, Max, FromPoint)
			|| !Map.TerrainPointAt(To.PositionCm.X, To.PositionCm.Y, Min, Max, ToPoint))
		{
			return false;
		}

		TerrainLine(FromPoint, ToPoint, Points);
		TArray<int32> Blocked;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			if (!LineStepAllowed(Map, Type, Points[Index - 1], Points[Index]))
			{
				const FIntPoint& Block = Points[Index];
				UE_LOG(LogSolid, Warning, TEXT("%s: blocked at %d,%d %s."),
					*toString(),
					Block.X,
					Block.Y,
					SolidBiome::Of(Map.GetPointBiome(Block)).Name());
				Blocked.Add(Index);
			}
		}
		if (Blocked.Num() > 0)
		{
			int32 Runs = 1;
			for (int32 Index = 1; Index < Blocked.Num(); ++Index)
			{
				if (Blocked[Index] != Blocked[Index - 1] + 1)
				{
					++Runs;
				}
			}
			const TArray<FIntPoint> Line = Points;
			if (Runs != 1 || !DetourAroundPatch(Map, Type, Line, Blocked[0], Blocked.Last(), Points))
			{
				Points.Reset();
				return false;
			}
		}

		PullStraight(Map, Type, Points);
		return Points.Num() > 1;
	}

	void FSolidRoad::CopyPoints(TArray<FIntPoint>& OutPoints) const
	{
		OutPoints = Points;
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
		FSolidRoad* Road,
		TArray<FIntPoint>& OutPoints)
	{
		OutCells.Reset();
		OutPoints.Reset();
		if (Road != nullptr && Road->FindRoute(Map))
		{
			Road->CopyPoints(OutPoints);
			if (CellsForPoints(Map, OutPoints, OutCells) && OutCells.Num() > 0)
			{
				UE_LOG(LogSolid, Warning, TEXT("%s: FindRoute."), *Road->toString());
				return true;
			}
			OutCells.Reset();
			OutPoints.Reset();
		}
		if (Road != nullptr)
		{
			UE_LOG(LogSolid, Warning, TEXT("%s: cell search."), *Road->toString());
		}
		OutPoints.Reset();

		const EType Type = Road != nullptr ? Road->Type : Definition.Type;
		TArray<FIntPoint> Starts;
		TArray<FIntPoint> Goals;
		RoadCollectEndpoint(Map, Definition.From, Starts);
		RoadCollectEndpoint(Map, Definition.To, Goals);
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

namespace SolidRoad
{
	static FString CompactName(FString Name)
	{
		Name.ReplaceInline(TEXT(" "), TEXT(""));
		return Name;
	}

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

