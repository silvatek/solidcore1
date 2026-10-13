#include "SolidCompanionCharacter.h"
#include "Character/SolidSkeletonHover.h"
#include "SolidBodyMesh.h"
#include "SolidClipLocomotion.h"
#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "SolidGameMode.h"
#include "SolidMaterials.h"
#include "SolidNameLabel.h"
#include "Party/SolidBattlePlan.h"
#include "Party/SolidParty.h"
#include "GameFramework/GameModeBase.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "ReferenceSkeleton.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPath.h"

ASolidCompanionCharacter::ASolidCompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->JumpZVelocity = 700.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->BrakingDecelerationFalling = 1500.f;
	Move->bRunPhysicsWithNoController = true;

	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, MeshGroundZOffset), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GetMesh()->SetVisibility(true);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	FaceRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FaceRoot"));
	FaceRoot->SetupAttachment(GetMesh());
	FaceRoot->SetMobility(EComponentMobility::Movable);
	FaceRoot->SetVisibility(false);

	UStaticMesh* EyeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* MouthMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	FaceLeftEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FaceLeftEye"));
	FaceRightEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FaceRightEye"));
	FaceMouth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FaceMouth"));
	FaceMouthLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FaceMouthLeft"));
	FaceMouthRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FaceMouthRight"));
	UStaticMeshComponent* Pieces[] = {
		FaceLeftEye, FaceRightEye, FaceMouth, FaceMouthLeft, FaceMouthRight,
	};
	UStaticMesh* PieceMeshes[] = { EyeMesh, EyeMesh, MouthMesh, MouthMesh, MouthMesh };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Pieces); ++Index)
	{
		Pieces[Index]->SetupAttachment(FaceRoot);
		Pieces[Index]->SetMobility(EComponentMobility::Movable);
		Pieces[Index]->SetStaticMesh(PieceMeshes[Index]);
		Pieces[Index]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pieces[Index]->SetCastShadow(false);
		Pieces[Index]->SetVisibility(false);
	}

	FaceText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FaceText"));
	FaceText->SetupAttachment(FaceRoot);
	FaceText->SetVisibility(false);

	// Default mesh/clips are Fab Viking; Party members may override CompanionMesh later.
	CompanionMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath));
	IdleAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultIdlePath));
	WalkAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultWalkPath));
	RunAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultRunPath));

	NameLabelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("NameLabelRoot"));
	NameLabelRoot->SetupAttachment(RootComponent);
	NameLabelBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NameLabelBorder"));
	NameLabelBorder->SetupAttachment(NameLabelRoot);
	NameLabelBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NameLabelBackground"));
	NameLabelBackground->SetupAttachment(NameLabelRoot);
	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(NameLabelRoot);
	ApplyNameLabel();
}

void ASolidCompanionCharacter::SetSkeletonLines(const bool bShow)
{
	bSkeletonLines = bShow;
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetVisibility(!bShow, true);
	}
	if (bShow)
	{
		if (FaceRoot)
		{
			FaceRoot->SetVisibility(false, true);
		}
		return;
	}
	if (HasActorBegunPlay())
	{
		ApplyFace();
	}
}

void ASolidCompanionCharacter::SetStagePreview(const bool bPreview)
{
	bStagePreview = bPreview;
	if (bPreview)
	{
		SetActorEnableCollision(false);
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->DisableMovement();
			Move->GravityScale = 0.f;
			Move->StopMovementImmediately();
		}
	}
	ApplyNameLabel();
}

void ASolidCompanionCharacter::ApplyNameLabel()
{
	if (bStagePreview)
	{
		if (NameLabelRoot)
		{
			NameLabelRoot->SetVisibility(false, true);
		}
		return;
	}

	const float CapsuleHalf = GetCapsuleComponent()
		? GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()
		: 96.f;
	SolidNameLabel::Configure(
		NameLabelRoot,
		NameLabelBorder,
		NameLabelBackground,
		NameLabel,
		SolidVocation::NameLabel(CharacterDisplayName, Vocation),
		SolidNameLabel::EStyle::Companion,
		CapsuleHalf,
		this);
}

