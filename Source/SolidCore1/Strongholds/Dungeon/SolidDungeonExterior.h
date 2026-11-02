#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdExterior.h"
#include "Terrain/SolidTerrainSeat.h"
#include "SolidDungeonExterior.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * Exterior of a dungeon stronghold. It is the rock on that stronghold's map cell.
 * Local +X faces the exit. The face sits just inside the next cell, and local +Y
 * shifts the stone onto the center of that cell's terrain points. Both follow
 * the stronghold's orientation. The mouth is a round arch, widest at the ground.
 * The ground edge continues down to 10 cm below the lowest terrain point under the rock.
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

	/** How far the ground edge is buried under the lowest terrain point under the rock. */
	static constexpr float GroundBurialCm = SolidTerrainSeat::BurialCm;

	/**
	 * Rock size, how far the face sits in front of the actor, and the local +Y
	 * shift onto the shelf center. The dungeon that owns this exterior supplies them.
	 */
	void SetFace(
		float InFaceWidthCm,
		float InFaceHeightCm,
		float InSlopeRunCm,
		float InFaceForwardCm,
		float InShelfLateralCm);

	/** Use this drop instead of measuring the rock's footprint. A negative value measures that footprint. */
	void SetGroundDrop(float InGroundDropCm);
	void BuildVisuals();

	UStaticMeshComponent* GetRockMesh() const { return RockMesh; }
	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }
	virtual UStaticMeshComponent* GetPassageMesh() const override { return MouthMesh; }

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
	float ShelfLateralCm = 0.f;
	/** Negative until a caller sets a drop. BuildVisuals then measures the footprint. */
	float ChosenDropCm = -1.f;

	void DiscardParts();
	UStaticMeshComponent* AddPart(
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const TCHAR* DebugName);
};

/**
 * The rock and the rounded mouth a dungeon exterior builds.
 * The opening is at local X = 0 and faces +X. The rock runs back to -SlopeRun.
 * The mouth front is at local X = 0 and the plug runs back by Depth.
 */
namespace SolidDungeonExteriorPrivate
{
	/** The rock's plan. The mesh front is local X = 0 and the slope runs back by SlopeRunCm. */
	inline SolidTerrainSeat::FFootprint RockFootprint(
		const float FaceWidthCm,
		const float SlopeRunCm,
		const FVector2D& Face)
	{
		SolidTerrainSeat::FFootprint Seat;
		Seat.LocalCenter = FVector2D(Face.X - SlopeRunCm * 0.5f, Face.Y);
		Seat.HalfXCm = SlopeRunCm * 0.5f;
		Seat.HalfYCm = FaceWidthCm * 0.5f;
		return Seat;
	}

	UStaticMesh* BuildRock(
		UObject* Outer,
		float FaceWidth,
		float FaceHeight,
		float SlopeRun,
		float OpeningWidth,
		float OpeningHeight,
		float BurialCm);

	UStaticMesh* BuildMouth(
		UObject* Outer,
		float OpeningWidth,
		float OpeningHeight,
		float Depth,
		float BurialCm);
}
