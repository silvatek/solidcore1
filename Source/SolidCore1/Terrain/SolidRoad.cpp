#include "SolidRoad.h"
#include "SolidBridges.h"
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

		bool CellIsRiver(const USolidWorldChart& Map, const int32 CellX, const int32 CellY)
		{
			if (CellX < 0 || CellY < 0 || CellX >= USolidWorldChart::MapWidth || CellY >= USolidWorldChart::MapHeight)
			{
				return false;
			}
			return Map.GetPointBiomeAtCell(CellX, CellY) == ESolidBiome::River;
		}

		/**
		 * A river step is allowed when it runs across the channel, not along it.
		 * A cell with no river beside it is a pond, so the step crosses it.
		 */
		bool StepIsSquareToRiver(const USolidWorldChart& Map, const FIntPoint& FromCell, const FIntPoint& ToCell)
		{
			const bool bAlongY = CellIsRiver(Map, ToCell.X, ToCell.Y - 1) || CellIsRiver(Map, ToCell.X, ToCell.Y + 1);
			const bool bAlongX = CellIsRiver(Map, ToCell.X + 1, ToCell.Y) || CellIsRiver(Map, ToCell.X - 1, ToCell.Y);
			if (!bAlongY && !bAlongX)
			{
				return true;
			}
			if (bAlongY && bAlongX)
			{
				return false;
			}
			const FIntPoint Step(ToCell.X - FromCell.X, ToCell.Y - FromCell.Y);
			if (bAlongY)
			{
				return Step.X != 0 && Step.Y == 0;
			}
			return Step.Y != 0 && Step.X == 0;
		}

		bool CellBesideRiver(const USolidWorldChart& Map, const int32 CellX, const int32 CellY)
		{
			for (int32 OffsetY = -RiverBendReach; OffsetY <= RiverBendReach; ++OffsetY)
			{
				for (int32 OffsetX = -RiverBendReach; OffsetX <= RiverBendReach; ++OffsetX)
				{
					const int32 NextCellX = CellX + OffsetX;
					const int32 NextCellY = CellY + OffsetY;
					if (NextCellX < 0 || NextCellY < 0 || NextCellX >= USolidWorldChart::MapWidth || NextCellY >= USolidWorldChart::MapHeight)
					{
						continue;
					}
					if (Map.GetMarkerAtCell(NextCellX, NextCellY) == USolidWorldChart::RiverMarker)
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
				const FIntPoint& RiverCell = Cells[RiverIndex];
				Best = FMath::Min(Best, FMath::Max(FMath::Abs(Cell.X - RiverCell.X), FMath::Abs(Cell.Y - RiverCell.Y)));
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

					const FIntPoint SideCell = Cells[Index - 1] + Out;
					if (SideCell + In != Cells[Index + 1])
					{
						continue;
					}
					if (ChebyshevToRiver(Cells, Rivers, SideCell) >= ChebyshevToRiver(Cells, Rivers, Cells[Index]))
					{
						continue;
					}
					if (SideCell.X < 0 || SideCell.Y < 0 || SideCell.X >= USolidWorldChart::MapWidth || SideCell.Y >= USolidWorldChart::MapHeight)
					{
						continue;
					}
					if (Map.GetMarkerAtCell(SideCell.X, SideCell.Y) == USolidWorldChart::RiverMarker)
					{
						continue;
					}
					if (EnterCost(Type, Map.GetPointBiomeAtCell(SideCell.X, SideCell.Y)) < 0)
					{
						continue;
					}
					if (Cells.Contains(SideCell))
					{
						continue;
					}

					Cells[Index] = SideCell;
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

		void TerrainLine(const FIntPoint& FromTerrainPoint, const FIntPoint& ToTerrainPoint, TArray<FIntPoint>& OutPoints)
		{
			OutPoints.Reset();
			int32 TerrainX = FromTerrainPoint.X;
			int32 TerrainY = FromTerrainPoint.Y;
			const int32 TerrainX1 = ToTerrainPoint.X;
			const int32 TerrainY1 = ToTerrainPoint.Y;
			const int32 Dx = FMath::Abs(TerrainX1 - TerrainX);
			const int32 Dy = FMath::Abs(TerrainY1 - TerrainY);
			const int32 Sx = TerrainX < TerrainX1 ? 1 : -1;
			const int32 Sy = TerrainY < TerrainY1 ? 1 : -1;
			int32 Err = Dx - Dy;
			while (true)
			{
				OutPoints.Add(FIntPoint(TerrainX, TerrainY));
				if (TerrainX == TerrainX1 && TerrainY == TerrainY1)
				{
					break;
				}
				const int32 Twice = Err * 2;
				if (Twice > -Dy)
				{
					Err -= Dy;
					TerrainX += Sx;
				}
				if (Twice < Dx)
				{
					Err += Dx;
					TerrainY += Sy;
				}
			}
		}

		bool RiverPoint(const USolidWorldChart& Map, const int32 TerrainX, const int32 TerrainY)
		{
			if (TerrainX < 0 || TerrainY < 0 || TerrainX >= USolidWorldChart::PointAxis || TerrainY >= USolidWorldChart::PointAxis)
			{
				return false;
			}
			return Map.GetPointBiome(FIntPoint(TerrainX, TerrainY)) == ESolidBiome::River;
		}

		bool LineStepAllowed(const USolidWorldChart& Map, const EType Type, const FIntPoint& FromTerrainPoint, const FIntPoint& ToTerrainPoint)
		{
			const ESolidBiome Biome = Map.GetPointBiome(ToTerrainPoint);
			if (Biome != ESolidBiome::River)
			{
				return EnterCost(Type, Biome, false) >= 0;
			}
			const int32 AlongY = (RiverPoint(Map, ToTerrainPoint.X, ToTerrainPoint.Y + 1) ? 1 : 0) + (RiverPoint(Map, ToTerrainPoint.X, ToTerrainPoint.Y - 1) ? 1 : 0);
			const int32 AlongX = (RiverPoint(Map, ToTerrainPoint.X + 1, ToTerrainPoint.Y) ? 1 : 0) + (RiverPoint(Map, ToTerrainPoint.X - 1, ToTerrainPoint.Y) ? 1 : 0);
			const FIntPoint Step(ToTerrainPoint.X - FromTerrainPoint.X, ToTerrainPoint.Y - FromTerrainPoint.Y);
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

		bool OnTerrainPoint(const FIntPoint& TerrainPoint)
		{
			return TerrainPoint.X >= 0 && TerrainPoint.Y >= 0
				&& TerrainPoint.X < USolidWorldChart::PointAxis
				&& TerrainPoint.Y < USolidWorldChart::PointAxis;
		}

		int32 PointKey(const FIntPoint& TerrainPoint)
		{
			return TerrainPoint.Y * USolidWorldChart::PointAxis + TerrainPoint.X;
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

		bool BesideMountainOrSea(const USolidWorldChart& Map, const FIntPoint& TerrainPoint)
		{
			for (const FIntPoint& Step : PointSteps)
			{
				const FIntPoint SideTerrainPoint = TerrainPoint + Step;
				if (!OnTerrainPoint(SideTerrainPoint))
				{
					continue;
				}
				const ESolidBiome Biome = Map.GetPointBiome(SideTerrainPoint);
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

			const FIntPoint BeforeTerrainPoint = Line[First - 1];
			const FIntPoint AfterTerrainPoint = Line[Last + 1];
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
				for (const FIntPoint& TerrainPoint : Frontier)
				{
					for (const FIntPoint& Step : PointSteps)
					{
						const FIntPoint SideTerrainPoint = TerrainPoint + Step;
						if (!OnTerrainPoint(SideTerrainPoint))
						{
							continue;
						}
						const int32 Key = PointKey(SideTerrainPoint);
						if (Banned.Contains(Key) || Beside.Contains(Key))
						{
							continue;
						}
						Beside.Add(Key);
						Grown.Add(SideTerrainPoint);
					}
				}
				if (Grown.Num() == 0)
				{
					return false;
				}
				Frontier = Grown;
				Beside.Add(PointKey(BeforeTerrainPoint));
				Beside.Add(PointKey(AfterTerrainPoint));

				TArray<FIntPoint> Queue;
				TMap<int32, int32> CameFrom;
				Queue.Add(BeforeTerrainPoint);
				CameFrom.Add(PointKey(BeforeTerrainPoint), INDEX_NONE);
				bool bFound = false;
				for (int32 Index = 0; Index < Queue.Num(); ++Index)
				{
					const FIntPoint HereTerrainPoint = Queue[Index];
					if (HereTerrainPoint == AfterTerrainPoint)
					{
						bFound = true;
						break;
					}
					for (const FIntPoint& Step : PointSteps)
					{
						const FIntPoint NextTerrainPoint = HereTerrainPoint + Step;
						if (!OnTerrainPoint(NextTerrainPoint))
						{
							continue;
						}
						const int32 Key = PointKey(NextTerrainPoint);
						if (!Beside.Contains(Key) || CameFrom.Contains(Key) || !LineStepAllowed(Map, Type, HereTerrainPoint, NextTerrainPoint))
						{
							continue;
						}
						if (bClearMountain && NextTerrainPoint != BeforeTerrainPoint && NextTerrainPoint != AfterTerrainPoint && BesideMountainOrSea(Map, NextTerrainPoint))
						{
							continue;
						}
						CameFrom.Add(Key, PointKey(HereTerrainPoint));
						Queue.Add(NextTerrainPoint);
					}
				}
				if (!bFound)
				{
					continue;
				}

				TArray<FIntPoint> Reversed;
				for (int32 Key = PointKey(AfterTerrainPoint); Key != INDEX_NONE; Key = CameFrom[Key])
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

		bool RiverAxis(const USolidWorldChart& Map, const FIntPoint& TerrainPoint, FIntPoint& OutAlong, FIntPoint& OutAcross)
		{
			const int32 AlongY = (RiverPoint(Map, TerrainPoint.X, TerrainPoint.Y + 1) ? 1 : 0) + (RiverPoint(Map, TerrainPoint.X, TerrainPoint.Y - 1) ? 1 : 0);
			const int32 AlongX = (RiverPoint(Map, TerrainPoint.X + 1, TerrainPoint.Y) ? 1 : 0) + (RiverPoint(Map, TerrainPoint.X - 1, TerrainPoint.Y) ? 1 : 0);
			if (AlongX > AlongY)
			{
				OutAlong = FIntPoint(1, 0);
				OutAcross = FIntPoint(0, 1);
				return true;
			}
			if (AlongY > AlongX)
			{
				OutAlong = FIntPoint(0, 1);
				OutAcross = FIntPoint(1, 0);
				return true;
			}
			return false;
		}

		bool FindRiverBank(
			const USolidWorldChart& Map,
			const FIntPoint& AnchorTerrainPoint,
			const FIntPoint& Dir,
			FIntPoint& OutBankTerrainPoint)
		{
			FIntPoint HereTerrainPoint = AnchorTerrainPoint;
			for (int32 Step = 0; Step < 6; ++Step)
			{
				const FIntPoint NextTerrainPoint = HereTerrainPoint + Dir;
				if (!OnTerrainPoint(NextTerrainPoint))
				{
					return false;
				}
				if (!RiverPoint(Map, NextTerrainPoint.X, NextTerrainPoint.Y))
				{
					OutBankTerrainPoint = NextTerrainPoint;
					return true;
				}
				HereTerrainPoint = NextTerrainPoint;
			}
			return false;
		}

		/** A river run is a crossing at 90 degrees. Slide along the channel when the hit is beside the sea. */
		bool CrossRiver(
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

			FIntPoint Along = FIntPoint::ZeroValue;
			FIntPoint Across = FIntPoint::ZeroValue;
			FIntPoint HitTerrainPoint = FIntPoint::ZeroValue;
			bool bAxis = false;
			for (int32 Index = First; Index <= Last; ++Index)
			{
				if (Map.GetPointBiome(Line[Index]) != ESolidBiome::River)
				{
					return false;
				}
				if (!bAxis && RiverAxis(Map, Line[Index], Along, Across))
				{
					HitTerrainPoint = Line[Index];
					bAxis = true;
				}
			}
			if (!bAxis)
			{
				return false;
			}

			const FIntPoint BeforeTerrainPoint = Line[First - 1];
			const FIntPoint AfterTerrainPoint = Line[Last + 1];
			const auto StepsBetween = [](const FIntPoint& FromTerrainPoint, const FIntPoint& ToTerrainPoint)
			{
				return FMath::Abs(FromTerrainPoint.X - ToTerrainPoint.X) + FMath::Abs(FromTerrainPoint.Y - ToTerrainPoint.Y);
			};

			for (int32 Slide = 0; Slide <= DetourReach; ++Slide)
			{
				for (int32 Sign = 1; Sign >= -1; Sign -= 2)
				{
					if (Slide == 0 && Sign < 0)
					{
						continue;
					}
					const FIntPoint AnchorTerrainPoint = HitTerrainPoint + Along * (Sign * Slide);
					if (!OnTerrainPoint(AnchorTerrainPoint) || !RiverPoint(Map, AnchorTerrainPoint.X, AnchorTerrainPoint.Y))
					{
						continue;
					}
					FIntPoint BankATerrainPoint = FIntPoint::ZeroValue;
					FIntPoint BankBTerrainPoint = FIntPoint::ZeroValue;
					if (!FindRiverBank(Map, AnchorTerrainPoint, FIntPoint(-Across.X, -Across.Y), BankATerrainPoint)
						|| !FindRiverBank(Map, AnchorTerrainPoint, Across, BankBTerrainPoint))
					{
						continue;
					}

					TArray<FIntPoint> Crossing;
					Crossing.Add(BankATerrainPoint);
					FIntPoint HereTerrainPoint = BankATerrainPoint;
					bool bReached = false;
					for (int32 Step = 0; Step < 8; ++Step)
					{
						const FIntPoint NextTerrainPoint = HereTerrainPoint + Across;
						if (!OnTerrainPoint(NextTerrainPoint) || !LineStepAllowed(Map, Type, HereTerrainPoint, NextTerrainPoint))
						{
							break;
						}
						Crossing.Add(NextTerrainPoint);
						if (NextTerrainPoint == BankBTerrainPoint)
						{
							bReached = true;
							break;
						}
						HereTerrainPoint = NextTerrainPoint;
					}
					if (!bReached)
					{
						continue;
					}

					const FIntPoint NearTerrainPoint = StepsBetween(BeforeTerrainPoint, BankATerrainPoint) <= StepsBetween(BeforeTerrainPoint, BankBTerrainPoint) ? BankATerrainPoint : BankBTerrainPoint;
					const FIntPoint FarTerrainPoint = NearTerrainPoint == BankATerrainPoint ? BankBTerrainPoint : BankATerrainPoint;
					if (StepsBetween(AfterTerrainPoint, FarTerrainPoint) >= StepsBetween(AfterTerrainPoint, NearTerrainPoint))
					{
						continue;
					}

					auto OpenTo = [&Map, Type](const FIntPoint& FromTerrainPoint, const FIntPoint& ToTerrainPoint) -> bool
					{
						if (FromTerrainPoint == ToTerrainPoint)
						{
							return true;
						}
						TArray<FIntPoint> Segment;
						TerrainLine(FromTerrainPoint, ToTerrainPoint, Segment);
						for (int32 Index = 1; Index < Segment.Num(); ++Index)
						{
							if (!LineStepAllowed(Map, Type, Segment[Index - 1], Segment[Index]))
							{
								return false;
							}
						}
						return true;
					};
					if (!OpenTo(BeforeTerrainPoint, NearTerrainPoint) || !OpenTo(FarTerrainPoint, AfterTerrainPoint))
					{
						continue;
					}

					OutPoints.Reset();
					for (int32 Index = 0; Index < First; ++Index)
					{
						OutPoints.Add(Line[Index]);
					}
					auto AppendLine = [&OutPoints](const FIntPoint& FromTerrainPoint, const FIntPoint& ToTerrainPoint)
					{
						if (FromTerrainPoint == ToTerrainPoint)
						{
							return;
						}
						TArray<FIntPoint> Segment;
						TerrainLine(FromTerrainPoint, ToTerrainPoint, Segment);
						for (int32 Index = 1; Index < Segment.Num(); ++Index)
						{
							OutPoints.Add(Segment[Index]);
						}
					};
					AppendLine(BeforeTerrainPoint, NearTerrainPoint);
					const int32 Start = Crossing[0] == NearTerrainPoint ? 1 : Crossing.Num() - 2;
					const int32 Step = Crossing[0] == NearTerrainPoint ? 1 : -1;
					const int32 Stop = Crossing[0] == NearTerrainPoint ? Crossing.Num() : -1;
					for (int32 Index = Start; Index != Stop; Index += Step)
					{
						if (OutPoints.Num() == 0 || OutPoints.Last() != Crossing[Index])
						{
							OutPoints.Add(Crossing[Index]);
						}
					}
					AppendLine(FarTerrainPoint, AfterTerrainPoint);
					for (int32 Index = Last + 2; Index < Line.Num(); ++Index)
					{
						OutPoints.Add(Line[Index]);
					}
					return OutPoints.Num() > 1;
				}
			}
			return false;
		}

		bool StraightRunAllowed(
			const USolidWorldChart& Map,
			const EType Type,
			const FIntPoint& FromTerrainPoint,
			const FIntPoint& ToTerrainPoint)
		{
			TArray<FIntPoint> Line;
			TerrainLine(FromTerrainPoint, ToTerrainPoint, Line);
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

		/**
		 * The span end closer to the other end of the route.
		 * A tie, or a bridge with no definition, has no such end.
		 */
		bool NearerSpanEnd(const FSolidWorldChartMark& Bridge, const FVector2D& OtherChartCm, FVector2D& OutChartCm)
		{
			const SolidBridge::FBridgeMarker* Marker = nullptr;
			for (const SolidBridge::FBridgeMarker& Defined : SolidBridge::Bridges)
			{
				if (Defined.Name && Bridge.Name.Equals(Defined.Name, ESearchCase::IgnoreCase))
				{
					Marker = &Defined;
					break;
				}
			}
			if (!Marker || Marker->LengthCm <= 0.f)
			{
				return false;
			}
			const float Half = Marker->LengthCm * 0.5f;
			const FVector2D Along = Marker->Orientation == SolidBridge::EOrientation::NorthSouth
				? FVector2D(0.f, Half)
				: FVector2D(Half, 0.f);
			const FVector2D OneEnd = Bridge.ChartCm + Along;
			const FVector2D OtherEnd = Bridge.ChartCm - Along;
			const float OneDist = FVector2D::DistSquared(OneEnd, OtherChartCm);
			const float OtherDist = FVector2D::DistSquared(OtherEnd, OtherChartCm);
			if (FMath::IsNearlyEqual(OneDist, OtherDist))
			{
				return false;
			}
			OutChartCm = OneDist < OtherDist ? OneEnd : OtherEnd;
			return true;
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
		FVector2D ChartMin = FVector2D::ZeroVector;
		FVector2D ChartMax = FVector2D::ZeroVector;
		USolidWorldChart::ChartBounds(ChartMin, ChartMax);
		FVector2D FromChartCm = From.ChartCm;
		FVector2D ToChartCm = To.ChartCm;
		if (From.Kind == ESolidWorldChartMarkKind::Bridge && !NearerSpanEnd(From, To.ChartCm, FromChartCm))
		{
			return false;
		}
		if (To.Kind == ESolidWorldChartMarkKind::Bridge && !NearerSpanEnd(To, From.ChartCm, ToChartCm))
		{
			return false;
		}
		FIntPoint FromTerrainPoint = FIntPoint::ZeroValue;
		FIntPoint ToTerrainPoint = FIntPoint::ZeroValue;
		if (!Map.ChartTerrainPointAt(FromChartCm.X, FromChartCm.Y, ChartMin, ChartMax, FromTerrainPoint)
			|| !Map.ChartTerrainPointAt(ToChartCm.X, ToChartCm.Y, ChartMin, ChartMax, ToTerrainPoint))
		{
			return false;
		}

		TerrainLine(FromTerrainPoint, ToTerrainPoint, Points);
		bool bOpen = false;
		for (int32 Attempt = 0; Attempt < 4; ++Attempt)
		{
			TArray<int32> Blocked;
			for (int32 Index = 1; Index < Points.Num(); ++Index)
			{
				if (!LineStepAllowed(Map, Type, Points[Index - 1], Points[Index]))
				{
					const FIntPoint& BlockTerrainPoint = Points[Index];
					UE_LOG(LogSolid, Warning, TEXT("%s: blocked at %d,%d %s."),
						*toString(),
						BlockTerrainPoint.X,
						BlockTerrainPoint.Y,
						SolidBiome::Of(Map.GetPointBiome(BlockTerrainPoint)).Name());
					Blocked.Add(Index);
				}
			}
			if (Blocked.Num() == 0)
			{
				bOpen = true;
				break;
			}

			int32 Last = Blocked[0];
			for (int32 Index = 1; Index < Blocked.Num(); ++Index)
			{
				if (Blocked[Index] != Last + 1)
				{
					break;
				}
				Last = Blocked[Index];
			}

			const TArray<FIntPoint> Line = Points;
			if (!CrossRiver(Map, Type, Line, Blocked[0], Last, Points)
				&& !DetourAroundPatch(Map, Type, Line, Blocked[0], Last, Points))
			{
				Points.Reset();
				return false;
			}
		}
		if (!bOpen)
		{
			Points.Reset();
			return false;
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

