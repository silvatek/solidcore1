#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Party/SolidCompany.h"
#include "SolidCompanionCharacter.generated.h"

class USkeletalMesh;
class UAnimSequence;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Companion — NPC Party member that steers toward a battle-plan slot around the Captain.
 * Uses direct CharacterMovement (no NavMesh) so it works on procedural terrain.
 * Mesh/clips default to Fab Viking via SoftObjectPtrs (Party members may use different meshes).
 */
UCLASS()
class SOLIDCORE1_API ASolidCompanionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASolidCompanionCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetFollowTarget(AActor* NewTarget);

	AActor* GetFollowTarget() const { return FollowTarget.Get(); }
	FORCEINLINE USceneComponent* GetNameLabelRoot() const { return NameLabelRoot; }
	FORCEINLINE UTextRenderComponent* GetNameLabel() const { return NameLabel; }
	FORCEINLINE UStaticMeshComponent* GetNameLabelBorder() const { return NameLabelBorder; }
	FORCEINLINE UStaticMeshComponent* GetNameLabelBackground() const { return NameLabelBackground; }

	/** Companion name. The nameplate appends the vocation initial. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Name")
	FString CharacterDisplayName = TEXT("Sam");

	void SetCharacterDisplayName(const FString& NewName);
	const FString& GetCharacterDisplayName() const { return CharacterDisplayName; }

	/** Shown on the nameplate as an initial in brackets, such as (W). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Name")
	ESolidVocation Vocation = ESolidVocation::Warrior;

	void SetVocation(ESolidVocation NewVocation);
	ESolidVocation GetVocation() const { return Vocation; }

	/** Desired distance behind the follow target (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow", meta = (ClampMin = "50.0"))
	float FollowDistance = 280.f;

	/** Stop steering when within this planar distance of the follow point (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 90.f;

	/** Beyond this planar separation, use CatchUpSpeed (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow", meta = (ClampMin = "100.0"))
	float CatchUpDistance = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	float CatchUpSpeed = 900.f;

	/** Soft lateral offset so the companion is not glued to the player's exact trail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	float SideOffset = 80.f;

	/**
	 * Index in the Party formation (0 = Sam, 1 = Alex, …).
	 * When a Party Battle Plan is active, follow point comes from this slot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow", meta = (ClampMin = "0"))
	int32 PartySlotIndex = 0;

	void SetPartySlotIndex(int32 NewSlotIndex) { PartySlotIndex = FMath::Max(0, NewSlotIndex); }
	int32 GetPartySlotIndex() const { return PartySlotIndex; }

	/** Mesh from the company roster. Applied on spawn, before visuals load. */
	void SetCompanionMesh(const TSoftObjectPtr<USkeletalMesh>& NewMesh);
	const TSoftObjectPtr<USkeletalMesh>& GetCompanionMesh() const { return CompanionMesh; }

	/** Matte shirt, trouser, and skin colors from the company record. */
	bool SetBodyColors(const FLinearColor& NewTorso, const FLinearColor& NewLegs, const FLinearColor& NewSkin);
	FLinearColor GetTorsoColor() const { return TorsoColor; }
	FLinearColor GetLegColor() const { return LegColor; }
	FLinearColor GetSkinColor() const { return SkinColor; }

	/**
	 * Height, build, and gender from the company record.
	 * Until this is called, the body stays on the name seed.
	 * Returns false when the stored values are already those numbers.
	 */
	bool SetBodyControls(float NewHeight, float NewBuild, float NewGender);
	float GetBodyHeight() const { return BodyHeight; }
	float GetBodyBuild() const { return BodyBuild; }
	float GetBodyGender() const { return BodyGender; }

	/** Rebuild the worn body from the current name and controls. */
	void RebuildBody();

	/** Shown on the character-editor stage. Does not follow the captain or join the party. */
	void SetStagePreview(bool bPreview);
	bool IsStagePreview() const { return bStagePreview; }

	/** Match the follow target's MaxWalkSpeed when possible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	bool bMatchFollowTargetSpeed = true;

	/** Planar speed above which the run clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Anim", meta = (ClampMin = "0.0"))
	float RunAnimSpeedThreshold = 380.f;

	/** Planar speed above which the walk clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Anim", meta = (ClampMin = "0.0"))
	float WalkAnimSpeedThreshold = 30.f;

protected:
	virtual void PostInitializeComponents() override;

	void ApplyVisuals();
	void ApplyBodyMesh();
	void ApplyFace();
	void LayoutFace(uint32 Codepoint);
	void PaintFace(const FLinearColor& Ink);
	void ApplyNameLabel();
	void ResolveFollowTarget();
	void UpdateFollow(float DeltaSeconds);
	void CacheLocomotionAnims();
	void UpdateLocomotionAnim();
	bool PlayLocomotionClip(UAnimSequence* Anim);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> NameLabelRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> NameLabelBorder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> NameLabelBackground;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> NameLabel;

	/** Anchor on the front of the head. The engine font has no emoji, so the face is drawn here. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> FaceRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FaceLeftEye;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FaceRightEye;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FaceMouth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FaceMouthLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FaceMouthRight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FaceInk;

	/** Kept hidden. The engine font cannot draw U+1F611, so the drawn face is used instead. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> FaceText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Visual")
	TSoftObjectPtr<USkeletalMesh> CompanionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	FLinearColor TorsoColor = FLinearColor(0.29f, 0.38f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	FLinearColor LegColor = FLinearColor(0.12f, 0.11f, 0.10f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	FLinearColor SkinColor = FLinearColor(0.96f, 0.76f, 0.72f);

	/** Slider values. Ignored until SetBodyControls, so a plain spawn keeps the name seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	float BodyHeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	float BodyBuild = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	float BodyGender = 0.5f;

	/** SetBodyControls has stored Height, Build, and Gender. */
	bool bHasBodyControls = false;

	/** Character-editor mannequin. Tick keeps the idle clip and does not steer. */
	bool bStagePreview = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	float MeshGroundZOffset = -90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Anim")
	TSoftObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Anim")
	TSoftObjectPtr<UAnimSequence> WalkAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Anim")
	TSoftObjectPtr<UAnimSequence> RunAnim;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FollowTarget;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedIdleAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedWalkAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedRunAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveLocomotionAnim;
};
