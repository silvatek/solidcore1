#pragma once

#include "CoreMinimal.h"
#include "Terrain/SolidBiome.h"

class ACharacter;
class USolidWorldChart;

namespace SolidBridge
{
	struct FBridge;
}

/**
 * Open-world footing. The ground is the terrain point under the foot.
 * Sea, mountain, and a river point are closed. A bridge is the only way across a river point.
 */
namespace SolidGroundMove
{
	inline bool IsImpassable(const ESolidBiome Biome)
	{
		return SolidBiome::Of(Biome).Impassable();
	}

	/** SampleBiome reads the terrain point. A river point is closed. */
	bool IsImpassableGround(
		const USolidWorldChart& Map,
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
	 * True when the point lies on either ramp, and not past the lip toward the span.
	 * The deck is not a ramp. A repaired bridge still reports its ramps.
	 */
	bool OnRamp(const SolidBridge::FBridge& Bridge, float X, float Y);

	/**
	 * Open ground, or the river channel where an intact bridge stands.
	 * An empty bridge list leaves the channel closed.
	 * A broken span also closes the bank in front of each ramp, so a step off that lip is refused.
	 */
	bool AllowsFooting(
		const USolidWorldChart& Map,
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
