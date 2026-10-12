#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidCaveInterior.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Meagre Dungeon chamber. A closed rock room with the mouth on one wall.
 * The captain arrives alone. Clicking the mouth to leave is later; F10 leaves.
 */
UCLASS()
class SOLIDCORE1_API ASolidCaveInterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidCaveInterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void BuildVisuals();

	/** Feet position inside the room. */
	FVector GetCaptainSpawnWorld() const;
	float GetCaptainYaw() const;

	UStaticMeshComponent* GetFloorMesh() const { return FloorMesh; }
	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }
	UPointLightComponent* GetRoomLight() const { return RoomLight; }

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
};