void ASolidCompanionCharacter::SetCharacterDisplayName(const FString& NewName)
{
	CharacterDisplayName = NewName;
	ApplyNameLabel();
	if (HasActorBegunPlay())
	{
		ApplyFace();
	}
}

bool ASolidCompanionCharacter::SetBodyColors(
	const FLinearColor& NewTorso,
	const FLinearColor& NewLegs,
	const FLinearColor& NewSkin)
{
	if (TorsoColor.Equals(NewTorso) && LegColor.Equals(NewLegs) && SkinColor.Equals(NewSkin))
	{
		return false;
	}
	TorsoColor = NewTorso;
	LegColor = NewLegs;
	SkinColor = NewSkin;
	if (HasActorBegunPlay())
	{
		ApplyVisuals();
	}
	return true;
}

bool ASolidCompanionCharacter::SetBodyControls(
	const float NewHeight,
	const float NewBuild,
	const float NewGender)
{
	const bool bChanged = !bHasBodyControls
		|| BodyHeight != NewHeight
		|| BodyBuild != NewBuild
		|| BodyGender != NewGender;
	BodyHeight = NewHeight;
	BodyBuild = NewBuild;
	BodyGender = NewGender;
	bHasBodyControls = true;
	if (!bChanged)
	{
		return false;
	}
	if (HasActorBegunPlay())
	{
		ApplyVisuals();
	}
	return true;
}

void ASolidCompanionCharacter::RebuildBody()
{
	if (HasActorBegunPlay())
	{
		ApplyVisuals();
	}
}

void ASolidCompanionCharacter::SetVocation(const ESolidVocation NewVocation)
{
	Vocation = NewVocation;
	ApplyNameLabel();
}

void ASolidCompanionCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyVisuals();
}

void ASolidCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisuals();
	ApplyNameLabel();
	if (bStagePreview)
	{
		SetStagePreview(true);
	}
	CacheLocomotionAnims();
	if (!bStagePreview)
	{
		ResolveFollowTarget();
	}
	UpdateLocomotionAnim();

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		FVector Rel = CharacterMesh->GetRelativeLocation();
		Rel.Z = MeshGroundZOffset;
		CharacterMesh->SetRelativeLocation(Rel);
	}

	UE_LOG(LogSolid, Warning,
		TEXT("Companion BeginPlay mesh=%s idle=%s walk=%s run=%s follow=%s"),
		GetMesh() && GetMesh()->GetSkeletalMeshAsset()
			? *GetMesh()->GetSkeletalMeshAsset()->GetName()
			: TEXT("<none>"),
		CachedIdleAnim ? *CachedIdleAnim->GetName() : TEXT("<null>"),
		CachedWalkAnim ? *CachedWalkAnim->GetName() : TEXT("<null>"),
		CachedRunAnim ? *CachedRunAnim->GetName() : TEXT("<null>"),
		FollowTarget.IsValid() ? *FollowTarget->GetName() : TEXT("<none>"));
}

void ASolidCompanionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bStagePreview)
	{
		UpdateLocomotionAnim();
		if (bSkeletonLines)
		{
			if (USkeletalMeshComponent* CharacterMesh = GetMesh())
			{
				CharacterMesh->SetVisibility(false, true);
			}
			DrawSkeletonLines();
		}
		return;
	}

	if (!FollowTarget.IsValid())
	{
		ResolveFollowTarget();
	}

	UpdateFollow(DeltaSeconds);
	UpdateLocomotionAnim();
	SolidNameLabel::FaceViewCamera(NameLabelRoot, GetWorld());
}

