#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "SolidFortressInterior.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * One fortress visit, at the origin.
 * The outer wall is a closed rectangle. The dark gate is on local -X.
 * The Keep and the Annex stand on the far side. A ceiling closes each room at one storey.
 * Each room has a light under that ceiling. Their floors draw the hex grid.
 * Each yard face has an open doorway.
 * The east wall of the yard carries an Arrange furniture plate.
 * You walk through that gap. It does not leave. The captain arrives in the yard, inside the gate.
 * A click on the gate leaves. Companions stay outside.
 */
UCLASS()
class SOLIDCORE1_API ASolidFortressInterior : public ASolidStrongholdInterior
{
	GENERATED_BODY()

public:
	ASolidFortressInterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The caller keeps this fortress alive. The game state owns the ones the company places. */
	void SetFortress(const SolidStronghold::FSolidFortress& InFortress) { Fortress = &InFortress; }
	void SetFurnishings(const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& Used);
	void BuildVisuals();

	UStaticMeshComponent* GetGateMesh() const { return GateMesh; }
	virtual UStaticMeshComponent* GetPassageMesh() const override { return GateMesh; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetCurtainMeshes() const { return Curtain; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetBuildingMeshes() const { return Buildings; }
	UInstancedStaticMeshComponent* GetFloorGrid() const { return FloorGrid; }
	const TArray<TObjectPtr<UPointLightComponent>>& GetRoomLights() const { return RoomLights; }

	/** Feet in the yard, one hex inside the gate. */
	virtual FVector GetCaptainSpawnWorld() const override;
	/** Yaw 0 looks into the yard. */
	virtual FRotator GetCaptainView() const override;

	/** The Arrange furniture plate on the east wall of the yard. Not a furnishing. */
	UStaticMeshComponent* GetArrangePlate() const { return ArrangePlate; }
	UInstancedStaticMeshComponent* GetArrangeLetters() const { return ArrangeLetters; }
	virtual int32 ArrangeSignCount() const override { return ArrangePlate ? 1 : 0; }
	virtual void SetArrangeSignDone(bool bDone) override;
	virtual UStaticMeshComponent* GetArrangeSignPlate(int32 Index) const override
	{
		return Index == 0 ? ArrangePlate.Get() : nullptr;
	}

	virtual int32 ArrangeFloorCount() const override;
	virtual bool CollectArrangeFloor(int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutCells) const override;
	virtual bool ArrangeFloorZ(int32 Floor, float& OutZ) const override;
	virtual FVector HexPointInRoom(const FVector& HexPoint) const override;
	virtual FVector RoomPointOnHexGrid(const FVector& RoomPoint) const override;
	virtual FVector HexDirectionInRoom(const FVector& HexDirection) const override;
	virtual FVector RoomDirectionOnHexGrid(const FVector& RoomDirection) const override;
	virtual float HexFacingInRoom(float HexFacingDeg) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Curtain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Buildings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> GateMesh;

	/** Yard ground. The top is at the captain's feet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> FloorMesh;

	/** Hex edges on the Keep and the Annex. The yard has none. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> FloorGrid;

	/** One warm light under each room's ceiling. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UPointLightComponent>> RoomLights;

	/** Light plate on the east wall of the yard. A click toggles arrange mode. It occupies no hex. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> ArrangePlate;

	/** Dark strokes of the arrange sign. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UInstancedStaticMeshComponent> ArrangeLetters;

private:
	/** The game state owns this fortress. Null until Configure sets it, so the first construction draws nothing. */
	const SolidStronghold::FSolidFortress* Fortress = nullptr;
	TArray<SolidStrongholdFurniture::FPlacedFurnishing> FurnishingsToPlace;
	bool bUseFurnishings = false;

	void DiscardParts();
	void PlaceFurnishings();
	UStaticMeshComponent* AddPart(
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const TCHAR* DebugName);
};
