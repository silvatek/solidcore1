#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidDungeonInterior.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Dungeon chamber shared by every size. Meagre, the cave, is an irregular den of fourteen hexes.
 * Rock follows that outline. The west entrance is wider than the captain, with the lip outside the flat.
 * The captain arrives alone. A left click on the dark opening leaves. F10 leaves too.
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

	/** Cave pieces from the furnishing catalog, on the single Meagre floor. No-op without a world. */
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

	/** Rock around the hex outline. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> WallShell;

	/** Hidden box on each hex. The open notch has none, so the walk matches the outline. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> FloorWalk;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> MouthMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UPointLightComponent> RoomLight;

	/** Yellow hex edges. No collision; the starter pieces sit on these cells. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> FloorGrid;
};