void ASolidCompanionCharacter::SetCompanionMesh(const TSoftObjectPtr<USkeletalMesh>& NewMesh)
{
	CompanionMesh = NewMesh;
	CachedIdleAnim = nullptr;
	CachedWalkAnim = nullptr;
	CachedRunAnim = nullptr;
	ActiveLocomotionAnim = nullptr;
	if (HasActorBegunPlay())
	{
		const bool bVikingMesh = CompanionMesh.IsNull()
			|| CompanionMesh.ToSoftObjectPath() == FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath);
		SolidClipLocomotion::ApplyMeshAndSingleNodeMode(
			GetMesh(),
			CompanionMesh,
			bVikingMesh ? SolidClipLocomotion::DefaultMeshPath : nullptr,
			TEXT("Companion"),
			/*bOnlyIfMeshUnset=*/false);
		ApplyBodyMesh();
		ApplyFace();
		CacheLocomotionAnims();
		UpdateLocomotionAnim();
	}
}

void ASolidCompanionCharacter::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;
}

void ASolidCompanionCharacter::ResolveFollowTarget()
{
	if (FollowTarget.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (Pawn != this)
				{
					FollowTarget = Pawn;
				}
			}
		}
	}
}

void ASolidCompanionCharacter::CacheLocomotionAnims()
{
	if (CachedIdleAnim || CachedWalkAnim || CachedRunAnim)
	{
		return;
	}

	if (const USkeletalMesh* MeshAsset = GetMesh() ? GetMesh()->GetSkeletalMeshAsset() : nullptr)
	{
		const FString Package = MeshAsset->GetOutermost()->GetName();
		if (SolidContentLookup::PackageContainsFolder(Package, SolidContentLookup::AssassinContentFolder))
		{
			UAnimSequence* Idle = SolidContentLookup::FindPackClip(
				SolidContentLookup::AssassinContentFolder, SolidContentLookup::AssassinIdleName);
			if (Idle)
			{
				CachedIdleAnim = Idle;
				CachedWalkAnim = SolidContentLookup::FindPackClip(
					SolidContentLookup::AssassinContentFolder, SolidContentLookup::AssassinWalkName);
				CachedRunAnim = SolidContentLookup::FindPackClip(
					SolidContentLookup::AssassinContentFolder, SolidContentLookup::AssassinRunName);
				return;
			}
		}
	}

	CachedIdleAnim = SolidClipLocomotion::LoadClip(
		IdleAnim, SolidClipLocomotion::DefaultIdlePath);
	CachedWalkAnim = SolidClipLocomotion::LoadClip(
		WalkAnim, SolidClipLocomotion::DefaultWalkPath);
	CachedRunAnim = SolidClipLocomotion::LoadClip(
		RunAnim, SolidClipLocomotion::DefaultRunPath);
}

void ASolidCompanionCharacter::ApplyVisuals()
{
	const bool bVikingMesh = CompanionMesh.IsNull()
		|| CompanionMesh.ToSoftObjectPath() == FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath);
	SolidClipLocomotion::ApplyMeshAndSingleNodeMode(
		GetMesh(),
		CompanionMesh,
		bVikingMesh ? SolidClipLocomotion::DefaultMeshPath : nullptr,
		TEXT("Companion"),
		/*bOnlyIfMeshUnset=*/true);
	ApplyBodyMesh();
	ApplyFace();
}

void ASolidCompanionCharacter::ApplyBodyMesh()
{
	const uint32 Seed = SolidBodyMesh::SeedForName(CharacterDisplayName);
	const FSolidBodyParams SeedParams = FSolidBodyParams::FromSeed(Seed);
	FSolidBodyControls Controls;
	if (bHasBodyControls)
	{
		Controls.Height = BodyHeight;
		Controls.Build = BodyBuild;
		Controls.Gender = BodyGender;
	}
	else
	{
		Controls = SolidBodyControls::FromSeed(SeedParams);
	}
	const FSolidBodyParams Params = SolidBodyControls::Resolve(SeedParams, Controls);
	SolidBodyMesh::ApplySeed(
		GetMesh(),
		Seed,
		Params,
		this,
		TEXT("Companion"),
		TorsoColor,
		LegColor,
		SkinColor);
}

