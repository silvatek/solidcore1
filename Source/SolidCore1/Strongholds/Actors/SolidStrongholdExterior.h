#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidStrongholdExterior.generated.h"

class UStaticMeshComponent;

/**
 * An actor drawn for one stronghold. Dungeon and tower exteriors are the kinds.
 * The stronghold names which kind, and fills it.
 */
UCLASS()
class SOLIDCORE1_API ASolidStrongholdExterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidStrongholdExterior();

	/** Dark opening a click uses to enter. Null when this exterior has none. */
	virtual UStaticMeshComponent* GetPassageMesh() const { return nullptr; }

	void SetStrongholdCell(FIntPoint Cell) { StrongholdCell = Cell; }
	FIntPoint GetStrongholdCell() const { return StrongholdCell; }

private:
	FIntPoint StrongholdCell = FIntPoint::ZeroValue;
};
