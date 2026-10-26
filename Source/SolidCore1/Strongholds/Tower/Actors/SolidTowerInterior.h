#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "Strongholds/Tower/SolidTower.h"
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
 * Stair treads sit on the flight. The yellow grid is the furniture cells.
 * The curved wall follows the interior taper, with the dark door on local -X.
 * A click on that door leaves. Companions stay outside.
 */
UCLASS()
class SOLIDCORE1_API ASolidTowerInterior : public ASolidStrongholdInterior
{
	GENERATED_BODY()

public:
	ASolidTowerInterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The caller keeps this tower alive. The game state owns the ones the company places. */
	void SetTower(const SolidStronghold::FSolidTower& InTower) { Tower = &InTower; }
	void BuildVisuals();

	/** Pieces the company carried in. Cells that are not on this floor are skipped. */
	void PlaceFurnishings(const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& Used);

	UStaticMeshComponent* GetCapMesh() const { return CapMesh; }
	UStaticMeshComponent* GetDoorMesh() const { return DoorMesh; }
	virtual UStaticMeshComponent* GetPassageMesh() const override { return DoorMesh; }
	UInstancedStaticMeshComponent* GetFloorGrid() const { return FloorGrid; }
	UInstancedStaticMeshComponent* GetArrangeLetters() const { return ArrangeLetters; }
	virtual int32 ArrangeFloorCount() const override;
	virtual bool CollectArrangeFloor(int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutCells) const override;
	virtual bool ArrangeFloorZ(int32 Floor, float& OutZ) const override;
	virtual bool ArrangeEntrance(int32 Floor, int32& OutX, int32& OutY) const override;

	virtual int32 ArrangeSignCount() const override { return ArrangePlates.Num(); }
	virtual UStaticMeshComponent* GetArrangeSignPlate(int32 Index) const override
	{
		return ArrangePlates.IsValidIndex(Index) ? ArrangePlates[Index].Get() : nullptr;
	}

	/** Feet on the cell nearest the door. */
	virtual FVector GetCaptainSpawnWorld() const override;
	/** Yaw 0 looks into the tower. */
	virtual FRotator GetCaptainView() const override;

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

	/** Arrange furniture plate on each floor, clear of the door and of the floor number. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> ArrangePlates;

	/** Dark strokes of those plates. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> ArrangeLetters;

private:
	/** The game state owns this tower. Null until Configure sets it, so the first construction draws nothing. */
	const SolidStronghold::FSolidTower* Tower = nullptr;

	void DiscardParts();
};
