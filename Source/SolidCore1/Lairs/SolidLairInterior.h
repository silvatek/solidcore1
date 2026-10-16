#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidLairInterior.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Iglin tunnels. A stone maze with a mouth at each end.
 * The mouths are solid. A left click on one leaves by that end.
 * Each mouth has a dim light. A torch in either hand lights the captain.
 */
UCLASS()
class SOLIDCORE1_API ASolidLairInterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidLairInterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void BuildVisuals();

	/** Puts the torch on the captain while a hand holds one, and puts it out otherwise. */
	void ApplyTorchLight(const FVector& PlayerWorldLocation, bool bHoldingTorch);

	UPointLightComponent* GetMouthLight(TCHAR End) const;
	UPointLightComponent* GetTorchLight() const { return TorchLight.Get(); }

	/** Feet position inside the end the captain came through. */
	FVector GetCaptainSpawnWorld(TCHAR End) const;
	/** Level, looking into the maze. */
	FRotator GetCaptainView(TCHAR End) const;

	UStaticMeshComponent* GetEndMouth(TCHAR End) const;
	UInstancedStaticMeshComponent* GetFloorTiles() const { return FloorTiles; }
	UInstancedStaticMeshComponent* GetWallShell() const { return WallShell; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UInstancedStaticMeshComponent> FloorTiles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UInstancedStaticMeshComponent> CeilingTiles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UInstancedStaticMeshComponent> WallShell;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UStaticMeshComponent> MouthX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UStaticMeshComponent> MouthY;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UPointLightComponent> LightNearX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UPointLightComponent> LightNearY;

	/** Follows the captain while either hand holds a torch. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UPointLightComponent> TorchLight;
};
