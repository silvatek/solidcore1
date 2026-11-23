#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidNpc.generated.h"

class USkeletalMeshComponent;
class USkeleton;

/**
 * The MakeHuman person.
 * The mesh is the import at exports1, at the scale that import wrote.
 * The Viking idle is retargeted onto that skeleton.
 */
UCLASS()
class SOLIDCORE1_API ASolidNpc : public AActor
{
	GENERATED_BODY()

public:
	ASolidNpc();

	virtual void BeginPlay() override;

	/** Load exports1 onto the body and play the retargeted Viking idle. */
	void BuildVisuals();

	/**
	 * The idle was authored on Source. Target keeps its own proportions.
	 * Bone 0 stays on the target pose so the figure does not walk off with the clip.
	 */
	static void RetargetFrom(USkeleton* Target, const USkeleton* Source);

	/** Ten metres. South of the Dragongate mark is negative Y. */
	static constexpr float SouthOfDragongateCm = 1000.f;

	/**
	 * Actor yaw. The MakeHuman mesh looks along +Y, so 0 turns that face north,
	 * toward a captain standing on the north side of this spot.
	 */
	static constexpr float FacesCaptainYawDeg = 0.f;

	/** The stand, ten metres south of the mark. */
	static FVector2D Place(const FVector2D& MarkXY)
	{
		return FVector2D(MarkXY.X, MarkXY.Y - SouthOfDragongateCm);
	}

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Npc")
	TObjectPtr<USkeletalMeshComponent> Body;
};
