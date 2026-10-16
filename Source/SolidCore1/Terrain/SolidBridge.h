#pragma once

#include "CoreMinimal.h"
#include "SolidRoad.h"
#include "GameFramework/Actor.h"
#include "SolidBridge.generated.h"

class USolidWorldMap;
class UStaticMeshComponent;

/**
 * A bridge where a route crosses river water.
 * Three pieces: a ramp, a flat center, and a ramp. The center is its own mesh
 * so a later change can swap a broken span in its place. The ramps stay.
 * The center is as wide as that road and as long as the water it crosses.
 * It follows the painted strip, half a terrain quad southwest of the cell centres.
 * Each ramp runs down to its own bank and continues into the ground.
 * Local offsets are centimetres. +X is east, +Y is north.
 */
namespace SolidBridge
{
	/** Slab thickness of the deck and both ramps. */
	inline constexpr float ThicknessCm = 40.f;
	/** The walking surface sits this far above the highest ground under the center. */
	inline constexpr float RiseCm = 50.f;
	/** Drop of a ramp for each centimetre along the road. */
	inline constexpr float Grade = 0.25f;
	/** The flat center continues this far past the water onto each bank. */
	inline constexpr float BankRestCm = 100.f;
	/** A ramp keeps going this far after its top meets the ground. */
	inline constexpr float BurialCm = 250.f;
	/** The underside stays at least this far above the water. */
	inline constexpr float MinClearanceCm = 40.f;
	/** Sample step along a route, in centimetres. */
	inline constexpr float SampleStepCm = 50.f;
	/** Safety cap on a ramp that has not met the ground. */
	inline constexpr float MaxRampCm = 40000.f;

	inline FLinearColor TimberColor()
	{
		return FLinearColor(0.36f, 0.23f, 0.12f);
	}

	/**
	 * One crossing, in world centimetres.
	 * Deck, RampFrom, and RampTo are centerlines. Z is the walking surface.
	 * RampFrom[0] is Deck[0]. RampTo[0] is the other end of the deck.
	 * RampFrom walks back toward the start of the route.
	 */
	struct FBridge
	{
		float WidthCm = 0.f;
		TArray<FVector> Deck;
		TArray<FVector> RampFrom;
		TArray<FVector> RampTo;
	};

	/**
	 * One bridge for each run of river water on any solved route.
	 * HeightAt returns the terrain surface in centimetres at a world XY.
	 * @return Number of bridges written.
	 */
	int32 Collect(
		const USolidWorldMap& Map,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		const TFunction<float(float X, float Y)>& HeightAt,
		TArray<FBridge>& Out);
}

/** Ramp, flat center, and ramp. The center mesh is the piece a broken span would replace. */
UCLASS()
class SOLIDCORE1_API ASolidBridge : public AActor
{
	GENERATED_BODY()

public:
	ASolidBridge();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetBridge(const SolidBridge::FBridge& InBridge);
	void BuildVisuals();

	bool HasBridge() const { return bHasBridge; }
	const SolidBridge::FBridge& GetBridge() const { return Bridge; }

	UStaticMeshComponent* GetRampFromMesh() const { return RampFromMesh; }
	UStaticMeshComponent* GetDeckMesh() const { return DeckMesh; }
	UStaticMeshComponent* GetRampToMesh() const { return RampToMesh; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Bridge")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Bridge")
	TObjectPtr<UStaticMeshComponent> RampFromMesh;

	/** Intact center. A later change replaces this mesh for a broken span. */
	UPROPERTY(VisibleAnywhere, Category = "Bridge")
	TObjectPtr<UStaticMeshComponent> DeckMesh;

	UPROPERTY(VisibleAnywhere, Category = "Bridge")
	TObjectPtr<UStaticMeshComponent> RampToMesh;

private:
	SolidBridge::FBridge Bridge;
	bool bHasBridge = false;
};
