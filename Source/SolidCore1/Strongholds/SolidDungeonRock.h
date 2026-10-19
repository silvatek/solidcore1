#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidDungeonRock.generated.h"

class UStaticMeshComponent;

/**
 * Rock exterior of a dungeon, standing on its map cell.
 * Local +X faces the exit. The dark mouth is the clickable opening.
 * The stone is rounded and lumpy inside the face size. A click on the stone does not enter.
 */
UCLASS()
class SOLIDCORE1_API ASolidDungeonRock : public AActor
{
	GENERATED_BODY()

public:
	ASolidDungeonRock();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void BuildVisuals();

	UStaticMeshComponent* GetRockMesh() const { return RockMesh; }
	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> RockMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> MouthMesh;

private:
	void DiscardParts();
	UStaticMeshComponent* AddPart(
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const TCHAR* DebugName);
};
