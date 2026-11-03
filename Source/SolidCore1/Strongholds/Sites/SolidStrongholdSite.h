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
 * The site is a tall truncated pyramid. The base is a square, and the top square is shorter.
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

	void SetSite(FIntPoint TerrainPoint, ESolidStrongholdSize Size);
	void BuildVisuals();

	/** Pointer line while the cursor rests on this site. */
	FString Hover() const;

	/**
	 * Spawns each stronghold site that has no exterior.
	 * The site is a tall truncated pyramid.
	 */
	static void Spawn(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdSite>>& Sites,
		bool& bSitesSpawned);

	/** Base edge and height of the pyramid, in centimetres. */
	static constexpr float BlockCm = 200.f;

	/** Top edge, as a fraction of the base. 0.65 is 35% shorter. */
	static constexpr float TopScale = 0.65f;

	UStaticMeshComponent* GetBlockMesh() const { return BlockMesh; }
	FIntPoint GetTerrainPoint() const { return TerrainPoint; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold")
	TObjectPtr<UStaticMeshComponent> BlockMesh;

private:
	FIntPoint TerrainPoint = FIntPoint::ZeroValue;
	ESolidStrongholdSize Size = ESolidStrongholdSize::Humble;
};
