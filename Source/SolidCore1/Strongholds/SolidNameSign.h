#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidNameSign.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The company stronghold's name.
 * A thin dark-brown pole and a flat light-brown board, the same as a town sign.
 * One line, the name. The board faces the grass.
 */
UCLASS()
class SOLIDCORE1_API ASolidNameSign : public AActor
{
	GENERATED_BODY()

public:
	ASolidNameSign();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetStrongholdName(const FString& InName);
	/** The name on the board. One line. */
	FString GetLabel() const;
	UStaticMeshComponent* GetPoleMesh() const { return PoleMesh; }
	UStaticMeshComponent* GetBoardMesh() const { return BoardMesh; }

	void BuildVisuals();

	static constexpr int32 OutlineCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	FString StrongholdName;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<UStaticMeshComponent> PoleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<UStaticMeshComponent> BoardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<UTextRenderComponent> LabelText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<UTextRenderComponent> OutlineUp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<UTextRenderComponent> OutlineDown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<UTextRenderComponent> OutlineLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sign")
	TObjectPtr<UTextRenderComponent> OutlineRight;
};
