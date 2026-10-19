#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidStrongholdInterior.generated.h"

class UStaticMeshComponent;

/**
 * An actor drawn for one stronghold visit. Dungeon and tower interiors are the kinds.
 * The stronghold names which kind, and fills it.
 */
UCLASS()
class SOLIDCORE1_API ASolidStrongholdInterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidStrongholdInterior();

	/** Feet position where the captain arrives. */
	virtual FVector GetCaptainSpawnWorld() const { return FVector::ZeroVector; }

	/** View on arrival. */
	virtual FRotator GetCaptainView() const { return FRotator::ZeroRotator; }

	/** Dark opening a click uses to leave. Null when this interior has none. */
	virtual UStaticMeshComponent* GetPassageMesh() const { return nullptr; }

	void SetStrongholdCell(FIntPoint Cell) { StrongholdCell = Cell; }
	FIntPoint GetStrongholdCell() const { return StrongholdCell; }

private:
	FIntPoint StrongholdCell = FIntPoint::ZeroValue;
};
