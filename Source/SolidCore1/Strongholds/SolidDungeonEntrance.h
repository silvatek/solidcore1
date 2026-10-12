#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidDungeonEntrance.generated.h"

class UStaticMeshComponent;

/**
 * Stone mouth on the cliff between a cave cell and the adjacent grassland.
 * The opening is solid. A left click on the dark mouth enters. F10 Enter cave does the same.
 * Local +X faces the grass.
 */
UCLASS()
class SOLIDCORE1_API ASolidDungeonEntrance : public AActor
{
	GENERATED_BODY()

public:
	ASolidDungeonEntrance();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void BuildVisuals();

	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }
	UStaticMeshComponent* GetLeftJamb() const { return LeftJamb; }
	UStaticMeshComponent* GetRightJamb() const { return RightJamb; }
	UStaticMeshComponent* GetLintel() const { return Lintel; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> MouthMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> LeftJamb;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> RightJamb;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> Lintel;
};
