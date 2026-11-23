#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidNpc.generated.h"

class UAnimSequence;
class UIKRetargeter;
class UIKRigDefinition;
class UPoseableMeshComponent;
class USkeletalMesh;

/** One chain the Viking idle retargets onto the figure. */
struct FSolidNpcRetargetChain
{
	const TCHAR* Name;
	const TCHAR* StartBone;
	const TCHAR* EndBone;
};

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

	/** Load exports1 onto the body. */
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

	/** Spine, both arms, both legs, and the head. */
	static constexpr int32 RetargetChainCount = 6;

	static const FSolidNpcRetargetChain& RetargetChain(int32 Index);

	/** An IK rig whose chains are the six retarget chains, rooted at the pelvis. */
	static UIKRigDefinition* MakeRetargetRig(USkeletalMesh* Mesh);

#if WITH_EDITOR
	/**
	 * Retarget the Viking idle onto the figure.
	 * The target stand is aligned to the Viking chains, then each bone copies one to one.
	 * Chain translation stays off. The pelvis keeps the figure's standing translation.
	 */
	static UIKRetargeter* MakeIdleRetargeter(USkeletalMesh* Viking, USkeletalMesh* Figure);
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Npc")
	TObjectPtr<UPoseableMeshComponent> Body;

#if WITH_EDITOR
	bool ApplyRetargetedIdle(UPoseableMeshComponent* BodyMesh, USkeletalMesh* Figure, UAnimSequence* Idle);
#endif
};
