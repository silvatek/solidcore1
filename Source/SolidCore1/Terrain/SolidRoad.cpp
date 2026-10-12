#include "SolidRoad.h"
#include "SolidWorldMap.h"

namespace SolidRoad
{
	namespace
	{
		bool RoadParseEndpoint(const FString& Text, FEndpoint& Out)
		{
			const FString Token = Text.TrimStartAndEnd();
			if (Token.Len() == 1 && (Token[0] == TEXT('C') || Token[0] == TEXT('c')))
			{
				Out.bCave = true;
				Out.LocationId = INDEX_NONE;
				return true;
			}

			if (Token.IsEmpty())
			{
				return false;
			}
			for (const TCHAR Char : Token)
			{
				if (!FChar::IsDigit(Char))
				{
					return false;
				}
			}

			Out.bCave = false;
			Out.LocationId = FCString::Atoi(*Token);
			return true;
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

		void RoadCollectEndpoint(const USolidWorldMap& Map, const FEndpoint& End, TArray<FIntPoint>& OutCells)
		{
			OutCells.Reset();
			if (End.bCave)
			{
				for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
				{
					for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
					{
						if (Map.GetMarkerAtCell(Col, Row) == TEXT('C'))
						{
							OutCells.Add(FIntPoint(Col, Row));
						}
					}
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
				const int32 NextCost = BestCost + EnterCost(Definition.Type, Map.GetBiomeAtCell(Next.X, Next.Y));
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
