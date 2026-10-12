#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidStrongholdFurniture.h"
#include "SolidFurnishing.generated.h"

class UStaticMeshComponent;

/** One stronghold piece: fireplace, bed, or chest. BuildVisuals after SetKind. */
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

	void BuildVisuals();

	UStaticMeshComponent* GetBodyMesh() const { return BodyMesh; }
	UStaticMeshComponent* GetTopMesh() const { return TopMesh; }

private:
	SolidStrongholdFurniture::EKind Kind = SolidStrongholdFurniture::EKind::Fireplace;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stronghold", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TopMesh;
};
