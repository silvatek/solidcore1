#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidDungeonEntrance.generated.h"

class UStaticMeshComponent;

/**
 * Stone mouth on the cliff between a closed cell and the adjacent grassland.
 * The cave uses one. Each Iglin tunnel end uses another, the same frame.
 * The opening is solid. A left click on the dark mouth enters that place.
 * F10 Enter cave does the same for the cave mouth only.
 * Local +X faces the grass. The stone sits toward the cliff of the actor origin.
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

	/** Marks this frame as one end of the Iglin tunnels. The cave frame stays unmarked. */
	void SetLairEnd(TCHAR Marker)
	{
		bLairMouth = true;
		EndMarker = static_cast<uint8>(FChar::ToUpper(Marker));
	}

	bool IsLairMouth() const { return bLairMouth; }
	TCHAR GetEndMarker() const { return static_cast<TCHAR>(EndMarker); }

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

private:
	/** False on the cave. True on a tunnel end. */
	bool bLairMouth = false;
	/** X or Y when this frame is a tunnel end. */
	uint8 EndMarker = 0;
};
