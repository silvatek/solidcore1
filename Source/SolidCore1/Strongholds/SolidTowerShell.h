#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTowerStronghold.h"
#include "SolidTowerShell.generated.h"

class UStaticMeshComponent;

/**
 * Stone tube for a tower stronghold, standing on its own map cell.
 * One course per floor. The foot of each course is the head of the course below,
 * so the side is one smooth taper, closed by a flat cap at the top exterior radius.
 * The dark door is a slab on local -X.
 * A click on that door enters. The captain can walk up to the stone.
 */
UCLASS()
class SOLIDCORE1_API ASolidTowerShell : public AActor
{
	GENERATED_BODY()

public:
	ASolidTowerShell();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetPlan(const SolidTowerStronghold::FPlan& InPlan);
	void BuildVisuals();

	const SolidTowerStronghold::FPlan& GetPlan() const { return Plan; }
	UStaticMeshComponent* GetCapMesh() const { return CapMesh; }
	UStaticMeshComponent* GetDoorMesh() const { return DoorMesh; }

	/** Map cell this shell stands on. A click on the door enters that cell. */
	void SetTowerCell(FIntPoint Cell) { TowerCell = Cell; }
	FIntPoint GetTowerCell() const { return TowerCell; }

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
	SolidTowerStronghold::FPlan Plan = SolidTowerStronghold::Imperious;
	FIntPoint TowerCell = FIntPoint::ZeroValue;

	void DiscardParts();
	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FLinearColor& Color, const TCHAR* DebugName);
};
