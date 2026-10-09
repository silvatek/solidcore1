#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidMonolith.generated.h"

class UStaticMeshComponent;

/** Large grey slab landmark at the starting clear zone. */
UCLASS()
class SOLIDCORE1_API ASolidMonolith : public AActor
{
	GENERATED_BODY()

public:
	ASolidMonolith();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void BuildVisuals();

	/** Width (X) of the slab in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monolith", meta = (ClampMin = "50.0"))
	float WidthCm = 220.f;

	/** Thickness (Y) of the slab in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monolith", meta = (ClampMin = "20.0"))
	float ThicknessCm = 70.f;

	/** Height (Z) of the slab in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monolith", meta = (ClampMin = "100.0"))
	float HeightCm = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monolith")
	FLinearColor Color = FLinearColor(0.42f, 0.44f, 0.46f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monolith")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monolith")
	TObjectPtr<UStaticMeshComponent> SlabMesh;
};