void ASolidCompanionCharacter::PaintFace(const FLinearColor& Ink)
{
	UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(FaceInk.Get());
	if (!Existing || Existing->GetOuter() != this)
	{
		FaceInk = SolidMaterials::CreateSolidColor(this, Ink, TEXT("FaceInk"), 1.f);
	}
	else
	{
		Existing->SetVectorParameterValue(TEXT("Base Color"), Ink);
		Existing->SetVectorParameterValue(TEXT("BaseColor"), Ink);
		Existing->SetScalarParameterValue(TEXT("Roughness"), 1.f);
		Existing->RecacheUniformExpressions(true);
	}

	UStaticMeshComponent* Pieces[] = {
		FaceLeftEye, FaceRightEye, FaceMouth, FaceMouthLeft, FaceMouthRight,
	};
	for (UStaticMeshComponent* Piece : Pieces)
	{
		if (Piece && FaceInk)
		{
			Piece->SetMaterial(0, FaceInk);
			Piece->SetVisibility(true);
		}
	}
}

void ASolidCompanionCharacter::LayoutFace(const uint32 Codepoint)
{
	float EyeZ = 1.6f;
	float EyeScale = 0.045f;
	float MouthZ = -3.8f;
	float CornerLift = 0.f;
	const float EyeGap = 4.2f;

	switch (Codepoint)
	{
	case 0x1F611u:
		EyeScale = 0.028f;
		EyeZ = 1.2f;
		break;
	case 0x1F642u:
		CornerLift = 2.f;
		break;
	case 0x1F641u:
		CornerLift = -2.f;
		break;
	case 0x1F612u:
		EyeZ = 0.2f;
		EyeScale = 0.034f;
		CornerLift = -0.7f;
		break;
	case 0x1F644u:
		EyeZ = 3.4f;
		break;
	default:
		break;
	}

	if (FaceLeftEye)
	{
		FaceLeftEye->SetRelativeLocation(FVector(0.8f, -EyeGap, EyeZ));
		FaceLeftEye->SetRelativeScale3D(FVector(EyeScale));
	}
	if (FaceRightEye)
	{
		FaceRightEye->SetRelativeLocation(FVector(0.8f, EyeGap, EyeZ));
		FaceRightEye->SetRelativeScale3D(FVector(EyeScale));
	}
	if (FaceMouth)
	{
		FaceMouth->SetRelativeLocation(FVector(0.8f, 0.f, MouthZ));
		FaceMouth->SetRelativeScale3D(FVector(0.012f, 0.05f, 0.014f));
	}
	if (FaceMouthLeft)
	{
		FaceMouthLeft->SetRelativeLocation(FVector(0.8f, -3.6f, MouthZ + CornerLift));
		FaceMouthLeft->SetRelativeScale3D(FVector(0.012f, 0.026f, 0.014f));
	}
	if (FaceMouthRight)
	{
		FaceMouthRight->SetRelativeLocation(FVector(0.8f, 3.6f, MouthZ + CornerLift));
		FaceMouthRight->SetRelativeScale3D(FVector(0.012f, 0.026f, 0.014f));
	}
}

void ASolidCompanionCharacter::DrawSkeletonLines() const
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UWorld* World = GetWorld();
	if (!CharacterMesh || !World)
	{
		return;
	}

	const FColor BoneColor(255, 176, 64);
	const int32 BoneCount = CharacterMesh->GetNumBones();
	for (int32 Index = 0; Index < BoneCount; ++Index)
	{
		const FName BoneName = CharacterMesh->GetBoneName(Index);
		if (BoneName.IsNone())
		{
			continue;
		}
		const FString Name = BoneName.ToString();
		if (SolidSkeletonHover::IsOmittedBoneName(Name))
		{
			continue;
		}

		const FVector BoneLocation = CharacterMesh->GetBoneLocation(BoneName);
		FName ParentName = CharacterMesh->GetParentBone(BoneName);
		while (!ParentName.IsNone() && SolidSkeletonHover::IsOmittedBoneName(ParentName.ToString()))
		{
			ParentName = CharacterMesh->GetParentBone(ParentName);
		}
		if (!ParentName.IsNone())
		{
			DrawDebugLine(
				World,
				CharacterMesh->GetBoneLocation(ParentName),
				BoneLocation,
				BoneColor,
				false,
				0.f,
				0,
				1.75f);
		}
	}
}

