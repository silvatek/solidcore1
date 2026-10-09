#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTree.generated.h"

class UStaticMeshComponent;

/** Placeholder procedural tree: brown cylinder trunk + green cone canopy. */
UCLASS()
class SOLIDCORE1_API ASolidTree : public AActor
{
	GENERATED_BODY()

public:
	ASolidTree();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Rebuild meshes/materials from current size properties. */
	void BuildVisuals();

	/** Randomize trunk/canopy proportions (call before BuildVisuals). */
	void ApplyRandomVariation(FRandomStream& Rng);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree", meta = (ClampMin = "10.0"))
	float TrunkHeightCm = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree", meta = (ClampMin = "5.0"))
	float TrunkRadiusCm = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree", meta = (ClampMin = "20.0"))
	float CanopyHeightCm = 320.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree", meta = (ClampMin = "20.0"))
	float CanopyRadiusCm = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FLinearColor TrunkColor = FLinearColor(0.28f, 0.16f, 0.07f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tree")
	FLinearColor CanopyColor = FLinearColor(0.10f, 0.32f, 0.08f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> TrunkMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> CanopyMesh;

	UMaterialInterface* MakeSolidColor(const FLinearColor& Color, const TCHAR* DebugName) const;
};
