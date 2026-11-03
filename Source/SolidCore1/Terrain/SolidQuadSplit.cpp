#include "SolidQuadSplit.h"
#include "SolidBiome.h"

FSolidQuadSplit::~FSolidQuadSplit() = default;

bool FSolidQuadTriangle::MidpointEnds(const int32 Corner, int32& OutEnd0, int32& OutEnd1)
{
	switch (Corner)
	{
	case 4:
		OutEnd0 = 0;
		OutEnd1 = 1;
		return true;
	case 5:
		OutEnd0 = 2;
		OutEnd1 = 3;
		return true;
	case 6:
		OutEnd0 = 0;
		OutEnd1 = 2;
		return true;
	case 7:
		OutEnd0 = 1;
		OutEnd1 = 3;
		return true;
	default:
		return false;
	}
}

ESolidQuadSplit FSolidQuadSplit::QuadSplit = ESolidQuadSplit::NorthWest;

void FSolidQuadSplit::Cycle()
{
	switch (QuadSplit)
	{
	case ESolidQuadSplit::NorthWest:
		QuadSplit = ESolidQuadSplit::NorthWestCut;
		break;
	case ESolidQuadSplit::NorthWestCut:
		QuadSplit = ESolidQuadSplit::PriorityMidpoint;
		break;
	case ESolidQuadSplit::PriorityMidpoint:
	default:
		QuadSplit = ESolidQuadSplit::NorthWest;
		break;
	}
}

const TCHAR* FSolidNorthWestSplit::Name() const
{
	return TEXT("NorthWest");
}

const TCHAR* FSolidNorthWestCutSplit::Name() const
{
	return TEXT("NorthWestCut");
}

const TCHAR* FSolidPriorityMidpointSplit::Name() const
{
	return TEXT("PriorityMidpoint");
}

FString FSolidQuadSplit::Description(
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome WestOfNorthWest,
	const ESolidBiome SouthWestOfNorthWest) const
{
	const FSolidQuadTriangles Picked = Triangles(
		Preferred, B00, B10, B01, B11, WestOfNorthWest, SouthWestOfNorthWest);
	FString Text;
	for (int32 Index = 0; Index < Picked.Count; ++Index)
	{
		if (Index > 0)
		{
			Text += TEXT(" / ");
		}
		Text += SolidBiome::Of(Picked.Triangles[Index].Material).Name();
	}
	return Text;
}

const FSolidQuadSplit& FSolidQuadSplit::Of(const ESolidQuadSplit Method)
{
	static const FSolidNorthWestSplit NorthWest;
	static const FSolidNorthWestCutSplit NorthWestCut;
	static const FSolidPriorityMidpointSplit PriorityMidpoint;
	switch (Method)
	{
	case ESolidQuadSplit::NorthWestCut:
		return NorthWestCut;
	case ESolidQuadSplit::PriorityMidpoint:
		return PriorityMidpoint;
	case ESolidQuadSplit::NorthWest:
	default:
		return NorthWest;
	}
}

ESolidBiome FSolidQuadSplit::BorderBiome(
	const ESolidBiome Preferred,
	const ESolidBiome A,
	const ESolidBiome B,
	const ESolidBiome C) const
{
	const ESolidBiome Corners[3] = { A, B, C };
	float BestOffset = SolidBiome::Of(A).HeightOffsetCm();
	for (int32 Index = 1; Index < 3; ++Index)
	{
		BestOffset = FMath::Max(BestOffset, SolidBiome::Of(Corners[Index]).HeightOffsetCm());
	}

	if (SolidBiome::Of(Preferred).HeightOffsetCm() == BestOffset)
	{
		return Preferred;
	}

	for (const ESolidBiome Corner : Corners)
	{
		if (SolidBiome::Of(Corner).HeightOffsetCm() == BestOffset)
		{
			return Corner;
		}
	}
	return A;
}