FString ASolidCompanionCharacter::BoneUnderScreenPoint(const float X, const float Y) const
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	APlayerController* Player = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!CharacterMesh || !Player)
	{
		return FString();
	}

	if (!bSkeletonLines)
	{
		FVector Origin = FVector::ZeroVector;
		FVector Dir = FVector::ZeroVector;
		if (!Player->DeprojectScreenPositionToWorld(X, Y, Origin, Dir) || !Dir.Normalize())
		{
			return FString();
		}

		FString BestLabel;
		float BestT = TNumericLimits<float>::Max();
		const int32 BoneCount = CharacterMesh->GetNumBones();
		auto FindBone = [CharacterMesh, BoneCount](const TCHAR* Wanted) -> FName
		{
			for (int32 Index = 0; Index < BoneCount; ++Index)
			{
				const FName BoneName = CharacterMesh->GetBoneName(Index);
				if (BoneName.ToString().Equals(Wanted, ESearchCase::IgnoreCase))
				{
					return BoneName;
				}
			}
			return NAME_None;
		};
		auto Consider = [&](const SolidBodyMesh::FSolidFittedCone& Cone, const FString& Label)
		{
			if (!Cone.bValid || Label.IsEmpty())
			{
				return;
			}
			float RayT = 0.f;
			float SegU = 0.f;
			const float Dist = SolidSkeletonHover::RaySegmentDistance(
				Origin,
				Dir,
				Cone.Start,
				Cone.End,
				RayT,
				SegU);
			const float Radius = FMath::Lerp(Cone.RadiusStart, Cone.RadiusEnd, SegU);
			if (Dist > Radius + 0.15f || RayT >= BestT)
			{
				return;
			}
			BestT = RayT;
			BestLabel = Label;
		};

		const TCHAR* ThighBones[] = {
			SolidBodyMesh::LeftThighBone,
			SolidBodyMesh::RightThighBone,
		};
		for (const TCHAR* Wanted : ThighBones)
		{
			const FName TwistName = FindBone(Wanted);
			const FName ParentName = TwistName.IsNone() ? NAME_None : CharacterMesh->GetParentBone(TwistName);
			if (TwistName.IsNone() || ParentName.IsNone())
			{
				continue;
			}
			Consider(
				SolidBodyMesh::LeftThighCone(
					CharacterMesh->GetBoneLocation(ParentName),
					CharacterMesh->GetBoneLocation(TwistName)),
				TwistName.ToString());
		}

		const TCHAR* CalfBones[] = {
			SolidBodyMesh::LeftCalfBone,
			SolidBodyMesh::RightCalfBone,
		};
		const TCHAR* CalfTwists[] = {
			SolidBodyMesh::LeftCalfTwistBone,
			SolidBodyMesh::RightCalfTwistBone,
		};
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FName CalfName = FindBone(CalfBones[Side]);
			const FName TwistName = FindBone(CalfTwists[Side]);
			const FName ParentName = CalfName.IsNone() ? NAME_None : CharacterMesh->GetParentBone(CalfName);
			if (CalfName.IsNone() || TwistName.IsNone() || ParentName.IsNone())
			{
				continue;
			}
			TArray<FString> Names;
			Names.Add(CalfName.ToString());
			Names.Add(TwistName.ToString());
			Names.Sort();
			Consider(
				SolidBodyMesh::CalfCone(
					CharacterMesh->GetBoneLocation(ParentName),
					CharacterMesh->GetBoneLocation(CalfName),
					CharacterMesh->GetBoneLocation(TwistName)),
				FString::Join(Names, TEXT(", ")));
		}
		return BestLabel;
	}

	TArray<FVector2D> From;
	TArray<FVector2D> To;
	TArray<FString> Names;
	const int32 BoneCount = CharacterMesh->GetNumBones();
	From.Reserve(BoneCount);
	To.Reserve(BoneCount);
	Names.Reserve(BoneCount);

	for (int32 Index = 0; Index < BoneCount; ++Index)
	{
		const FName BoneName = CharacterMesh->GetBoneName(Index);
		if (BoneName.IsNone())
		{
			continue;
		}
		const FString Name = BoneName.ToString();
		if (SolidSkeletonHover::IsOmittedBoneName(Name))
		{
			continue;
		}

		FVector2D BoneScreen = FVector2D::ZeroVector;
		if (!Player->ProjectWorldLocationToScreen(CharacterMesh->GetBoneLocation(BoneName), BoneScreen))
		{
			continue;
		}

		FName ParentName = CharacterMesh->GetParentBone(BoneName);
		while (!ParentName.IsNone() && SolidSkeletonHover::IsOmittedBoneName(ParentName.ToString()))
		{
			ParentName = CharacterMesh->GetParentBone(ParentName);
		}

		FVector2D ParentScreen = BoneScreen;
		if (!ParentName.IsNone())
		{
			if (!Player->ProjectWorldLocationToScreen(CharacterMesh->GetBoneLocation(ParentName), ParentScreen))
			{
				ParentScreen = BoneScreen;
			}
		}

		From.Add(ParentScreen);
		To.Add(BoneScreen);
		Names.Add(Name);
	}

	return SolidSkeletonHover::BoneUnderPoint(
		From.GetData(),
		To.GetData(),
		Names.GetData(),
		Names.Num(),
		FVector2D(X, Y),
		SolidSkeletonHover::PickRadiusPx);
}

