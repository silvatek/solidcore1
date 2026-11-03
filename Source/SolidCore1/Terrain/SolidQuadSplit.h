#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.h"

/** How a quad's triangles choose a material. The chunk is built with one of these. */
enum class ESolidQuadSplit : uint8
{
	NorthWest,
	NorthWestCut,
	PriorityMidpoint,
};

/**
 * One triangle.
 * Corners 0-3 are mesh order 00, 10, 01, 11.
 * 4, 5, 6, and 7 are the midpoints of the south, north, east, and west edges.
 */
struct FSolidQuadTriangle
{
	ESolidBiome Material = ESolidBiome::Grassland;
	int32 Corner0 = 0;
	int32 Corner1 = 0;
	int32 Corner2 = 0;

	/** True when Corner is an edge midpoint. The ends are grid corners 0-3. */
	static bool MidpointEnds(int32 Corner, int32& OutEnd0, int32& OutEnd1);
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
	static const FSolidQuadSplit& Of(ESolidQuadSplit Method);

	/** The method the mesh and the debug line use. Starts as NorthWest. */
	static ESolidQuadSplit QuadSplit;

	/** The next method in enum order. NorthWest follows PriorityMidpoint. */
	static void Cycle();

	/** The word for this method. The debug line prints it. */
	virtual const TCHAR* Name() const = 0;

	/**
	 * Triangles for one quad.
	 * WestOfNorthWest is the point directly west of corner 11.
	 * SouthWestOfNorthWest is the point south-west of that corner.
	 * NorthWest and PriorityMidpoint ignore both. NorthWestCut uses them for two of its cuts.
	 */
	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const = 0;

	/** Material name of each triangle, in order, separated by a slash. The debug line prints this. */
	FString Description(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const;

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

/** Both triangles take corner 11, northwest. A higher shelf still replaces it. */
class SOLIDCORE1_API FSolidNorthWestSplit : public FSolidQuadSplit
{
public:
	virtual const TCHAR* Name() const override;

	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const override;
};

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
class SOLIDCORE1_API FSolidNorthWestCutSplit : public FSolidQuadSplit
{
public:
	virtual const TCHAR* Name() const override;

	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const override;
};

/**
 * Four matching corners are two triangles of that corner.
 * Three matching corners keep the odd one as the triangle from that corner to the
 * midpoints of its two edges. The other three corners share the rest.
 * Any other quad still takes the northwest corner.
 */
class SOLIDCORE1_API FSolidPriorityMidpointSplit : public FSolidQuadSplit
{
public:
	virtual const TCHAR* Name() const override;

	virtual FSolidQuadTriangles Triangles(
		ESolidBiome Preferred,
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		ESolidBiome WestOfNorthWest,
		ESolidBiome SouthWestOfNorthWest) const override;
};
