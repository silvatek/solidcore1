#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidMonsterActor.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UAnimSequence;

/**
 * One reptile on the open world.
 * It stands and idles. It blocks a walk and ignores a ground click. It has no name.
 */
UCLASS()
class SOLIDCORE1_API ASolidMonster : public AActor
{
	GENERATED_BODY()

public:
	ASolidMonster();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The choice index from the open-world scatter. */
	void SetChoice(int32 InChoiceIndex);
	void BuildVisuals();

	int32 GetChoiceIndex() const { return ChoiceIndex; }
	const UCapsuleComponent* GetCapsule() const { return Capsule; }
	const USkeletalMeshComponent* GetBody() const { return Body; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Monster")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Monster")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, Category = "Monster")
	TObjectPtr<USkeletalMeshComponent> Body;

	int32 ChoiceIndex = INDEX_NONE;
	TObjectPtr<UAnimSequence> ActiveIdle;
};
