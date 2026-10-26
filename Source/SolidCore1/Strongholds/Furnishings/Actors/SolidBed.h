#pragma once

#include "SolidFurnishing.h"
#include "SolidBed.generated.h"

class UStaticMeshComponent;

/** A wooden bed with a white pillow. Two neighbouring hexes. */
UCLASS()
class SOLIDCORE1_API ASolidBed : public ASolidFurnishing
{
	GENERATED_BODY()

public:
	ASolidBed();

	virtual SolidStrongholdFurniture::EKind GetKind() const override;
	virtual int32 HexCount() const override;
	virtual void BuildVisuals() override;

	float LengthCm() const;
	float WidthCm() const;
	float HeightCm() const;
	float PillowLengthCm() const;
	float PillowWidthCm() const;
	float PillowHeightCm() const;
	FLinearColor PillowColor() const;

	UStaticMeshComponent* GetBedMesh() const { return BedMesh; }
	UStaticMeshComponent* GetPillow() const { return Pillow; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Pillow;
};
