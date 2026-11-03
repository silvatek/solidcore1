#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.h"

/** How a quad's triangles choose a material. The chunk is built with one of these. */
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

/** One triangle. Corners are indexes in mesh order 00, 10, 01, 11. */
struct FSolidQuadTriangle
{
	ESolidBiome Material = ESolidBiome::Grassland;
	int32 Corner0 = 0;
	int32 Corner1 = 0;
	int32 Corner2 = 0;
};

/** The triangles one method returns for a quad. */
struct FSolidQuadTriangles
{
	static constexpr int32 Capacity = 8;
	FSolidQuadTriangle Triangles[Capacity] = {};
	int32 Count = 0;
};

/**
 * How one 2 m quad is split into triangles. The enum selects the kind.
 * The same instance answers for every quad.
 */
class SOLIDCORE1_API FSolidQuadSplit
{
public:
	virtual ~FSolidQuadSplit();

	/** The subclass for this method. The same instance on every call. */
	static const FSolidQuadSplit& Of(ESolidQuadMaterial Method);

	/** The method the mesh and the debug line use. Change this and rebuild. */
	static constexpr ESolidQuadMaterial QuadMaterial = ESolidQuadMaterial::NorthWest;

	/**
	 * Triangles for one quad.
	 * WestOfNorthWest is the point directly west of corner 11.
	 * SouthWestOfNorthWest is the point south-west of that corner.
	 * NorthWest and OddCorner ignore both. NorthWestCut uses them for two of its cuts.
	 */
	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const = 0;

	/**
	 * Texture for one terrain triangle. Preferred is the quad's current biome.
	 * A corner on a higher shelf replaces it, so the cliff between a mountain and a
	 * river uses the mountain. Equal shelves stay on Preferred.
	 */
	ESolidBiome BorderBiome(ESolidBiome Preferred, ESolidBiome A, ESolidBiome B, ESolidBiome C) const;

protected:
	/** Two triangles on one diagonal. A mountain corner keeps the mountain. */
	FSolidQuadTriangles TwoTriangles(
		ESolidBiome First,
		ESolidBiome Second,
		bool bFlip,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11) const;
};

/** Both triangles take the northwest corner. */
class SOLIDCORE1_API FSolidNorthWestSplit : public FSolidQuadSplit
{
public:
	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const override;
};

/** The single odd corner is cut off. Any other quad takes the northwest corner. */
class SOLIDCORE1_API FSolidOddCornerSplit : public FSolidQuadSplit
{
public:
	static const FSolidOddCornerSplit& Instance();

	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const override;

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
		int32& OutCorner) const;
};

/** Three cuts, and otherwise the northwest corner. */
class SOLIDCORE1_API FSolidNorthWestCutSplit : public FSolidQuadSplit
{
public:
	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const override;
};
