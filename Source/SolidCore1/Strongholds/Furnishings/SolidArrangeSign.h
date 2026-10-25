#pragma once

#include "CoreMinimal.h"

namespace SolidStronghold
{
	class FSolidTower;
}

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The Arrange furniture plate on an interior wall.
 * Dark letters on a light plate, drawn as strokes, because a text component on the tower wall did not draw.
 * It is not a furnishing and it occupies no hex. It does not have to stand by the entrance.
 */
class FSolidArrangeSign
{
public:
	struct FStroke
	{
		FVector CenterCm = FVector::ZeroVector;
		FVector SizeCm = FVector::OneVector;
	};

	static const TCHAR* Label() { return TEXT("Arrange furniture"); }

	/** Depth toward the room, width across the view, height up. */
	FVector PlateSizeCm() const;

	static FLinearColor PlateColor();
	static FLinearColor TextColor();

	void Place(const FVector& InLocation, float InYawDeg);
	bool IsPresent() const { return bPresent; }
	const FVector& Location() const { return StandLocation; }
	float YawDeg() const { return FacingYawDeg; }

	/** Dark strokes for the two lines, centered on the sign. */
	void CollectStrokes(TArray<FStroke>& Out) const;

	/** The light plate, toward the wall from the strokes. */
	FVector PlateLocation() const;

	/**
	 * On the east wall of the cave, facing into the room, clear of the mouth.
	 * The plate is narrowed to that one hex side. A wider board runs through the corner rock.
	 */
	static FSolidArrangeSign ForCave();

	/** On this floor's curved wall, inset like a floor number, clear of that number and of the door. */
	static FSolidArrangeSign ForTowerFloor(const SolidStronghold::FSolidTower& Tower, int32 Floor);

	void ApplyPlate(UStaticMeshComponent* Plate, UStaticMesh* Cube, float CubeSizeCm) const;
	void AppendLetters(UInstancedStaticMeshComponent* Instances, float CubeSizeCm) const;

private:
	FVector StandLocation = FVector::ZeroVector;
	float FacingYawDeg = 0.f;
	bool bPresent = false;
	/** 1 on a tower floor. The cave shrinks the whole sign onto one wall face. */
	float MetricScale = 1.f;

	static float NaturalPlateWidthCm();

	static constexpr float CellCm = 2.4f;
	static constexpr float LetterGapCm = 1.4f;
	static constexpr float LineGapCm = 4.f;
	static constexpr float PlatePadCm = 8.f;
	static constexpr float PlateDepthCm = 6.f;
	static constexpr float StrokeDepthCm = 2.2f;
	static constexpr float PlateGapCm = 1.5f;
};
