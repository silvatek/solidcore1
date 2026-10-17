#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTowerStronghold.h"
#include "SolidTowerInterior.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * One tower visit, at the origin, the same way the den is.
 * Each floor is a stone disc. The arrival hole is an annular gap, so the middle stays floored.
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

	const SolidTowerStronghold::FPlan& GetPlan() const { return Plan; }
	int32 GetFloorCount() const { return Floors.Num(); }
	UStaticMeshComponent* GetFloor(int32 Index) const;
	int32 GetWallCount() const { return Walls.Num(); }
	UStaticMeshComponent* GetWall(int32 Index) const;
	UStaticMeshComponent* GetCapMesh() const { return CapMesh; }
	UStaticMeshComponent* GetDoorMesh() const { return DoorMesh; }
	UInstancedStaticMeshComponent* GetFloorGrid() const { return FloorGrid; }
	UInstancedStaticMeshComponent* GetStairs() const { return Stairs; }

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UPointLightComponent>> Lights;

private:
	SolidTowerStronghold::FPlan Plan = SolidTowerStronghold::Imperious;
	FIntPoint TowerCell = FIntPoint::ZeroValue;

	void DiscardParts();
};
