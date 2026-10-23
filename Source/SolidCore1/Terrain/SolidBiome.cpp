#include "SolidBiome.h"

FSolidBiome::FSolidBiome(FLinearColor InColor)
	: Tint(InColor)
{
}

FLinearColor FSolidBiome::Color() const
{
	return Tint;
}

const FSolidBiome& SolidBiome::Of(const ESolidBiome Biome)
{
	static const FSolidBiome Grassland(FLinearColor(0.45f, 0.72f, 0.28f));
	static const FSolidBiome Forest(FLinearColor(0.08f, 0.28f, 0.08f));
	static const FSolidBiome Mountain(FLinearColor(0.16f, 0.16f, 0.18f));
	static const FSolidBiome Town(FLinearColor(0.45f, 0.28f, 0.12f));
	static const FSolidBiome Desert(FLinearColor(0.85f, 0.75f, 0.32f));
	static const FSolidBiome Swamp(FLinearColor(0.22f, 0.32f, 0.18f));
	static const FSolidBiome Sea(FLinearColor(0.15f, 0.35f, 0.75f));
	static const FSolidBiome River(FLinearColor(0.15f, 0.35f, 0.75f));
	static const FSolidBiome Road(FLinearColor(0.52f, 0.38f, 0.20f));
	static const FSolidBiome Unknown(FLinearColor(0.4f, 0.4f, 0.4f));

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
