#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidLairEntrance.generated.h"

class UStaticMeshComponent;

/**
 * Stone frame in a cliff, between a mountain cell and the adjacent grassland.
 * Each Iglin tunnel end uses one.
 * The opening is solid. A left click on the dark mouth enters that place.
 * Local +X faces the grass. The stone sits toward the cliff of the actor origin.
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

	UStaticMeshComponent* GetMouthMesh() const { return MouthMesh; }
	UStaticMeshComponent* GetLeftJamb() const { return LeftJamb; }
	UStaticMeshComponent* GetRightJamb() const { return RightJamb; }
	UStaticMeshComponent* GetLintel() const { return Lintel; }

	/** Marks this frame as one end of the Iglin tunnels. */
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
	TObjectPtr<UStaticMeshComponent> MouthMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UStaticMeshComponent> LeftJamb;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UStaticMeshComponent> RightJamb;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lair")
	TObjectPtr<UStaticMeshComponent> Lintel;

private:
	/** False until SetLairEnd. True on a tunnel end. */
	bool bLairMouth = false;
	/** X or Y when this frame is a tunnel end. */
	uint8 EndMarker = 0;
};
