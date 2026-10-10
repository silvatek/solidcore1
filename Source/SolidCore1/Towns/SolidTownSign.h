#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidTownSign.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/** Welcome sign: thin dark-brown pole, flat light-brown board, white name with a black border. */
UCLASS()
class SOLIDCORE1_API ASolidTownSign : public AActor
{
	GENERATED_BODY()

public:
	ASolidTownSign();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void SetTownName(const FString& InName);
	const FString& GetTownName() const { return TownName; }
	FString GetLabel() const;

	void BuildVisuals();

	UStaticMeshComponent* GetPoleMesh() const { return PoleMesh; }
	UStaticMeshComponent* GetBoardMesh() const { return BoardMesh; }
	UTextRenderComponent* GetLabelText() const { return LabelText; }
	UTextRenderComponent* GetOutlineText(int32 Index) const;

	static constexpr int32 OutlineCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	FString TownName;

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
