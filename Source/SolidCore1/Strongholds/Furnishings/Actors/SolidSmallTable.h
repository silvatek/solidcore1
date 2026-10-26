#pragma once

#include "SolidFurnishing.h"
#include "SolidSmallTable.generated.h"

class UStaticMeshComponent;

/** A wooden top on four posts. One hex. Holds nothing. */
UCLASS()
class SOLIDCORE1_API ASolidSmallTable : public ASolidFurnishing
{
	GENERATED_BODY()

public:
	ASolidSmallTable();

	static constexpr int32 LegCount = 4;

	virtual void BuildVisuals() override;

	float TopCm() const;
	float TopThicknessCm() const;
	float LegCm() const;
	float LegHeightCm() const;
	float LegInsetCm() const;
	float LegOffsetCm() const;

	UStaticMeshComponent* GetTop() const { return Top; }
	UStaticMeshComponent* GetLeg(int32 Index) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Top;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;
};
