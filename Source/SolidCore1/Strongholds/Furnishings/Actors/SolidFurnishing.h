#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "SolidFurnishing.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * A placed stronghold piece. The cell, the facing, and the ghost wash live here.
 * The subclass builds the meshes. This class does not draw a kind.
 */
UCLASS(Abstract)
class SOLIDCORE1_API ASolidFurnishing : public AActor
{
	GENERATED_BODY()

public:
	ASolidFurnishing();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	virtual int32 HexCount() const;

	/** The cell and facing this piece was put down on. */
	void SetPlacement(const SolidStrongholdFurniture::FUsed& InPlacement);
	const SolidStrongholdFurniture::FUsed& GetPlacement() const { return Placement; }
	bool HasPlacement() const { return bHasPlacement; }

	/**
	 * A washed-out copy while the piece is held.
	 * The materials have no opacity, so the ghost is a lighter colour of the same meshes.
	 */
	void SetGhost(bool bInGhost);
	bool IsGhost() const { return bGhostLook; }

	virtual void BuildVisuals();

	float FloorLiftCm() const;
	FLinearColor BeigeColor() const;

protected:
	FLinearColor Shown(const FLinearColor& Color) const;

	UStaticMeshComponent* MakePart(const TCHAR* Name);
	UStaticMesh* BasicCube() const;
	UStaticMesh* BasicCylinder() const;

	void PlacePart(
		UStaticMeshComponent* Part,
		UStaticMesh* Mesh,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& SizeCm,
		const FVector& MeshSizeCm,
		const FLinearColor& Color,
		const TCHAR* DebugName);

private:
	SolidStrongholdFurniture::FUsed Placement;
	bool bHasPlacement = false;
	bool bGhostLook = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;
};
