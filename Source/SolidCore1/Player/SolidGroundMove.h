#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.h"

class ACharacter;
class USolidWorldMap;

namespace SolidBridge
{
	struct FBridge;
}

/**
 * Open-world footing. Sea and mountain take the whole map cell.
 * A river blocks only its channel, the width defined for that water,
 * so the rest of the cell can still be walked.
 * A bridge is the only way across the channel. Sea and mountain stay closed.
 */
namespace SolidGroundMove
{
	inline bool IsImpassable(const ESolidBiome Biome)
	{
		return Biome == ESolidBiome::Sea
			|| Biome == ESolidBiome::Mountain
			|| Biome == ESolidBiome::River;
	}

	/** SampleBiome already limits a river to its channel. */
	bool IsImpassableGround(
		const USolidWorldMap& Map,
		float X,
		float Y,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY);

	/**
	 * True when the point lies on an intact deck or on a ramp.
	 * A broken deck stays closed. Its ramps stop at the lip, so the gap beyond is closed too.
	 */
	bool BridgeOpens(const SolidBridge::FBridge& Bridge, float X, float Y);

	/**
	 * Open ground, or the river channel where an intact bridge stands.
	 * An empty bridge list leaves the channel closed.
	 * A broken span also closes the bank in front of each ramp, so a step off that lip is refused.
	 */
	bool AllowsFooting(
		const USolidWorldMap& Map,
		float X,
		float Y,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		const TArray<SolidBridge::FBridge>& Bridges);

	/**
	 * Desired is a planar step or velocity. The result keeps its length when the
	 * step stays on open ground, keeps the part that slides along an edge, and
	 * is zero when the step would enter closed ground. From closed ground, only
	 * a step that reaches open ground is kept.
	 * ProbeCm is how far ahead the footing is tested.
	 */
	FVector2D ClampPlanar(
		const FVector2D& From,
		const FVector2D& Desired,
		float ProbeCm,
		TFunctionRef<bool(float X, float Y)> Allows);

	/** False when a straight step from the character would leave open ground. */
	bool AllowsStep(const ACharacter* Character, const FVector2D& Desired);

	/** Drop movement that would enter the sea, a mountain, or the river channel. */
	void Hold(ACharacter* Character);
}
