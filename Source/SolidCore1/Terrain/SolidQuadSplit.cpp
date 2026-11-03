#include "SolidQuadSplit.h"
#include "SolidBiome.h"

FSolidQuadSplit::~FSolidQuadSplit() = default;

ESolidQuadSplit FSolidQuadSplit::QuadSplit = ESolidQuadSplit::NorthWest;

void FSolidQuadSplit::Cycle()
{
	switch (QuadSplit)
	{
	case ESolidQuadSplit::NorthWest:
		QuadSplit = ESolidQuadSplit::OddCorner;
		break;
	case ESolidQuadSplit::OddCorner:
		QuadSplit = ESolidQuadSplit::NorthWestCut;
		break;
	case ESolidQuadSplit::NorthWestCut:
	default:
		QuadSplit = ESolidQuadSplit::NorthWest;
		break;
	}
}

const TCHAR* FSolidNorthWestSplit::Name() const
{
	return TEXT("NorthWest");
}

const TCHAR* FSolidOddCornerSplit::Name() const
{
	return TEXT("OddCorner");
}

const TCHAR* FSolidNorthWestCutSplit::Name() const
{
	return TEXT("NorthWestCut");
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
	switch (Method)
	{
	case ESolidQuadSplit::OddCorner:
		return FSolidOddCornerSplit::Instance();
	case ESolidQuadSplit::NorthWestCut:
		return NorthWestCut;
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

const FSolidOddCornerSplit& FSolidOddCornerSplit::Instance()
{
	static const FSolidOddCornerSplit Split;
	return Split;
}

FSolidQuadTriangles FSolidOddCornerSplit::Triangles(
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome /*WestOfNorthWest*/,
	const ESolidBiome /*SouthWestOfNorthWest*/) const
{
	ESolidBiome First = BorderBiome(Preferred, B00, B11, B10);
	ESolidBiome Second = BorderBiome(Preferred, B00, B01, B11);
	bool bFlip = false;
	int32 CutCorner = 0;
	if (DiagonalCut(B00, B10, B01, B11, CutCorner))
	{
		const ESolidBiome Corners[4] = { B00, B10, B01, B11 };
		const ESolidBiome Odd = Corners[CutCorner];
		const ESolidBiome Majority = Corners[(CutCorner + 1) % 4];
		bFlip = CutCorner == 0 || CutCorner == 3;
		const bool bOddOnFirst = bFlip ? (CutCorner == 0) : (CutCorner == 1);
		First = bOddOnFirst ? Odd : Majority;
		Second = bOddOnFirst ? Majority : Odd;
	}
	return TwoTriangles(First, Second, bFlip, B00, B10, B01, B11);
}

bool FSolidOddCornerSplit::DiagonalCut(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	int32& OutCorner) const
{
	const ESolidBiome Corners[4] = { B00, B10, B01, B11 };
	for (int32 Odd = 0; Odd < 4; ++Odd)
	{
		const FSolidBiome& Single = SolidBiome::Of(Corners[Odd]);
		if (!Single.FlatColour())
		{
			continue;
		}

		const float Shelf = Single.HeightOffsetCm();
		ESolidBiome Majority = Corners[Odd];
		bool bMajority = false;
		bool bMatch = true;
		for (int32 Index = 0; Index < 4; ++Index)
		{
			if (Index == Odd)
			{
				continue;
			}
			const FSolidBiome& Ground = SolidBiome::Of(Corners[Index]);
			if (!Ground.FlatColour() || Ground.HeightOffsetCm() != Shelf)
			{
				bMatch = false;
				break;
			}
			if (!bMajority)
			{
				Majority = Corners[Index];
				bMajority = true;
			}
			else if (Corners[Index] != Majority)
			{
				bMatch = false;
				break;
			}
		}
		if (bMatch && bMajority && Majority != Corners[Odd])
		{
			OutCorner = Odd;
			return true;
		}
	}
	return false;
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
