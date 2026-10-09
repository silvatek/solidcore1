#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidBuilding.generated.h"

class UStaticMeshComponent;

/** Placeholder town building: grey cuboid body + red cone roof. */
UCLASS()
class SOLIDCORE1_API ASolidBuilding : public AActor
{
	GENERATED_BODY()

public:
	ASolidBuilding();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void BuildVisuals();

	/** Randomize footprint / height (call before BuildVisuals). */
	void ApplyRandomVariation(FRandomStream& Rng);

	/** Planar footprint width (local X) in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building", meta = (ClampMin = "50.0"))
	float FootprintXCm = 280.f;

	/** Planar footprint depth (local Y) in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building", meta = (ClampMin = "50.0"))
	float FootprintYCm = 240.f;

	/** Grey body height in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building", meta = (ClampMin = "50.0"))
	float BodyHeightCm = 320.f;

	/** Red roof prism height in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building", meta = (ClampMin = "30.0"))
	float RoofHeightCm = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FLinearColor BodyColor = FLinearColor(0.40f, 0.40f, 0.42f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FLinearColor RoofColor = FLinearColor(0.55f, 0.10f, 0.08f);

	/** Half-extent used for non-overlap packing (max of footprint axes). */
	float GetPackingRadiusCm() const
	{
		return 0.5f * FMath::Max(FootprintXCm, FootprintYCm);
	}

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<UStaticMeshComponent> RoofMesh;
};
