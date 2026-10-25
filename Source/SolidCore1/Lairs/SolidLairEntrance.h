#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidLairEntrance.generated.h"

class UStaticMeshComponent;

/**
 * The dungeon rock and rounded cave mouth, on the grass beside a lair.
 * Each Iglin tunnel end uses one.
 * The actor origin is the grass stand. Local +X faces that grass.
 * The opening sits toward the cliff. A click on the dark mouth enters.
 * A click on the rock does not.
 */
UCLASS()
class SOLIDCORE1_API ASolidLairEntrance : public AActor
{
	GENERATED_BODY()

public:
	ASolidLairEntrance();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void BuildVisuals();

	UStaticMeshComponent* GetRockMesh() const { return RockMesh; }
	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }

	/** Marks this mouth as one end of the Iglin tunnels. */
	void SetLairEnd(TCHAR Marker)
	{
		bLairMouth = true;
		EndMarker = static_cast<uint8>(FChar::ToUpper(Marker));
	}

	bool IsLairMouth() const { return bLairMouth; }
	TCHAR GetEndMarker() const { return static_cast<TCHAR>(EndMarker); }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UStaticMeshComponent> RockMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UStaticMeshComponent> MouthMesh;

private:
	/** False until SetLairEnd. True on a tunnel end. */
	bool bLairMouth = false;
	/** X or Y when this mouth is a tunnel end. */
	uint8 EndMarker = 0;
};
