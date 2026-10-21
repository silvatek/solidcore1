#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidMonsterMeasure.generated.h"

class UStaticMeshComponent;

/**
 * Posts and a ground grid of known size, beside one reptile.
 * A screenshot can compare the mesh with these. They do not block a walk or a click.
 */
namespace SolidMonsterMeasure
{
	/** Red cube in one grid cell (cm). */
	inline constexpr float RedPostCm = 100.f;
	/** Blue cube in another grid cell (cm). */
	inline constexpr float BluePostCm = 200.f;
	/** Red cell, measured from the grid centre (cm). */
	inline constexpr float RedPostXCm = 150.f;
	inline constexpr float RedPostSideCm = 150.f;
	/** Blue cell, measured from the grid centre (cm). */
	inline constexpr float BluePostXCm = -150.f;
	inline constexpr float BluePostSideCm = 150.f;
	/** Reptile cell, measured from the grid centre (cm). */
	inline constexpr float MonsterXCm = 150.f;
	inline constexpr float MonsterYCm = -150.f;
	/** How far the captain stands back from the reptile, along the grid's local −X (cm). */
	inline constexpr float ViewStandCm = 450.f;
	/** Camera pitch when View reptile places the captain (degrees, negative looks down). */
	inline constexpr float ViewPitchDeg = -30.f;

	/** Captain feet on the grid, local to the grid centre, looking toward the reptile. */
	inline FVector2D ViewStandLocalXY()
	{
		return FVector2D(MonsterXCm - ViewStandCm, MonsterYCm);
	}
	/** Cube width so a post sits inside its cell (cm). */
	inline constexpr float PostWidthCm = 70.f;

	/** Half the painted square, measured from the reptile's feet (cm). */
	inline constexpr float GridHalfCm = 400.f;
	/** Distance from one grid line to the next (cm). */
	inline constexpr float GridStepCm = 100.f;
	/** Raised paint so the lines read on the grass (cm). */
	inline constexpr float LineWidthCm = 10.f;
	inline constexpr float LineThicknessCm = 20.f;

	inline constexpr int32 GridLinesPerAxis = static_cast<int32>(GridHalfCm / GridStepCm) * 2 + 1;
	inline constexpr int32 GridLineCount = GridLinesPerAxis * 2;
}

UCLASS()
class SOLIDCORE1_API ASolidMonsterMeasure : public AActor
{
	GENERATED_BODY()

public:
	ASolidMonsterMeasure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void BuildVisuals();

	const UStaticMeshComponent* GetRedPost() const { return RedPost; }
	const UStaticMeshComponent* GetBluePost() const { return BluePost; }
	const UStaticMeshComponent* GetGridLine(int32 Index) const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Monster")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Monster")
	TObjectPtr<UStaticMeshComponent> RedPost;

	UPROPERTY(VisibleAnywhere, Category = "Monster")
	TObjectPtr<UStaticMeshComponent> BluePost;

	UPROPERTY(VisibleAnywhere, Category = "Monster")
	TObjectPtr<UStaticMeshComponent> GridLines[SolidMonsterMeasure::GridLineCount];
};
