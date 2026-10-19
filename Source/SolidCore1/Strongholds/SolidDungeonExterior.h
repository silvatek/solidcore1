#pragma once

#include "CoreMinimal.h"
#include "SolidStrongholdExterior.h"
#include "SolidDungeonExterior.generated.h"

class USolidTerrainMap;
class UStaticMeshComponent;

/**
 * Exterior of a dungeon stronghold. It is the rock on that stronghold's map cell.
 * Local +X faces the exit. The face sits just inside the next cell, so the shift
 * follows the stronghold's orientation. The dark mouth is a rounded opening.
 * The ground edge continues down to 10 cm below the lowest terrain point in the cell.
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

	/** How far the ground edge is buried under the lowest terrain point in the cell. */
	static constexpr float GroundBurialCm = 10.f;

	/**
	 * How far below the actor origin the ground edge must reach.
	 * That is 10 cm under the lowest terrain point in the map cell around ActorLocation.
	 * Zero when the terrain has no point in that cell.
	 */
	static float GroundDropCm(const USolidTerrainMap* Terrain, float SurfaceBias, const FVector& ActorLocation);

	/** Rock size and how far the face sits in front of the actor, from the dungeon that owns this exterior. */
	void SetFace(float InFaceWidthCm, float InFaceHeightCm, float InSlopeRunCm, float InFaceForwardCm);

	/** Use this drop instead of measuring the cell. A negative value measures the cell. */
	void SetGroundDrop(float InGroundDropCm);
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
	float FaceForwardCm = 0.f;
	/** Negative until a caller sets a drop. BuildVisuals then measures the cell. */
	float ChosenDropCm = -1.f;

	void DiscardParts();
	UStaticMeshComponent* AddPart(
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const TCHAR* DebugName);
};
