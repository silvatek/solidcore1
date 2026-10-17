#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTowerStronghold.h"
#include "SolidTowerShell.generated.h"

class UStaticMeshComponent;

/**
 * Stone tube for a tower stronghold, standing on its own map cell.
 * One cylinder course per floor, the radius at the bottom of that band,
 * and a flat cap of the top exterior radius. The dark door is a slab on local -X.
 * A click does not enter. The captain can walk up to the stone.
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
	int32 GetCourseCount() const { return Courses.Num(); }
	UStaticMeshComponent* GetCourse(int32 Index) const;
	UStaticMeshComponent* GetCapMesh() const { return CapMesh; }
	UStaticMeshComponent* GetDoorMesh() const { return DoorMesh; }

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

	void DiscardParts();
	UStaticMeshComponent* AddPart(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FLinearColor& Color, const TCHAR* DebugName);
};
