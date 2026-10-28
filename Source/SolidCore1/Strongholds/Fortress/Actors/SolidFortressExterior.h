#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdExterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "SolidFortressExterior.generated.h"

class UStaticMeshComponent;

/**
 * Exterior of a fortress stronghold, standing on its own map cell.
 * The curtain is a closed rectangle. The gate is a dark slab on local -X, cut through the north wall.
 * Each building fills its rooms up to the curtain, and keeps the walls that face the yard.
 * A black rectangle marks that building's doorway on the yard face.
 * Placement turns local -X toward the grass. A click on the gate does not enter yet.
 */
UCLASS()
class SOLIDCORE1_API ASolidFortressExterior : public ASolidStrongholdExterior
{
	GENERATED_BODY()

public:
	ASolidFortressExterior();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The caller keeps this fortress alive. The game state owns the ones the company places. */
	void SetFortress(const SolidStronghold::FSolidFortress& InFortress) { Fortress = &InFortress; }
	void BuildVisuals();

	UStaticMeshComponent* GetGateMesh() const { return GateMesh; }
	virtual UStaticMeshComponent* GetPassageMesh() const override { return GateMesh; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetCurtainMeshes() const { return Curtain; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetBuildingMeshes() const { return Buildings; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Curtain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Buildings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> GateMesh;

private:
	/** The game state owns this fortress. Null until Configure sets it, so the first construction draws nothing. */
	const SolidStronghold::FSolidFortress* Fortress = nullptr;

	void DiscardParts();
	UStaticMeshComponent* AddPart(
		UStaticMesh* Mesh,
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const TCHAR* DebugName);
};
