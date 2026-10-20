#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Strongholds/SolidStronghold.h"

class ASolidTerrainStreamer;
class UStaticMeshComponent;
class UWorld;
struct FActorSpawnParameters;

#include "SolidStrongholdSite.generated.h"

/**
 * A stronghold site in the world.
 * The site is drawn as a stone block.
 * A click lists each size and type that is unlocked, larger than the company base, and already built.
 */
UCLASS()
class SOLIDCORE1_API ASolidStrongholdSite : public AActor
{
	GENERATED_BODY()

public:
	ASolidStrongholdSite();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetSite(FIntPoint Cell, ESolidStrongholdSize Size);
	void BuildVisuals();

	/**
	 * Spawns each stronghold site that has no exterior.
	 * The site is drawn as a stone block.
	 */
	static void Spawn(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdSite>>& Sites,
		bool& bSitesSpawned);

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
