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

FSolidQuadTriangles FSolidPriorityMidpointSplit::Triangles(
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome /*WestOfNorthWest*/,
	const ESolidBiome /*SouthWestOfNorthWest*/) const
{
	if (B00 == B10 && B10 == B01 && B01 == B11)
	{
		return TwoTriangles(B00, B00, false, B00, B10, B01, B11);
	}

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

	if (Odd >= 0)
	{
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
		const ESolidBiome Majority = CornerBiome[Cut.Opposite];
		Append(CornerBiome[Odd], Odd, Cut.FirstMid, Cut.SecondMid);
		Append(Majority, Cut.Opposite, Cut.FirstMid, Cut.FarEndOfFirst);
		Append(Majority, Cut.Opposite, Cut.SecondMid, Cut.FirstMid);
		Append(Majority, Cut.Opposite, Cut.FarEndOfSecond, Cut.SecondMid);
		return Out;
	}

	return TwoTriangles(
		BorderBiome(Preferred, B00, B11, B10),
		BorderBiome(Preferred, B00, B01, B11),
		false,
		B00, B10, B01, B11);
}