FSolidQuadTriangles FSolidQuadSplit::TwoTriangles(
	const ESolidBiome First,
	const ESolidBiome Second,
	const bool bFlip) const
{
	FSolidQuadTriangles Out;
	auto Append = [&](const ESolidBiome Material, const int32 A, const int32 B, const int32 C)
	{
		if (Out.Count >= FSolidQuadTriangles::Capacity)
		{
			return;
		}
		FSolidQuadTriangle& Triangle = Out.Triangles[Out.Count];
		Triangle.Material = Material;
		Triangle.Corner0 = A;
		Triangle.Corner1 = B;
		Triangle.Corner2 = C;
		++Out.Count;
	};
	if (bFlip)
	{
		Append(First, 1, 0, 2);
		Append(Second, 1, 2, 3);
	}
	else
	{
		Append(First, 0, 3, 1);
		Append(Second, 0, 2, 3);
	}
	return Out;
}

FSolidQuadTriangles FSolidQuadSplit::ApplyShelf(
	FSolidQuadTriangles Triangles,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	const ESolidBiome Corner[4] = { B00, B10, B01, B11 };
	for (int32 Index = 0; Index < Triangles.Count; ++Index)
	{
		FSolidQuadTriangle& Triangle = Triangles.Triangles[Index];
		const int32 Vertices[3] = { Triangle.Corner0, Triangle.Corner1, Triangle.Corner2 };
		float Low = 0.f;
		float High = 0.f;
		bool bTerrain = false;
		for (const int32 Vertex : Vertices)
		{
			if (Vertex < 0 || Vertex > 3)
			{
				continue;
			}
			const float Shelf = SolidBiome::Of(Corner[Vertex]).HeightOffsetCm();
			if (!bTerrain)
			{
				Low = Shelf;
				High = Shelf;
				bTerrain = true;
			}
			else
			{
				Low = FMath::Min(Low, Shelf);
				High = FMath::Max(High, Shelf);
			}
		}
		if (!bTerrain || Low == High)
		{
			continue;
		}
		if (SolidBiome::Of(Triangle.Material).HeightOffsetCm() == High)
		{
			continue;
		}
		for (const int32 Vertex : Vertices)
		{
			if (Vertex < 0 || Vertex > 3)
			{
				continue;
			}
			if (SolidBiome::Of(Corner[Vertex]).HeightOffsetCm() == High)
			{
				Triangle.Material = Corner[Vertex];
				break;
			}
		}
	}
	return Triangles;
}

FSolidQuadTriangles FSolidNorthWestSplit::Triangles(
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome /*WestOfNorthWest*/,
	const ESolidBiome /*SouthWestOfNorthWest*/) const
{
	return ApplyShelf(
		TwoTriangles(
			BorderBiome(Preferred, B00, B11, B10),
			BorderBiome(Preferred, B00, B01, B11),
			false),
		B00, B10, B01, B11);
}

FSolidQuadTriangles FSolidNorthWestCutSplit::Triangles(
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome WestOfNorthWest,
	const ESolidBiome SouthWestOfNorthWest) const
{
	ESolidBiome First = BorderBiome(Preferred, B00, B11, B10);
	ESolidBiome Second = BorderBiome(Preferred, B00, B01, B11);
	bool bFlip = false;

	// Mesh order is southeast, southwest, northeast, northwest.
	const bool bNorthWestAlone = B01 == B10 && B10 == B00 && B00 != B11;
	if (bNorthWestAlone)
	{
		bFlip = true;
		First = B00;
		Second = B11;
	}
	else
	{
		const bool bSouthContinuesWest = B10 != B11 && B01 == B11 && B00 == B11 && WestOfNorthWest == B10;
		const bool bWestEnd = WestOfNorthWest != B11
			&& B01 == B11
			&& B10 == WestOfNorthWest
			&& B00 == WestOfNorthWest
			&& SouthWestOfNorthWest == WestOfNorthWest;
		if (bSouthContinuesWest || bWestEnd)
		{
			First = B10;
			Second = B11;
		}
	}
	return ApplyShelf(TwoTriangles(First, Second, bFlip), B00, B10, B01, B11);
}

