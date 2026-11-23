#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidNpc.generated.h"

class USkeletalMeshComponent;

/**
 * The MakeHuman person.
 * The mesh is the import at exports1, at the scale that import wrote.
 */
UCLASS()
class SOLIDCORE1_API ASolidNpc : public AActor
{
	GENERATED_BODY()

public:
	ASolidNpc();

	virtual void BeginPlay() override;

	/** Load exports1 onto the body and play the Viking idle. */
	void BuildVisuals();

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
