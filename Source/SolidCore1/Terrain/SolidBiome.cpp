#include "SolidBiome.h"
#include "SolidTerrainMaterials.h"

FSolidBiome::FSolidBiome(
	const SolidTerrainMaterials::FBiomeMaterial& Material,
	const TCHAR* InName,
	const float InHeightOffsetCm,
	const float InHeightVarianceCm,
	const int32 InSurfacePreference,
	const TCHAR InMarker,
	const bool bInImpassable,
	const bool bInPaintable,
	const bool bInFlatColour)
	: Ground(Material)
	, Label(InName)
	, ShelfCm(InHeightOffsetCm)
	, VarianceCm(InHeightVarianceCm)
	, Preference(InSurfacePreference)
	, Letter(InMarker)
	, bImpassable(bInImpassable)
	, bPaintable(bInPaintable)
	, bFlatColour(bInFlatColour)
{
}

FLinearColor FSolidBiome::Color() const
{
	return Ground.Color;
}

const SolidTerrainMaterials::FBiomeMaterial& FSolidBiome::Material() const
{
	return Ground;
}

const TCHAR* FSolidBiome::Name() const
{
	return Label;
}

float FSolidBiome::HeightOffsetCm() const
{
	return ShelfCm;
}

float FSolidBiome::HeightVarianceCm() const
{
	return VarianceCm;
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
		SolidTerrainMaterials::Grassland, TEXT("Grassland"), 100.f, 3000.f, 0, TEXT('.'), false, true, false);
	static const FSolidBiome Forest(
		SolidTerrainMaterials::Forest, TEXT("Forest"), 100.f, 3000.f, 2, TEXT('#'), false, true, true);
	static const FSolidBiome Mountain(
		SolidTerrainMaterials::Mountain, TEXT("Mountain"), 1000.f, 3000.f, 4, TEXT('M'), true, true, false);
	static const FSolidBiome Town(
		SolidTerrainMaterials::Town, TEXT("Town"), 100.f, 3000.f, 0, TEXT('o'), false, true, true);
	static const FSolidBiome Desert(
		SolidTerrainMaterials::Desert, TEXT("Desert"), 100.f, 3000.f, 2, TEXT('^'), false, true, true);
	static const FSolidBiome Swamp(
		SolidTerrainMaterials::Swamp, TEXT("Swamp"), 100.f, 3000.f, 0, TEXT('S'), false, false, true);
	static const FSolidBiome Sea(
		SolidTerrainMaterials::Sea, TEXT("Sea"), 0.f, 3000.f, 8, TEXT('~'), true, true, true);
	static const FSolidBiome River(
		SolidTerrainMaterials::River, TEXT("River"), 0.f, 3000.f, 6, TEXT('*'), true, true, true);
	static const FSolidBiome Road(
		SolidTerrainMaterials::Road, TEXT("Road"), 100.f, 3000.f, 0, TEXT('R'), false, false, false);
	static const FSolidBiome Unknown(
		SolidTerrainMaterials::Unknown, TEXT("Unknown"), 100.f, 3000.f, 1, TEXT('.'), false, false, false);

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
