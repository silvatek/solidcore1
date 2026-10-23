#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.h"

/**
 * One kind of ground. A terrain point and the world chart store ESolidBiome.
 * This object answers for that token.
 */
class FSolidBiome
{
public:
	explicit FSolidBiome(FLinearColor InColor);

	/** Tint for this ground. Grassland's mesh stays the Fab grass. */
	FLinearColor Color() const;

private:
	FLinearColor Tint;
};

namespace SolidBiome
{
	/** The biome for this token. The same instance on every call. */
	const FSolidBiome& Of(ESolidBiome Biome);
}