namespace
{
void AppendTriangle(
	FSolidQuadTriangles& Out,
	const ESolidBiome Material,
	const int32 A,
	const int32 B,
	const int32 C)
{
	if (Out.Count >= FSolidQuadTriangles::Capacity)
	{
		return;
	}
	FSolidQuadTriangle& Triangle = Out.Triangles[Out.Count];
	Triangle.Material = Material;
	Triangle.Corner0 = A;
	Triangle.Corner1 = B;
	Triangle.Corner2 = C;
	++Out.Count;
}

int32 BiomeRank(const ESolidBiome Biome)
{
	switch (Biome)
	{
	case ESolidBiome::Sea: return 0;
	case ESolidBiome::Grassland: return 1;
	case ESolidBiome::Forest: return 2;
	case ESolidBiome::Desert: return 3;
	case ESolidBiome::Mountain: return 4;
	case ESolidBiome::Town: return 5;
	case ESolidBiome::River: return 6;
	case ESolidBiome::Road: return 7;
	default: return -1;
	}
}
}

// 1. Four corners match.
FSolidQuadTriangles FSolidPriorityMidpointSplit::FourMatchingCorners(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	if (!(B00 == B10 && B10 == B01 && B01 == B11))
	{
		return FSolidQuadTriangles();
	}
	return TwoTriangles(B00, B00, false);
}

// 2. Opposite sides are pairs.
FSolidQuadTriangles FSolidPriorityMidpointSplit::OppositeSides(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	FSolidQuadTriangles Out;
	const bool bNorthAndSouth = B00 == B10 && B01 == B11 && B00 != B01;
	if (bNorthAndSouth)
	{
		AppendTriangle(Out, B00, 0, 6, 7);
		AppendTriangle(Out, B00, 0, 7, 1);
		AppendTriangle(Out, B01, 2, 3, 7);
		AppendTriangle(Out, B01, 2, 7, 6);
		return Out;
	}
	const bool bWestAndEast = B10 == B11 && B00 == B01 && B10 != B00;
	if (bWestAndEast)
	{
		AppendTriangle(Out, B10, 1, 5, 3);
		AppendTriangle(Out, B10, 1, 4, 5);
		AppendTriangle(Out, B00, 0, 2, 5);
		AppendTriangle(Out, B00, 0, 5, 4);
	}
	return Out;
}

// 3. Both diagonals are pairs.
FSolidQuadTriangles FSolidPriorityMidpointSplit::Checker(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	FSolidQuadTriangles Out;
	const bool bChecker = B00 == B11 && B10 == B01 && B00 != B10;
	const int32 AcrossRank = BiomeRank(B00);
	const int32 OtherRank = BiomeRank(B10);
	if (!(bChecker && AcrossRank >= 0 && OtherRank >= 0))
	{
		return Out;
	}
	if (AcrossRank > OtherRank)
	{
		AppendTriangle(Out, B00, 0, 7, 4);
		AppendTriangle(Out, B00, 0, 3, 7);
		AppendTriangle(Out, B00, 0, 6, 3);
		AppendTriangle(Out, B00, 6, 5, 3);
		AppendTriangle(Out, B10, 1, 4, 7);
		AppendTriangle(Out, B10, 2, 5, 6);
	}
	else
	{
		AppendTriangle(Out, B10, 1, 4, 6);
		AppendTriangle(Out, B10, 1, 6, 2);
		AppendTriangle(Out, B10, 1, 5, 7);
		AppendTriangle(Out, B10, 1, 2, 5);
		AppendTriangle(Out, B00, 0, 6, 4);
		AppendTriangle(Out, B00, 3, 7, 5);
	}
	return Out;
}

