#pragma once

#include "CoreMinimal.h"
#include "SolidStrongholdExterior.h"
#include "SolidDungeonExterior.generated.h"

class UStaticMeshComponent;

/**
 * Exterior of a dungeon stronghold. It is the rock on that stronghold's map cell.
 * Local +X faces the exit. The dark mouth is the clickable opening.
 * The stone is rounded and lumpy inside the face size. A click on the stone does not enter.
 */
UCLASS()
class SOLIDCORE1_API ASolidDungeonExterior : public ASolidStrongholdExterior
{
	GENERATED_BODY()

public:
	ASolidDungeonExterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Rock size, from the dungeon that owns this exterior. */
	void SetFace(float InFaceWidthCm, float InFaceHeightCm, float InSlopeRunCm);
	void BuildVisuals();

	UStaticMeshComponent* GetRockMesh() const { return RockMesh; }
	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> RockMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> MouthMesh;

private:
	float FaceWidthCm = 0.f;
	float FaceHeightCm = 0.f;
	float SlopeRunCm = 0.f;

	void DiscardParts();
	UStaticMeshComponent* AddPart(
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const TCHAR* DebugName);
};
