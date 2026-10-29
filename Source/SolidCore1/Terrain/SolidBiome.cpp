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
