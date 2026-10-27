#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "SolidDungeonInterior.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Dungeon chamber shared by every size. Meagre, the cave, is fourteen irregular hexes.
 * Rock follows that outline. The west entrance is wider than the captain, with the lip outside the flat.
 * The captain arrives alone. A left click on the dark opening leaves. F10 leaves too.
 */
UCLASS()
class SOLIDCORE1_API ASolidDungeonInterior : public ASolidStrongholdInterior
{
	GENERATED_BODY()

public:
	ASolidDungeonInterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BuildVisuals();

	/** Feet position inside the room. */
	virtual FVector GetCaptainSpawnWorld() const override;
	/** Level, looking into the cave. From the Meagre Dungeon visit view. */
	virtual FRotator GetCaptainView() const override;
	float GetCaptainYaw() const;

	/**
	 * The company's list for this visit.
	 * PlaceStarterFurnishings uses it. With none set, the starter three are placed.
	 */
	void SetFurnishings(const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& Used);

	/** The company's pieces on the Meagre floor. No-op without a world. */
	void PlaceStarterFurnishings();

	UStaticMeshComponent* GetFloorMesh() const { return FloorMesh; }
	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }
	virtual UStaticMeshComponent* GetPassageMesh() const override { return MouthMesh; }
	UPointLightComponent* GetRoomLight() const { return RoomLight; }
	UInstancedStaticMeshComponent* GetFloorGrid() const { return FloorGrid; }

	/** The Arrange furniture plate on the east wall. Not a furnishing. */
	UStaticMeshComponent* GetArrangePlate() const { return ArrangePlate; }
	UInstancedStaticMeshComponent* GetArrangeLetters() const { return ArrangeLetters; }
	virtual int32 ArrangeFloorCount() const override;
	virtual bool CollectArrangeFloor(int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutCells) const override;
	virtual bool ArrangeFloorZ(int32 Floor, float& OutZ) const override;
	virtual bool ArrangeEntrance(int32 Floor, int32& OutX, int32& OutY) const override;

	virtual int32 ArrangeSignCount() const override { return ArrangePlate ? 1 : 0; }
	virtual void SetArrangeSignDone(bool bDone) override;
	virtual UStaticMeshComponent* GetArrangeSignPlate(int32 Index) const override
	{
		return Index == 0 ? ArrangePlate.Get() : nullptr;
	}

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

	/** Hex edges. No collision; the starter pieces sit on these cells. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> FloorGrid;

	/** Light plate on the east wall. A click toggles arrange mode. It occupies no hex. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> ArrangePlate;

	/** Dark strokes of the arrange sign. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> ArrangeLetters;

private:
	/** Set by SetFurnishings. BeginPlay can place the starters before that call. */
	bool bUseFurnishings = false;
	TArray<SolidStrongholdFurniture::FPlacedFurnishing> FurnishingsToPlace;
};
