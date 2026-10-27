#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "SolidFortressInterior.generated.h"

class UStaticMeshComponent;

/**
 * One fortress visit, at the origin.
 * The outer wall is a closed rectangle. The dark gate is on local -X.
 * The Keep and the Annex stand on the far side, each with a dark doorway on the yard face.
 * That doorway does not leave. The captain arrives in the yard, inside the gate.
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
	void BuildVisuals();

	UStaticMeshComponent* GetGateMesh() const { return GateMesh; }
	virtual UStaticMeshComponent* GetPassageMesh() const override { return GateMesh; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetCurtainMeshes() const { return Curtain; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetBuildingMeshes() const { return Buildings; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetDoorMeshes() const { return Doors; }

	/** Feet in the yard, one hex inside the gate. */
	virtual FVector GetCaptainSpawnWorld() const override;
	/** Yaw 0 looks into the yard. */
	virtual FRotator GetCaptainView() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Curtain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Buildings;

	/** Dark doorway on each building's yard face. Not a way out. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Doors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> GateMesh;

	/** Yard ground. The top is at the captain's feet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> FloorMesh;

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
