#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "SolidFortressInterior.generated.h"

class UStaticMeshComponent;

/**
 * One fortress visit, at the origin.
 * The outer wall is a closed rectangle. No gate and no buildings yet.
 * Plan north is local -X, the gate side of the shell.
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

	const TArray<TObjectPtr<UStaticMeshComponent>>& GetCurtainMeshes() const { return Curtain; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TArray<TObjectPtr<UStaticMeshComponent>> Curtain;

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
