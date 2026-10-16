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

	/** True when the point lies on the deck or either ramp. */
	bool BridgeOpens(const SolidBridge::FBridge& Bridge, float X, float Y);

	/**
	 * Open ground, or the river channel where a bridge stands.
	 * An empty bridge list leaves the channel closed.
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
	 * is zero when the step would enter closed ground. A step that starts on
	 * closed ground is left alone so the character can walk back out.
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
