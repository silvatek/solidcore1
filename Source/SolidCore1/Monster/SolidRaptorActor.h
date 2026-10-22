#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidRaptor.h"
#include "SolidRaptorActor.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UAnimSequence;

/**
 * One raptor on the open world.
 * It stands and idles. It blocks a walk and ignores a ground click. It has no name.
 * FSolidRaptor owns the kind answers; this actor is the engine shell for that kind.
 */
UCLASS()
class SOLIDCORE1_API ASolidRaptor : public AActor
{
	GENERATED_BODY()

public:
	ASolidRaptor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Copy the encounter member this actor stands for, then draw it. */
	void SetRaptor(const FSolidRaptor& InRaptor);
	void BuildVisuals();

	const UCapsuleComponent* GetCapsule() const { return Capsule; }
	const USkeletalMeshComponent* GetBody() const { return Body; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Raptor")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Raptor")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, Category = "Raptor")
	TObjectPtr<USkeletalMeshComponent> Body;

	/** The encounter member this actor shows. Default until SetRaptor. */
	FSolidRaptor Raptor;

	TObjectPtr<UAnimSequence> ActiveIdle;
};
