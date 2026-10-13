#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Party/SolidCompany.h"
#include "SolidCharacterStage.generated.h"

class ASolidCompanionCharacter;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Plain room in front of the camera, with one companion idle inside it.
 * The companion is a stage preview: it does not follow and it is not in the party.
 */
UCLASS()
class SOLIDCORE1_API ASolidCharacterStage : public AActor
{
	GENERATED_BODY()

public:
	ASolidCharacterStage();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Place this companion in the room. The same actor is reused when the roster cycles. */
	void ShowCompanion(const FSolidCompanion& Companion);

	/** Keep the skeleton view when the roster cycles. The preview applies it on the next show. */
	void SetSkeletonLines(bool bShow);
	bool IsShowingSkeletonLines() const { return bSkeletonLines; }

	/** Bone under this screen point while the skeleton is showing. Empty otherwise. */
	FString BoneUnderPointer(float X, float Y) const;

private:
	void PaintRoom();
	void Place();
	void EnsurePreview();
	void ApplyPreview();

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UStaticMeshComponent> Backdrop;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UStaticMeshComponent> Floor;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> FillLight;

	UPROPERTY(Transient)
	TObjectPtr<ASolidCompanionCharacter> Preview;

	UPROPERTY(Transient)
	FSolidCompanion Shown;

	bool bHasCompanion = false;
	bool bRoomPainted = false;
	bool bSkeletonLines = false;
};
