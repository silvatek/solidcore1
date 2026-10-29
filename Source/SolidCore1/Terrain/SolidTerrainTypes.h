#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.generated.h"

/** Coarse gameplay biome for a terrain simulation point. */
UENUM(BlueprintType)
enum class ESolidBiome : uint8
{
	Grassland UMETA(DisplayName = "Grassland"),
	Forest UMETA(DisplayName = "Forest"),
	Mountain UMETA(DisplayName = "Mountain"),
	Town UMETA(DisplayName = "Town"),
	Desert UMETA(DisplayName = "Desert"),
	Swamp UMETA(DisplayName = "Swamp"),
	Sea UMETA(DisplayName = "Sea"),
	River UMETA(DisplayName = "River"),
	Road UMETA(DisplayName = "Road"),
};

/** One cell in the world simulation grid (gameplay + heightfield source). */
USTRUCT(BlueprintType)
struct FSolidTerrainPoint
{
	GENERATED_BODY()

	/** World X in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float X = 0.f;

	/** World Y in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Y = 0.f;

	/** World surface height in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Height = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	ESolidBiome Biome = ESolidBiome::Grassland;

	/** Constant. Play ignores it. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Threat = 0.f;

	/** Exploration fog / obscurity in [0, 1] (1 = fully fogged). Distance bands: SolidTerrainFog.h. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Fog = 1.f;
};

namespace SolidTerrainTypes
{
	/**
	 * Texture for one terrain triangle. Preferred is the quad's current biome.
	 * A corner on a higher shelf replaces it, so the cliff between a mountain and a
	 * river uses the mountain. Equal shelves stay on Preferred.
	 */
	ESolidBiome BorderBiome(ESolidBiome Preferred, ESolidBiome A, ESolidBiome B, ESolidBiome C);

	/**
	 * Corner weights for a flat-colour fade, in mesh order 00, 10, 01, 11.
	 * Weight 0 is corner 00's biome. Weight 1 is the other flat colour on that shelf.
	 * The halfway line between those rows is the middle of the fade.
	 * False when the quad keeps the single material it uses now: one biome, three
	 * biomes, a road, grassland, or a higher shelf.
	 */
	bool FadeWeight(
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		float OutWeights[4]);

	/**
	 * True when the quad is cut on the diagonal that isolates one corner.
	 * OutCorner is that corner in mesh order 00, 10, 01, 11.
	 * False when the quad keeps one material: one biome, a straight pair,
	 * a checker, three biomes, a road, grassland, or a higher shelf.
	 */
	bool DiagonalCut(
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		int32& OutCorner);

	/** How a quad's two triangles choose a material. The chunk is built with one of these. */
	enum class ESolidQuadMaterial : uint8
	{
		/** Both triangles take corner 11, northwest. A higher shelf still replaces it. */
		NorthWest,
		/** The single odd corner is cut off. Any other quad takes that northwest corner. */
		OddCorner,
		/**
		 * Three cuts, and otherwise the northwest corner.
		 * The other three match and differ from the northwest: the northwest triangle
		 * keeps that corner, and the southeast triangle takes the other biome.
		 * The south corner differs, the east and southeast match the northwest, and the
		 * point west of the northwest matches the south corner: the southwest triangle
		 * takes the south corner, and the northeast triangle takes the northwest corner.
		 * The point west of the northwest differs, the east matches it, and the
		 * south-west, south, and south-east match that west point: the northeast
		 * triangle takes the northwest corner, and the southwest triangle takes the
		 * other biome.
		 */
		NorthWestCut,
	};

	/** Change this and rebuild. The mesh and the debug line both use it. */
	inline constexpr ESolidQuadMaterial QuadMaterial = ESolidQuadMaterial::NorthWestCut;

	/** Materials for the quad's two triangles, and whether the mesh diagonal is flipped. */
	struct FSolidQuadTriangles
	{
		ESolidBiome First = ESolidBiome::Grassland;
		ESolidBiome Second = ESolidBiome::Grassland;
		bool bFlip = false;
	};

	/**
	 * WestOfNorthWest is the point directly west of corner 11.
	 * SouthWestOfNorthWest is the point south-west of that corner.
	 * NorthWest and OddCorner ignore both. NorthWestCut uses them for two of its cuts.
	 */
	FSolidQuadTriangles QuadTriangles(
		ESolidQuadMaterial Method,
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest);
}
