#include "SolidBiome.h"

FSolidBiome::FSolidBiome(
	const FLinearColor InColor,
	const TCHAR* InName,
	const float InHeightOffsetCm,
	const int32 InSurfacePreference,
	const TCHAR InMarker,
	const bool bInImpassable,
	const bool bInPaintable,
	const bool bInFlatColour)
	: Tint(InColor)
	, Label(InName)
	, ShelfCm(InHeightOffsetCm)
	, Preference(InSurfacePreference)
	, Letter(InMarker)
	, bImpassable(bInImpassable)
	, bPaintable(bInPaintable)
	, bFlatColour(bInFlatColour)
{
}

FLinearColor FSolidBiome::Color() const
{
	return Tint;
}

const TCHAR* FSolidBiome::Name() const
{
	return Label;
}

float FSolidBiome::HeightOffsetCm() const
{
	return ShelfCm;
}

int32 FSolidBiome::SurfacePreference() const
{
	return Preference;
}

TCHAR FSolidBiome::Marker() const
{
	return Letter;
}

bool FSolidBiome::Impassable() const
{
	return bImpassable;
}

bool FSolidBiome::Paintable() const
{
	return bPaintable;
}

bool FSolidBiome::FlatColour() const
{
	return bFlatColour;
}

const FSolidBiome& SolidBiome::Of(const ESolidBiome Biome)
{
	static const FSolidBiome Grassland(
		FLinearColor(0.45f, 0.72f, 0.28f), TEXT("Grassland"), 100.f, 0, TEXT('.'), false, true, false);
	static const FSolidBiome Forest(
		FLinearColor(0.08f, 0.28f, 0.08f), TEXT("Forest"), 100.f, 2, TEXT('#'), false, true, true);
	static const FSolidBiome Mountain(
		FLinearColor(0.16f, 0.16f, 0.18f), TEXT("Mountain"), 1000.f, 4, TEXT('M'), true, true, false);
	static const FSolidBiome Town(
		FLinearColor(0.45f, 0.28f, 0.12f), TEXT("Town"), 100.f, 0, TEXT('o'), false, true, true);
	static const FSolidBiome Desert(
		FLinearColor(0.85f, 0.75f, 0.32f), TEXT("Desert"), 100.f, 2, TEXT('^'), false, true, true);
	static const FSolidBiome Swamp(
		FLinearColor(0.22f, 0.32f, 0.18f), TEXT("Swamp"), 100.f, 0, TEXT('S'), false, false, true);
	static const FSolidBiome Sea(
		FLinearColor(0.15f, 0.35f, 0.75f), TEXT("Sea"), 0.f, 8, TEXT('~'), true, true, true);
	static const FSolidBiome River(
		FLinearColor(0.15f, 0.35f, 0.75f), TEXT("River"), 0.f, 6, TEXT('*'), true, true, true);
	static const FSolidBiome Road(
		FLinearColor(0.52f, 0.38f, 0.20f), TEXT("Road"), 100.f, 0, TEXT('R'), false, false, false);
	static const FSolidBiome Unknown(
		FLinearColor(0.4f, 0.4f, 0.4f), TEXT("Unknown"), 100.f, 1, TEXT('.'), false, false, false);

	switch (Biome)
	{
	case ESolidBiome::Grassland: return Grassland;
	case ESolidBiome::Forest: return Forest;
	case ESolidBiome::Mountain: return Mountain;
	case ESolidBiome::Town: return Town;
	case ESolidBiome::Desert: return Desert;
	case ESolidBiome::Swamp: return Swamp;
	case ESolidBiome::Sea: return Sea;
	case ESolidBiome::River: return River;
	case ESolidBiome::Road: return Road;
	default: return Unknown;
	}
}

ESolidBiome SolidTerrainTypes::BorderBiome(
	const ESolidBiome Preferred,
	const ESolidBiome A,
	const ESolidBiome B,
	const ESolidBiome C)
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

bool SolidTerrainTypes::FadeWeight(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	float OutWeights[4])
{
	const ESolidBiome Corners[4] = { B00, B10, B01, B11 };
	const FSolidBiome& First = SolidBiome::Of(B00);
	if (!First.FlatColour())
	{
		return false;
	}

	const float Shelf = First.HeightOffsetCm();
	ESolidBiome Other = B00;
	for (const ESolidBiome Corner : Corners)
	{
		const FSolidBiome& Ground = SolidBiome::Of(Corner);
		if (!Ground.FlatColour() || Ground.HeightOffsetCm() != Shelf)
		{
			return false;
		}
		if (Corner == B00)
		{
			continue;
		}
		if (Other == B00)
		{
			Other = Corner;
		}
		else if (Corner != Other)
		{
			return false;
		}
	}
	if (Other == B00)
	{
		return false;
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		OutWeights[Index] = Corners[Index] == B00 ? 0.f : 1.f;
	}
	return true;
}

bool SolidTerrainTypes::DiagonalCut(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	int32& OutCorner)
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

SolidTerrainTypes::FSolidQuadTriangles SolidTerrainTypes::QuadTriangles(
	const ESolidQuadMaterial Method,
	const ESolidBiome Preferred,
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	const ESolidBiome WestOfNorthWest)
{
	FSolidQuadTriangles Out;
	Out.First = BorderBiome(Preferred, B00, B11, B10);
	Out.Second = BorderBiome(Preferred, B00, B01, B11);
	switch (Method)
	{
	case ESolidQuadMaterial::NorthWest:
		break;
	case ESolidQuadMaterial::NorthWestCut:
	{
		// Mesh order is southeast, southwest, northeast, northwest.
		const bool bNorthWestAlone = B01 == B10 && B10 == B00 && B00 != B11;
		if (bNorthWestAlone)
		{
			Out.bFlip = true;
			Out.First = B00;
			Out.Second = B11;
			break;
		}

		const bool bSouthContinuesWest = B10 != B11 && B01 == B11 && B00 == B11 && WestOfNorthWest == B10;
		if (bSouthContinuesWest)
		{
			Out.First = B10;
			Out.Second = B11;
			break;
		}
		break;
	}
	case ESolidQuadMaterial::OddCorner:
	{
		int32 CutCorner = 0;
		if (!DiagonalCut(B00, B10, B01, B11, CutCorner))
		{
			break;
		}

		const ESolidBiome Corners[4] = { B00, B10, B01, B11 };
		const ESolidBiome Odd = Corners[CutCorner];
		const ESolidBiome Majority = Corners[(CutCorner + 1) % 4];
		Out.bFlip = CutCorner == 0 || CutCorner == 3;
		const bool bOddOnFirst = Out.bFlip ? (CutCorner == 0) : (CutCorner == 1);
		Out.First = bOddOnFirst ? Odd : Majority;
		Out.Second = bOddOnFirst ? Majority : Odd;
		break;
	}
	}
	return Out;
}