// 4. One diagonal is a pair.
FSolidQuadTriangles FSolidPriorityMidpointSplit::OneDiagonal(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	FSolidQuadTriangles Out;
	const bool bAcross = B00 == B11 && B10 != B01 && B10 != B00 && B01 != B00;
	if (bAcross)
	{
		AppendTriangle(Out, B00, 0, 7, 4);
		AppendTriangle(Out, B00, 0, 3, 7);
		AppendTriangle(Out, B00, 0, 6, 3);
		AppendTriangle(Out, B00, 6, 5, 3);
		AppendTriangle(Out, B10, 1, 4, 7);
		AppendTriangle(Out, B01, 2, 5, 6);
		return Out;
	}
	const bool bOther = B10 == B01 && B00 != B11 && B00 != B10 && B11 != B10;
	if (bOther)
	{
		AppendTriangle(Out, B10, 1, 4, 6);
		AppendTriangle(Out, B10, 1, 6, 2);
		AppendTriangle(Out, B10, 1, 5, 7);
		AppendTriangle(Out, B10, 1, 2, 5);
		AppendTriangle(Out, B00, 0, 6, 4);
		AppendTriangle(Out, B11, 3, 7, 5);
	}
	return Out;
}

// 5. Three corners match.
FSolidQuadTriangles FSolidPriorityMidpointSplit::ThreeMatchingCorners(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	const ESolidBiome CornerBiome[4] = { B00, B10, B01, B11 };
	int32 Odd = -1;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		int32 Same = 0;
		for (int32 Other = 0; Other < 4; ++Other)
		{
			if (CornerBiome[Other] == CornerBiome[Index])
			{
				++Same;
			}
		}
		if (Same == 1)
		{
			if (Odd >= 0)
			{
				Odd = -1;
				break;
			}
			Odd = Index;
		}
	}

	FSolidQuadTriangles Out;
	if (Odd < 0)
	{
		return Out;
	}

	struct FCornerCut
	{
		int32 FirstMid;
		int32 SecondMid;
		int32 Opposite;
		int32 FarEndOfFirst;
		int32 FarEndOfSecond;
	};
	const FCornerCut Cuts[4] = {
		{ 6, 4, 3, 2, 1 },
		{ 4, 7, 2, 0, 3 },
		{ 5, 6, 1, 3, 0 },
		{ 7, 5, 0, 1, 2 },
	};
	const FCornerCut& Cut = Cuts[Odd];
	const ESolidBiome Majority = CornerBiome[Cut.Opposite];
	AppendTriangle(Out, CornerBiome[Odd], Odd, Cut.FirstMid, Cut.SecondMid);
	AppendTriangle(Out, Majority, Cut.Opposite, Cut.FirstMid, Cut.FarEndOfFirst);
	AppendTriangle(Out, Majority, Cut.Opposite, Cut.SecondMid, Cut.FirstMid);
	AppendTriangle(Out, Majority, Cut.Opposite, Cut.FarEndOfSecond, Cut.SecondMid);
	return Out;
}

