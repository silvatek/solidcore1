#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.h"

/**
 * One kind of ground. A terrain point and the world chart store ESolidBiome.
 * This object answers for that token: its colour, its name, its shelf, its
 * chart letter, and whether that ground is open or paintable.
 */
class SOLIDCORE1_API FSolidBiome
{
public:
	explicit FSolidBiome(
		FLinearColor InColor,
		const TCHAR* InName,
		float InHeightOffsetCm,
		int32 InSurfacePreference,
		TCHAR InMarker,
		bool bInImpassable,
		bool bInPaintable);

	/** Tint for this ground. Grassland's mesh stays the Fab grass. */
	FLinearColor Color() const;

	/** The word for this ground. Unknown when the token is not a biome. */
	const TCHAR* Name() const;

	/**
	 * Vertical shelf added to a terrain point's height after the biome is chosen, in centimetres.
	 * Sea and river stay on the noise surface. Mountain rises 10 m. Other ground rises 1 m.
	 * A road shares that land shelf, so a strip does not cut a trench.
	 */
	float HeightOffsetCm() const;

	/** Lower is better. Smaller than one road step, so it only breaks a tie. */
	int32 SurfacePreference() const;

	/**
	 * Chart letter for this ground. Swamp is S and a road is R.
	 * A text map loads those two as grassland, because they are not paintable.
	 */
	TCHAR Marker() const;

	/** Sea, mountain, and a river channel block a step. */
	bool Impassable() const;

	/** The chart brush paints this ground. Swamp and road are not paint biomes. */
	bool Paintable() const;

private:
	FLinearColor Tint;
	const TCHAR* Label;
	float ShelfCm;
	int32 Preference;
	TCHAR Letter;
	bool bImpassable;
	bool bPaintable;
};

namespace SolidBiome
{
	/** The biome for this token. The same instance on every call. */
	SOLIDCORE1_API const FSolidBiome& Of(ESolidBiome Biome);
}
