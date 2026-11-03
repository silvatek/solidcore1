#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.h"

namespace SolidTerrainMaterials
{
	struct FBiomeMaterial;
}

/**
 * One kind of ground. A terrain point and the world chart store ESolidBiome.
 * This object answers for that token: its colour, its name, its shelf, its
 * height variance, its chart letter, whether that ground is open or paintable,
 * and whether it is a flat colour. Its material is a SolidTerrainMaterials definition.
 */
class SOLIDCORE1_API FSolidBiome
{
public:
	explicit FSolidBiome(
		const SolidTerrainMaterials::FBiomeMaterial& Material,
		const TCHAR* InName,
		float InHeightOffsetCm,
		float InHeightVarianceCm,
		int32 InSurfacePreference,
		TCHAR InMarker,
		bool bInImpassable,
		bool bInPaintable,
		bool bInFlatColour);

	/** Tint for this ground. The same colour as its material definition. */
	FLinearColor Color() const;

	/** How this ground is drawn. Grassland and forest reference the grass texture. */
	const SolidTerrainMaterials::FBiomeMaterial& Material() const;

	/** The word for this ground. Unknown when the token is not a biome. */
	const TCHAR* Name() const;

	/**
	 * Vertical shelf added to a terrain point's height after the biome is chosen, in centimetres.
	 * Sea and river stay on the noise surface. Mountain rises 10 m. Other ground rises 1 m.
	 * A road shares that land shelf, so a strip does not cut a trench.
	 */
	float HeightOffsetCm() const;

	/**
	 * Noise amplitude for a point of this ground, in centimetres.
	 * SampleHeight uses it.
	 */
	float HeightVarianceCm() const;

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

	/**
	 * Forest, town, desert, swamp, sea, and river.
	 * The quad split treats these as flat colours. Forest's material is the darker grass.
	 */
	bool FlatColour() const;

private:
	const SolidTerrainMaterials::FBiomeMaterial& Ground;
	const TCHAR* Label;
	float ShelfCm;
	float VarianceCm;
	int32 Preference;
	TCHAR Letter;
	bool bImpassable;
	bool bPaintable;
	bool bFlatColour;
};

namespace SolidBiome
{
	/** The biome for this token. The same instance on every call. */
	SOLIDCORE1_API const FSolidBiome& Of(ESolidBiome Biome);
}
