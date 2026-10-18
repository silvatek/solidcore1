#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidForagePile.generated.h"

class UStaticMeshComponent;

/** One stick or one pitch. The same cube draws every pile. It is a query hit and does not block a walk. */
UCLASS()
class SOLIDCORE1_API ASolidForagePile : public AActor
{
	GENERATED_BODY()

public:
	ASolidForagePile();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetChoice(int32 InChoiceIndex, const TCHAR* InItemId);
	void BuildVisuals();

	int32 GetChoiceIndex() const { return ChoiceIndex; }
	const TCHAR* GetItemId() const { return ItemId; }
	const UStaticMeshComponent* GetPileMesh() const { return PileMesh; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Forage")
	FLinearColor PileColor = FLinearColor(0.42f, 0.40f, 0.36f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Forage")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Forage")
	TObjectPtr<UStaticMeshComponent> PileMesh;

	int32 ChoiceIndex = INDEX_NONE;
	const TCHAR* ItemId = nullptr;
};
