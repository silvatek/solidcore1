#include "SolidCharacterStage.h"
#include "Companion/SolidCompanionCharacter.h"
#include "SolidMaterials.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"

namespace
{
	void PrepareSurface(UStaticMeshComponent* Surface, UStaticMesh* Cube)
	{
		if (!Surface)
		{
			return;
		}
		Surface->SetStaticMesh(Cube);
		Surface->SetMobility(EComponentMobility::Movable);
		Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Surface->SetCastShadow(false);
	}
}

ASolidCharacterStage::ASolidCharacterStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(SceneRoot);
	PrepareSurface(Backdrop, Cube);
	Backdrop->SetUsingAbsoluteLocation(true);
	Backdrop->SetUsingAbsoluteRotation(true);
	Backdrop->SetUsingAbsoluteScale(true);

	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(SceneRoot);
	PrepareSurface(Floor, Cube);
	Floor->SetUsingAbsoluteLocation(true);
	Floor->SetUsingAbsoluteRotation(true);
	Floor->SetUsingAbsoluteScale(true);

	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(SceneRoot);
	FillLight->SetUsingAbsoluteLocation(true);
	FillLight->SetMobility(EComponentMobility::Movable);
	FillLight->SetCastShadows(true);
	FillLight->SetIntensity(12000.f);
	FillLight->SetAttenuationRadius(1600.f);
	FillLight->SetLightColor(FLinearColor::White);
}

void ASolidCharacterStage::BeginPlay()
{
	Super::BeginPlay();
	PaintRoom();
	Place();
}

void ASolidCharacterStage::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	(void)DeltaSeconds;
	Place();
}

void ASolidCharacterStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Preview)
	{
		Preview->Destroy();
		Preview = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ASolidCharacterStage::ShowCompanion(const FSolidCompanion& Companion)
{
	Shown = Companion;
	bHasCompanion = true;
	EnsurePreview();
	ApplyPreview();
	Place();
}

void ASolidCharacterStage::SetSkeletonLines(const bool bShow)
{
	bSkeletonLines = bShow;
	if (Preview)
	{
		Preview->SetSkeletonLines(bShow);
	}
}

FString ASolidCharacterStage::BoneUnderPointer(const float X, const float Y) const
{
	if (!bSkeletonLines || !Preview)
	{
		return FString();
	}
	return Preview->BoneUnderScreenPoint(X, Y);
}

void ASolidCharacterStage::PaintRoom()
{
	if (bRoomPainted)
	{
		return;
	}
	UMaterialInterface* Mat = SolidMaterials::CreateSolidColor(
		this,
		FLinearColor(0.62f, 0.62f, 0.64f),
		TEXT("CharacterStage"),
		1.f);
	if (!Mat)
	{
		return;
	}
	if (Backdrop)
	{
		Backdrop->SetMaterial(0, Mat);
	}
	if (Floor)
	{
		Floor->SetMaterial(0, Mat);
	}
	bRoomPainted = true;
}

void ASolidCharacterStage::EnsurePreview()
{
	if (Preview || !bHasCompanion)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FTransform SpawnTransform(GetActorRotation(), GetActorLocation());
	ASolidCompanionCharacter* Spawned = World->SpawnActorDeferred<ASolidCompanionCharacter>(
		ASolidCompanionCharacter::StaticClass(),
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Spawned)
	{
		return;
	}

	Spawned->SetStagePreview(true);
	Spawned->SetCharacterDisplayName(Shown.Name);
	Spawned->SetVocation(Shown.Vocation);
	Spawned->SetBodyControls(Shown.BodyHeight, Shown.BodyBuild, Shown.BodyGender);
	Spawned->SetBodyColors(Shown.TorsoColor, Shown.LegColor, Shown.SkinColor);
	Spawned->FinishSpawning(SpawnTransform);
	Preview = Spawned;
}

void ASolidCharacterStage::ApplyPreview()
{
	if (!Preview || !bHasCompanion)
	{
		return;
	}
	const bool bNewPerson = Preview->GetCharacterDisplayName() != Shown.Name;
	Preview->SetCharacterDisplayName(Shown.Name);
	Preview->SetVocation(Shown.Vocation);
	const bool bControlsChanged = Preview->SetBodyControls(Shown.BodyHeight, Shown.BodyBuild, Shown.BodyGender);
	const bool bColorsChanged = Preview->SetBodyColors(Shown.TorsoColor, Shown.LegColor, Shown.SkinColor);
	if (bNewPerson && !bControlsChanged && !bColorsChanged)
	{
		Preview->RebuildBody();
	}
	Preview->SetSkeletonLines(bSkeletonLines);
}

void ASolidCharacterStage::Place()
{
	UWorld* World = GetWorld();
	APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Player)
	{
		return;
	}

	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	Player->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector Forward = CamRot.Vector();
	const FVector Up = FVector::UpVector;

	if (Backdrop)
	{
		Backdrop->SetWorldLocation(CamLoc + Forward * 300.f);
		Backdrop->SetWorldRotation(FRotationMatrix::MakeFromX(Forward).Rotator());
		Backdrop->SetWorldScale3D(FVector(0.12f, 80.f, 80.f));
	}
	if (FillLight)
	{
		FillLight->SetWorldLocation(CamLoc + Forward * 40.f + Up * 30.f);
	}

	const FVector PreviewLoc = CamLoc + Forward * 220.f - Up * 12.f;
	if (Floor)
	{
		const float FeetZ = PreviewLoc.Z - 96.f;
		Floor->SetWorldLocation(FVector(PreviewLoc.X, PreviewLoc.Y, FeetZ - 12.f));
		Floor->SetWorldRotation(FRotator::ZeroRotator);
		Floor->SetWorldScale3D(FVector(80.f, 80.f, 0.2f));
	}
	if (Preview)
	{
		Preview->SetActorLocation(PreviewLoc);
		Preview->SetActorRotation(FRotator(0.f, CamRot.Yaw + 180.f, 0.f));
	}
}