// 6. Two adjacent corners are the highest material.
FSolidQuadTriangles FSolidPriorityMidpointSplit::HighestAdjacent(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	FSolidQuadTriangles Out;
	const int32 Rank00 = BiomeRank(B00);
	const int32 Rank10 = BiomeRank(B10);
	const int32 Rank01 = BiomeRank(B01);
	const int32 Rank11 = BiomeRank(B11);
	const int32 Centre = FSolidQuadTriangle::Centre;
	const bool bSouth = B00 == B10 && B01 != B11 && Rank00 >= 0 && Rank01 >= 0 && Rank11 >= 0
		&& Rank00 > Rank01 && Rank00 > Rank11;
	if (bSouth)
	{
		AppendTriangle(Out, B00, 0, 6, 7);
		AppendTriangle(Out, B00, 0, 7, 1);
		AppendTriangle(Out, B01, 2, Centre, 6);
		AppendTriangle(Out, B01, 2, 5, Centre);
		AppendTriangle(Out, B11, 3, 7, Centre);
		AppendTriangle(Out, B11, 3, Centre, 5);
		return Out;
	}
	const bool bNorth = B01 == B11 && B00 != B10 && Rank01 >= 0 && Rank00 >= 0 && Rank10 >= 0
		&& Rank01 > Rank00 && Rank01 > Rank10;
	if (bNorth)
	{
		AppendTriangle(Out, B01, 2, 3, 7);
		AppendTriangle(Out, B01, 2, 7, 6);
		AppendTriangle(Out, B00, 0, 6, Centre);
		AppendTriangle(Out, B00, 0, Centre, 4);
		AppendTriangle(Out, B10, 1, 4, Centre);
		AppendTriangle(Out, B10, 1, Centre, 7);
		return Out;
	}
	const bool bEast = B00 == B01 && B10 != B11 && Rank00 >= 0 && Rank10 >= 0 && Rank11 >= 0
		&& Rank00 > Rank10 && Rank00 > Rank11;
	if (bEast)
	{
		AppendTriangle(Out, B00, 0, 2, 5);
		AppendTriangle(Out, B00, 0, 5, 4);
		AppendTriangle(Out, B10, 1, 4, Centre);
		AppendTriangle(Out, B10, 1, Centre, 7);
		AppendTriangle(Out, B11, 3, 7, Centre);
		AppendTriangle(Out, B11, 3, Centre, 5);
		return Out;
	}
	const bool bWest = B10 == B11 && B00 != B01 && Rank10 >= 0 && Rank00 >= 0 && Rank01 >= 0
		&& Rank10 > Rank00 && Rank10 > Rank01;
	if (bWest)
	{
		AppendTriangle(Out, B10, 1, 5, 3);
		AppendTriangle(Out, B10, 1, 4, 5);
		AppendTriangle(Out, B00, 0, 6, Centre);
		AppendTriangle(Out, B00, 0, Centre, 4);
		AppendTriangle(Out, B01, 2, Centre, 6);
		AppendTriangle(Out, B01, 2, 5, Centre);
	}
	return Out;
}

// 7. One corner is the highest, and the opposite side is a pair.
FSolidQuadTriangles FSolidPriorityMidpointSplit::HighestOpposite(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	const ESolidBiome Corner[4] = { B00, B10, B01, B11 };
	const int32 Rank[4] = {
		BiomeRank(B00), BiomeRank(B10), BiomeRank(B01), BiomeRank(B11) };
	int32 High = -1;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (Rank[Index] < 0)
		{
			continue;
		}
		bool bHighest = true;
		for (int32 Other = 0; Other < 4; ++Other)
		{
			if (Other != Index && (Rank[Other] < 0 || Rank[Index] <= Rank[Other]))
			{
				bHighest = false;
			}
		}
		if (bHighest)
		{
			High = Index;
		}
	}

	FSolidQuadTriangles Out;
	if (High < 0)
	{
		return Out;
	}

	struct FCut
	{
		int32 HighCorner;
		int32 Pair0;
		int32 Pair1;
		int32 Remaining;
		int32 Tri[5][3];
	};
	// Corner order is southeast, southwest, northeast, northwest.
	// Each row is one opposite side: the high triangle, the pair's half, then the remainder.
	const FCut Cuts[] = {
		{ 0, 2, 3, 1, { { 0, 6, 4 }, { 2, 3, 7 }, { 2, 7, 6 }, { 1, 4, 6 }, { 1, 6, 7 } } },
		{ 0, 1, 3, 2, { { 0, 6, 4 }, { 1, 5, 3 }, { 1, 4, 5 }, { 2, 4, 6 }, { 2, 5, 4 } } },
		{ 1, 2, 3, 0, { { 1, 4, 7 }, { 2, 3, 7 }, { 2, 7, 6 }, { 0, 6, 7 }, { 0, 7, 4 } } },
		{ 1, 0, 2, 3, { { 1, 4, 7 }, { 0, 2, 5 }, { 0, 5, 4 }, { 3, 7, 4 }, { 3, 4, 5 } } },
		{ 2, 0, 1, 3, { { 2, 5, 6 }, { 0, 6, 7 }, { 0, 7, 1 }, { 3, 7, 6 }, { 3, 6, 5 } } },
		{ 2, 1, 3, 0, { { 2, 5, 6 }, { 1, 5, 3 }, { 1, 4, 5 }, { 0, 6, 5 }, { 0, 5, 4 } } },
		{ 3, 0, 1, 2, { { 3, 7, 5 }, { 0, 6, 7 }, { 0, 7, 1 }, { 2, 5, 7 }, { 2, 7, 6 } } },
		{ 3, 0, 2, 1, { { 3, 7, 5 }, { 0, 2, 5 }, { 0, 5, 4 }, { 1, 4, 5 }, { 1, 5, 7 } } },
	};
	for (const FCut& Cut : Cuts)
	{
		if (Cut.HighCorner != High)
		{
			continue;
		}
		if (Corner[Cut.Pair0] != Corner[Cut.Pair1] || Corner[Cut.Remaining] == Corner[Cut.Pair0])
		{
			continue;
		}
		AppendTriangle(Out, Corner[High], Cut.Tri[0][0], Cut.Tri[0][1], Cut.Tri[0][2]);
		AppendTriangle(Out, Corner[Cut.Pair0], Cut.Tri[1][0], Cut.Tri[1][1], Cut.Tri[1][2]);
		AppendTriangle(Out, Corner[Cut.Pair0], Cut.Tri[2][0], Cut.Tri[2][1], Cut.Tri[2][2]);
		AppendTriangle(Out, Corner[Cut.Remaining], Cut.Tri[3][0], Cut.Tri[3][1], Cut.Tri[3][2]);
		AppendTriangle(Out, Corner[Cut.Remaining], Cut.Tri[4][0], Cut.Tri[4][1], Cut.Tri[4][2]);
		return Out;
	}
	return Out;
}

