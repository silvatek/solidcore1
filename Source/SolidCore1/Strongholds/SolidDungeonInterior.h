#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidDungeonInterior.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Dungeon chamber shared by every size. Meagre, the cave, uses the room as built.
 * A closed rock room with the mouth on one wall.
 * The captain arrives alone. Clicking the mouth to leave is later; F10 leaves.
 */
UCLASS()
class SOLIDCORE1_API ASolidDungeonInterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidDungeonInterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BuildVisuals();

	/** Feet position inside the room. */
	FVector GetCaptainSpawnWorld() const;
	float GetCaptainYaw() const;

	/** Fireplace, bed, and chest on the Meagre floor. No-op without a world. */
	void PlaceStarterFurnishings();

	UStaticMeshComponent* GetFloorMesh() const { return FloorMesh; }
	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }
	UPointLightComponent* GetRoomLight() const { return RoomLight; }
	UInstancedStaticMeshComponent* GetFloorGrid() const { return FloorGrid; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> FloorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> CeilingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> WallBack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> WallLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> WallRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> WallFront;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> MouthMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UPointLightComponent> RoomLight;

	/** Yellow hex edges. No collision; the starter pieces sit on these cells. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> FloorGrid;
};
