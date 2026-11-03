#pragma once

#include "SolidFurnishing.h"
#include "SolidFireplace.generated.h"

class UStaticMeshComponent;

/** The stylized campfire mesh. One hex. The ember disc stays hidden. */
UCLASS()
class SOLIDCORE1_API ASolidFireplace : public ASolidFurnishing
{
	GENERATED_BODY()

public:
	ASolidFireplace();

	virtual void BuildVisuals() override;

	float AcrossCm() const;
	float HearthCm() const;
	float EmberRadiusCm() const;
	float EmberHeightCm() const;
	FLinearColor StoneColor() const;
	FLinearColor EmberColor() const;

	UStaticMeshComponent* GetHearth() const { return Hearth; }
	UStaticMeshComponent* GetEmbers() const { return Embers; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Hearth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Embers;
};