FSolidQuadTriangles FSolidPriorityMidpointSplit::Triangles(
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome /*WestOfNorthWest*/,
	const ESolidBiome /*SouthWestOfNorthWest*/) const
{
	// 1. Four corners match.
	const FSolidQuadTriangles Four = FourMatchingCorners(B00, B10, B01, B11);
	if (Four.Count > 0)
	{
		return ApplyShelf(Four, B00, B10, B01, B11);
	}
	// 2. Opposite sides are pairs.
	const FSolidQuadTriangles Sides = OppositeSides(B00, B10, B01, B11);
	if (Sides.Count > 0)
	{
		return ApplyShelf(Sides, B00, B10, B01, B11);
	}
	// 3. Both diagonals are pairs.
	const FSolidQuadTriangles Band = Checker(B00, B10, B01, B11);
	if (Band.Count > 0)
	{
		return ApplyShelf(Band, B00, B10, B01, B11);
	}
	// 4. One diagonal is a pair.
	const FSolidQuadTriangles Diagonal = OneDiagonal(B00, B10, B01, B11);
	if (Diagonal.Count > 0)
	{
		return ApplyShelf(Diagonal, B00, B10, B01, B11);
	}
	// 5. Three corners match.
	const FSolidQuadTriangles Three = ThreeMatchingCorners(B00, B10, B01, B11);
	if (Three.Count > 0)
	{
		return ApplyShelf(Three, B00, B10, B01, B11);
	}
	// 6. Two adjacent corners are the highest material.
	const FSolidQuadTriangles High = HighestAdjacent(B00, B10, B01, B11);
	if (High.Count > 0)
	{
		return ApplyShelf(High, B00, B10, B01, B11);
	}
	// 7. One corner is the highest, and the opposite side is a pair.
	const FSolidQuadTriangles Opposite = HighestOpposite(B00, B10, B01, B11);
	if (Opposite.Count > 0)
	{
		return ApplyShelf(Opposite, B00, B10, B01, B11);
	}
	// None of the seven cases. The northwest corner.
	return ApplyShelf(
		TwoTriangles(
			BorderBiome(Preferred, B00, B11, B10),
			BorderBiome(Preferred, B00, B01, B11),
			false),
		B00, B10, B01, B11);
}
