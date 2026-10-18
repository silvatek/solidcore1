#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidStrongholdFurniture.h"
#include "SolidTowerStronghold.h"
#include "SolidTowerInterior.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * One tower visit, at the origin, the same way the cave is.
 * Each floor is a stone disc. The arrival hole is an annular gap, so the middle stays floored.
 * A flat landing in the stair band follows that hole. The next flight starts at its far end.
 * The wall beside each landing shows that floor's number on a light plate. Ground is 1.
 * A rail follows the inner edge of each flight and the inner edge of each arrival hole.
 * Both stop at the landing, so the landing stays open onto the floor.
 * Stair treads sit on the flight. The yellow grid is the kept hexes.
 * The curved wall follows the interior taper, with the dark door on local -X.
 * A click on that door leaves. Companions stay outside.
 */
UCLASS()
class SOLIDCORE1_API ASolidTowerInterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidTowerInterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetPlan(const SolidTowerStronghold::FPlan& InPlan);
	void BuildVisuals();

	/** Pieces the company carried in. Cells that are not on this floor are skipped. */
	void PlaceFurnishings(const TArray<SolidStrongholdFurniture::FUsed>& Used);

	const SolidTowerStronghold::FPlan& GetPlan() const { return Plan; }
	int32 GetFloorCount() const { return Floors.Num(); }
	UStaticMeshComponent* GetFloor(int32 Index) const;
	int32 GetWallCount() const { return Walls.Num(); }
	UStaticMeshComponent* GetWall(int32 Index) const;
	UStaticMeshComponent* GetCapMesh() const { return CapMesh; }
	UStaticMeshComponent* GetDoorMesh() const { return DoorMesh; }
	UInstancedStaticMeshComponent* GetFloorGrid() const { return FloorGrid; }
	UInstancedStaticMeshComponent* GetStairs() const { return Stairs; }
	UInstancedStaticMeshComponent* GetRails() const { return Rails; }
	int32 GetFloorSignCount() const { return FloorSignPlates.Num(); }
	UStaticMeshComponent* GetFloorSignPlate(int32 Index) const;
	UInstancedStaticMeshComponent* GetFloorSignDigits() const { return FloorSignDigits; }

	/** Feet on the kept hex nearest the door. */
	FVector GetCaptainSpawnWorld() const;
	/** Yaw 0 looks into the tower. */
	FRotator GetCaptainView() const;

	/** Map cell this visit belongs to. A click inside leaves by that door. */
	void SetTowerCell(FIntPoint Cell) { TowerCell = Cell; }
	FIntPoint GetTowerCell() const { return TowerCell; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Floors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Walls;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> CapMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> FloorGrid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> Stairs;

	/** Waist-high rail on each flight and each arrival hole. The landing is left open. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> Rails;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UPointLightComponent>> Lights;

	/** Light plate fixed to the wall behind each floor number. Not a town sign. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> FloorSignPlates;

	/** Dark strokes of those numbers. The same cube the floors use, so the digit draws. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> FloorSignDigits;

private:
	SolidTowerStronghold::FPlan Plan = SolidTowerStronghold::Imperious;
	FIntPoint TowerCell = FIntPoint::ZeroValue;

	void DiscardParts();
};