void ASolidCompanionCharacter::ApplyFace()
{
	UStaticMeshComponent* Pieces[] = {
		FaceLeftEye, FaceRightEye, FaceMouth, FaceMouthLeft, FaceMouthRight,
	};
	auto HideFace = [this, &Pieces]()
	{
		if (FaceRoot)
		{
			FaceRoot->SetVisibility(false, true);
		}
		if (FaceText)
		{
			FaceText->SetVisibility(false);
		}
		for (UStaticMeshComponent* Piece : Pieces)
		{
			if (Piece)
			{
				Piece->SetVisibility(false);
			}
		}
	};

	if (!FaceRoot)
	{
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	const USkeletalMesh* MeshAsset = CharacterMesh ? CharacterMesh->GetSkeletalMeshAsset() : nullptr;
	if (!MeshAsset || MeshAsset->GetRefSkeleton().GetRawBoneNum() == 0)
	{
		HideFace();
		return;
	}

	const FReferenceSkeleton& Ref = MeshAsset->GetRefSkeleton();
	const TArray<FMeshBoneInfo>& Infos = Ref.GetRawRefBoneInfo();
	const TArray<FTransform>& Pose = Ref.GetRawRefBonePose();
	const int32 Count = FMath::Min(Infos.Num(), Pose.Num());

	int32 HeadIndex = INDEX_NONE;
	int32 Fallback = INDEX_NONE;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (SolidBodyMesh::RoleForBoneName(Infos[Index].Name.ToString()) != ESolidBodyBoneRole::Head)
		{
			continue;
		}
		if (Fallback == INDEX_NONE)
		{
			Fallback = Index;
		}
		const int32 Parent = Infos[Index].ParentIndex;
		if (Infos.IsValidIndex(Parent)
			&& SolidBodyMesh::RoleForBoneName(Infos[Parent].Name.ToString()) == ESolidBodyBoneRole::Neck)
		{
			HeadIndex = Index;
			break;
		}
	}
	if (HeadIndex == INDEX_NONE)
	{
		HeadIndex = Fallback;
	}
	if (HeadIndex == INDEX_NONE)
	{
		HideFace();
		return;
	}

	TArray<int32> Chain;
	for (int32 Index = HeadIndex; Infos.IsValidIndex(Index) && Chain.Num() <= Count; Index = Infos[Index].ParentIndex)
	{
		Chain.Add(Index);
		if (Infos[Index].ParentIndex == Index)
		{
			break;
		}
	}

	FVector HeadPos = FVector::ZeroVector;
	FQuat HeadRot = FQuat::Identity;
	FVector ParentPos = FVector::ZeroVector;
	for (int32 Step = Chain.Num() - 1; Step >= 0; --Step)
	{
		const int32 Index = Chain[Step];
		if (Index == HeadIndex)
		{
			ParentPos = HeadPos;
		}
		const FTransform& Local = Pose[Index];
		HeadPos += HeadRot.RotateVector(Local.GetTranslation());
		HeadRot = HeadRot * Local.GetRotation();
	}

	FVector Up = HeadPos - ParentPos;
	if (!Up.Normalize())
	{
		Up = FVector::UpVector;
	}
	FVector Forward = FVector(0.f, 1.f, 0.f) - Up * FVector::DotProduct(FVector(0.f, 1.f, 0.f), Up);
	if (!Forward.Normalize())
	{
		Forward = FVector::XAxisVector;
	}

	// Sit on the flat front. The offset uses the largest head so the features stay outside the skull.
	const float UpOffset = 22.f * 0.42f;
	const float ForwardOffset = 30.f * 0.48f * 0.42f + 1.4f;
	const FVector Offset = Up * UpOffset + Forward * ForwardOffset;
	const FQuat FaceRot = FRotationMatrix::MakeFromXZ(
		HeadRot.UnrotateVector(Forward),
		HeadRot.UnrotateVector(Up)).ToQuat();

	FaceRoot->AttachToComponent(
		CharacterMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		Infos[HeadIndex].Name);
	FaceRoot->SetRelativeLocation(HeadRot.UnrotateVector(Offset));
	FaceRoot->SetRelativeRotation(FaceRot.Rotator());
	FaceRoot->SetVisibility(true, true);
	if (FaceText)
	{
		FaceText->SetVisibility(false);
	}

	const FLinearColor Ink = SkinColor.GetLuminance() > 0.35f
		? FLinearColor(0.02f, 0.02f, 0.025f)
		: FLinearColor(0.95f, 0.95f, 0.92f);
	PaintFace(Ink);
	LayoutFace(SolidBodyMesh::FaceCodepointForName(CharacterDisplayName));
}

