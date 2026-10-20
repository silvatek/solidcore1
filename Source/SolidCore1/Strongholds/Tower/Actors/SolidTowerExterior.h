#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdExterior.h"
#include "Strongholds/Tower/SolidTower.h"
#include "SolidTowerExterior.generated.h"

class UStaticMeshComponent;

/**
 * Exterior of a tower stronghold, standing on its own map cell.
 * One course per floor. The foot of each course is the head of the course below,
 * so the side is one smooth taper, closed by a flat cap at the top exterior radius.
 * The dark door is a slab on local -X.
 * A click on that door enters. The captain can walk up to the stone.
 */
UCLASS()
class SOLIDCORE1_API ASolidTowerExterior : public ASolidStrongholdExterior
{
	GENERATED_BODY()

public:
	ASolidTowerExterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The caller keeps this tower alive. The game state owns the ones the company places. */
	void SetTower(const SolidStronghold::FSolidTower& InTower) { Tower = &InTower; }
	void BuildVisuals();

	const SolidStronghold::FPlan& GetPlan() const { check(Tower); return Tower->Plan(); }
	UStaticMeshComponent* GetCapMesh() const { return CapMesh; }
	UStaticMeshComponent* GetDoorMesh() const { return DoorMesh; }
	virtual UStaticMeshComponent* GetPassageMesh() const override { return DoorMesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Courses;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> CapMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

private:
	/** The game state owns this tower. Null until Configure sets it, so the first construction draws nothing. */
	const SolidStronghold::FSolidTower* Tower = nullptr;

	void DiscardParts();
	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FLinearColor& Color, const TCHAR* DebugName);
};
