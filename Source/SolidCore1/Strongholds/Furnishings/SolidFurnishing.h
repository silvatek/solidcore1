#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidStrongholdFurniture.h"
#include "SolidFurnishing.generated.h"

class UStaticMeshComponent;

/** One stronghold piece: fireplace, bed, chest, or small table. BuildVisuals after SetKind. */
UCLASS()
class SOLIDCORE1_API ASolidFurnishing : public AActor
{
	GENERATED_BODY()

public:
	ASolidFurnishing();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetKind(SolidStrongholdFurniture::EKind Kind);
	SolidStrongholdFurniture::EKind GetKind() const { return Kind; }

	/** The cell and facing this piece was put down on. */
	void SetPlacement(const SolidStrongholdFurniture::FUsed& InPlacement);
	const SolidStrongholdFurniture::FUsed& GetPlacement() const { return Placement; }
	bool HasPlacement() const { return bHasPlacement; }

	/**
	 * A washed-out copy while the piece is held.
	 * The materials have no opacity, so the ghost is a lighter colour of the same meshes.
	 */
	void SetGhost(bool bInGhost);

	void BuildVisuals();

	UStaticMeshComponent* GetBodyMesh() const { return BodyMesh; }
	UStaticMeshComponent* GetTopMesh() const { return TopMesh; }

	/** Posts under the small table. The other kinds hide these. */
	static constexpr int32 LegCount = 4;
	UStaticMeshComponent* GetLegMesh(int32 Index) const
	{
		return Index >= 0 && Index < LegCount ? LegMeshes[Index].Get() : nullptr;
	}

private:
	SolidStrongholdFurniture::EKind Kind = SolidStrongholdFurniture::EKind::Fireplace;
	SolidStrongholdFurniture::FUsed Placement;
	bool bHasPlacement = false;
	bool bGhostLook = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TopMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LegMeshes[LegCount];
};
