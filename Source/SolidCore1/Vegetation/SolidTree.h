#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTree.generated.h"

class UStaticMeshComponent;

/** A forest tree: Tree1, Tree2, or Tree3 from the stylized environment pack. */
UCLASS()
class SOLIDCORE1_API ASolidTree : public AActor
{
	GENERATED_BODY()

public:
	static constexpr int32 MeshCount = 3;

	ASolidTree();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Load the chosen pack mesh and sit its base in the ground. */
	void BuildVisuals();

	/** Pick one of the three pack meshes. Call before BuildVisuals. */
	void ApplyRandomVariation(FRandomStream& Rng);

	int32 GetMeshIndex() const { return MeshIndex; }
	const UStaticMeshComponent* GetTreeMesh() const { return TrunkMesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> TrunkMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> CanopyMesh;

private:
	int32 MeshIndex = 0;
};
