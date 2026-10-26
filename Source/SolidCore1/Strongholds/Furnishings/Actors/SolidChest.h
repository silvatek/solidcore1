#pragma once

#include "SolidFurnishing.h"
#include "SolidChest.generated.h"

class UStaticMeshComponent;

/** A wooden chest with a half-cylinder lid. One hex. Holds items. */
UCLASS()
class SOLIDCORE1_API ASolidChest : public ASolidFurnishing
{
	GENERATED_BODY()

public:
	ASolidChest();

	virtual void BuildVisuals() override;

	float LengthCm() const;
	float WidthCm() const;
	float BodyHeightCm() const;
	float LidRadiusCm() const;

	UStaticMeshComponent* GetChestMesh() const { return ChestMesh; }
	UStaticMeshComponent* GetLid() const { return Lid; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ChestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Lid;
};
