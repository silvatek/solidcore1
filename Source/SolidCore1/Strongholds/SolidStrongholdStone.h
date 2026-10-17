#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidStronghold.h"
#include "SolidStrongholdStone.generated.h"

class UStaticMeshComponent;

/**
 * A stone block on a stronghold site.
 * A click asks for a type only when that size is unlocked, larger than the company base, and already built.
 */
UCLASS()
class SOLIDCORE1_API ASolidStrongholdStone : public AActor
{
	GENERATED_BODY()

public:
	ASolidStrongholdStone();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetSite(FIntPoint Cell, ESolidStrongholdSize Size);
	void BuildVisuals();

	/** Edge length of the block, in centimetres. */
	static constexpr float BlockCm = 200.f;

	UStaticMeshComponent* GetBlockMesh() const { return BlockMesh; }
	FIntPoint GetCell() const { return Cell; }
	ESolidStrongholdSize GetSize() const { return Size; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> BlockMesh;

private:
	FIntPoint Cell = FIntPoint::ZeroValue;
	ESolidStrongholdSize Size = ESolidStrongholdSize::Humble;
};