bool ASolidCompanionCharacter::PlayLocomotionClip(UAnimSequence* Anim)
{
	return SolidClipLocomotion::PlayLoopingClip(
		GetMesh(), Anim, ActiveLocomotionAnim, TEXT("Companion"));
}

void ASolidCompanionCharacter::UpdateLocomotionAnim()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !CharacterMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	CacheLocomotionAnims();

	const float Speed = GetVelocity().Size2D();
	float PlayerSpeed = 0.f;
	if (const AActor* Target = FollowTarget.Get())
	{
		PlayerSpeed = Target->GetVelocity().Size2D();
	}

	// Prefer run when clearly running/sprinting (walk≈500, sprint≈900).
	const bool bPreferRun =
		Speed >= RunAnimSpeedThreshold
		|| PlayerSpeed >= RunAnimSpeedThreshold
		|| (GetCharacterMovement() && GetCharacterMovement()->MaxWalkSpeed >= CatchUpSpeed - 1.f);

	const SolidClipLocomotion::EClip Kind = SolidClipLocomotion::SelectClip(
		Speed,
		WalkAnimSpeedThreshold,
		bPreferRun,
		/*bInAir=*/false,
		/*bAllowJump=*/false);

	UAnimSequence* Desired = CachedIdleAnim;
	switch (Kind)
	{
	case SolidClipLocomotion::EClip::Run:
		if (CachedRunAnim)
		{
			Desired = CachedRunAnim;
		}
		else if (CachedWalkAnim && Speed >= WalkAnimSpeedThreshold)
		{
			Desired = CachedWalkAnim;
		}
		break;
	case SolidClipLocomotion::EClip::Walk:
		Desired = CachedWalkAnim ? CachedWalkAnim.Get() : Desired;
		break;
	default:
		break;
	}

	if (!Desired)
	{
		UE_LOG(LogSolid, Warning, TEXT("Companion: no locomotion anim loaded (still T-pose)."));
		return;
	}

	if (SolidClipLocomotion::IsPlayingClip(CharacterMesh, ActiveLocomotionAnim, Desired))
	{
		return;
	}

	PlayLocomotionClip(Desired);
}

