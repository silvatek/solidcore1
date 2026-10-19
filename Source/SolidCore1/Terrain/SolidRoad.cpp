#include "SolidRoad.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWorldMap.h"

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

		bool CellBesideRiver(const USolidWorldMap& Map, const int32 X, const int32 Y)
		{
			for (int32 OffsetY = -RiverBendReach; OffsetY <= RiverBendReach; ++OffsetY)
			{
				for (int32 OffsetX = -RiverBendReach; OffsetX <= RiverBendReach; ++OffsetX)
				{
					const int32 NextX = X + OffsetX;
					const int32 NextY = Y + OffsetY;
					if (NextX < 0 || NextY < 0 || NextX >= USolidWorldMap::MapWidth || NextY >= USolidWorldMap::MapHeight)
					{
						continue;
					}
					if (Map.GetMarkerAtCell(NextX, NextY) == USolidWorldMap::RiverMarker)
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
		void SlideSpursAfterRiver(const USolidWorldMap& Map, const EType Type, TArray<FIntPoint>& Cells)
		{
			TArray<int32> Rivers;
			for (int32 Index = 0; Index < Cells.Num(); ++Index)
			{
				if (Map.GetMarkerAtCell(Cells[Index].X, Cells[Index].Y) == USolidWorldMap::RiverMarker)
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
					if (Side.X < 0 || Side.Y < 0 || Side.X >= USolidWorldMap::MapWidth || Side.Y >= USolidWorldMap::MapHeight)
					{
						continue;
					}
					if (Map.GetMarkerAtCell(Side.X, Side.Y) == USolidWorldMap::RiverMarker)
					{
						continue;
					}
					if (EnterCost(Type, Map.GetBiomeAtCell(Side.X, Side.Y)) >= AvoidCost)
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

		void RoadCollectEndpoint(const USolidWorldMap& Map, const FEndpoint& End, TArray<FIntPoint>& OutCells)
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
				OutCells = Location.Cells;
			}
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

	bool Solve(const USolidWorldMap& Map, const FDefinition& Definition, TArray<FIntPoint>& OutCells)
	{
		OutCells.Reset();

		TArray<FIntPoint> Starts;
		TArray<FIntPoint> Goals;
		RoadCollectEndpoint(Map, Definition.From, Starts);
		RoadCollectEndpoint(Map, Definition.To, Goals);
		if (Starts.Num() == 0 || Goals.Num() == 0)
		{
			return false;
		}

		const int32 Width = USolidWorldMap::MapWidth;
		const int32 Height = USolidWorldMap::MapHeight;
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
				const int32 NextCost = BestCost + EnterCost(Definition.Type, Map.GetBiomeAtCell(Next.X, Next.Y)) + Bend;
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
		SlideSpursAfterRiver(Map, Definition.Type, OutCells);
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
}
