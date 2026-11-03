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
	const bool bFlip,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11) const
{
	auto KeepMountainTop = [](const ESolidBiome Material, const ESolidBiome A, const ESolidBiome B, const ESolidBiome C)
	{
		const ESolidBiome Corners[3] = { A, B, C };
		int32 Top = 0;
		float Best = SolidBiome::Of(A).HeightOffsetCm();
		for (int32 Index = 1; Index < 3; ++Index)
		{
			const float Height = SolidBiome::Of(Corners[Index]).HeightOffsetCm();
			if (Height > Best)
			{
				Best = Height;
				Top = Index;
			}
		}
		return Corners[Top] == ESolidBiome::Mountain ? ESolidBiome::Mountain : Material;
	};

	const ESolidBiome Corners[4] = { B00, B10, B01, B11 };
	FSolidQuadTriangles Out;
	auto Append = [&](const ESolidBiome Material, const int32 A, const int32 B, const int32 C)
	{
		if (Out.Count >= FSolidQuadTriangles::Capacity)
		{
			return;
		}
		FSolidQuadTriangle& Triangle = Out.Triangles[Out.Count];
		Triangle.Material = KeepMountainTop(Material, Corners[A], Corners[B], Corners[C]);
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

FSolidQuadTriangles FSolidNorthWestSplit::Triangles(
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome /*WestOfNorthWest*/,
	const ESolidBiome /*SouthWestOfNorthWest*/) const
{
	return TwoTriangles(
		BorderBiome(Preferred, B00, B11, B10),
		BorderBiome(Preferred, B00, B01, B11),
		false,
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
	return TwoTriangles(First, Second, bFlip, B00, B10, B01, B11);
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
	return TwoTriangles(B00, B00, false, B00, B10, B01, B11);
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
		return Four;
	}
	// 2. Opposite sides are pairs.
	const FSolidQuadTriangles Sides = OppositeSides(B00, B10, B01, B11);
	if (Sides.Count > 0)
	{
		return Sides;
	}
	// 3. Both diagonals are pairs.
	const FSolidQuadTriangles Band = Checker(B00, B10, B01, B11);
	if (Band.Count > 0)
	{
		return Band;
	}
	// 4. One diagonal is a pair. Not implemented.
	// 5. Three corners match.
	const FSolidQuadTriangles Three = ThreeMatchingCorners(B00, B10, B01, B11);
	if (Three.Count > 0)
	{
		return Three;
	}
	// 6. Two adjacent corners are the highest material. Not implemented.
	// 7. One corner is the highest, and the opposite side is a pair. Not implemented.
	return TwoTriangles(
		BorderBiome(Preferred, B00, B11, B10),
		BorderBiome(Preferred, B00, B01, B11),
		false,
		B00, B10, B01, B11);
}