void ASolidCompanionCharacter::UpdateFollow(const float DeltaSeconds)
{
	AActor* Target = FollowTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector TargetLoc = Target->GetActorLocation();
	const FVector TargetForward = Target->GetActorForwardVector();
	const FVector TargetRight = Target->GetActorRightVector();

	float AlongForward = -FollowDistance;
	float AlongRight = SideOffset;
	ESolidBattleFormation Formation = ESolidBattleFormation::Line;
	if (UWorld* World = GetWorld())
	{
		if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			if (const USolidParty* Party = GameMode->GetParty())
			{
				Formation = Party->GetActiveFormation();
				const int32 CompanionCount = FMath::Max(GameMode->GetCompanions().Num(), 1);
				const FVector2D Slot = SolidBattleFormationSlots::SlotOffset(
					Formation,
					PartySlotIndex,
					CompanionCount,
					Party->GetActiveSpacing());
				AlongForward = Slot.X;
				AlongRight = Slot.Y;
			}
		}
	}

	const FVector FollowPoint =
		TargetLoc
		+ TargetForward * AlongForward
		+ TargetRight * AlongRight;

	FVector ToFollow = FollowPoint - GetActorLocation();
	ToFollow.Z = 0.f;
	const float PlanarDist = ToFollow.Size();

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	float DesiredMaxSpeed = WalkSpeed;
	if (bMatchFollowTargetSpeed)
	{
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		{
			if (const UCharacterMovementComponent* TargetMove = TargetCharacter->GetCharacterMovement())
			{
				DesiredMaxSpeed = FMath::Max(WalkSpeed, TargetMove->MaxWalkSpeed);
			}
		}
	}
	if (PlanarDist >= CatchUpDistance)
	{
		DesiredMaxSpeed = FMath::Max(DesiredMaxSpeed, CatchUpSpeed);
	}

	const bool bFaceCaptain = SolidBattleFormationSlots::ShouldFaceCaptain(
		Formation, PlanarDist, AcceptanceRadius);
	Move->bOrientRotationToMovement = !bFaceCaptain;
	if (bFaceCaptain)
	{
		const float Yaw = SolidBattleFormationSlots::YawFacingPoint(
			FVector2D(GetActorLocation().X, GetActorLocation().Y),
			FVector2D(TargetLoc.X, TargetLoc.Y));
		const FRotator Desired(0.f, Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(
			GetActorRotation(), Desired, DeltaSeconds, SolidBattleFormationSlots::FaceTurnInterpSpeed));
	}

	if (PlanarDist <= AcceptanceRadius)
	{
		Move->MaxWalkSpeed = DesiredMaxSpeed;
		return;
	}

	Move->MaxWalkSpeed = DesiredMaxSpeed;
	AddMovementInput(ToFollow.GetSafeNormal(), 1.f);
}
