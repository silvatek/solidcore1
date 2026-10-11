#include "SolidNameLabel.h"
#include "SolidMaterials.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

namespace SolidNameLabelPrivate
{
	static void SetupPlate(
		UStaticMeshComponent* Plate,
		UStaticMesh* Cube,
		UMaterialInterface* Material,
		const FVector& RelativeLocation,
		const FVector& Scale)
	{
		if (!Plate || !Cube)
		{
			return;
		}

		Plate->SetStaticMesh(Cube);
		Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plate->SetCastShadow(false);
		Plate->SetRelativeLocation(RelativeLocation);
		Plate->SetRelativeRotation(FRotator::ZeroRotator);
		Plate->SetRelativeScale3D(Scale);
		Plate->SetVisibility(true);
		if (Material)
		{
			Plate->SetMaterial(0, Material);
		}
	}
}

void SolidNameLabel::Configure(
	USceneComponent* Root,
	UStaticMeshComponent* Border,
	UStaticMeshComponent* Background,
	UTextRenderComponent* Text,
	const FString& DisplayName,
	const EStyle Style,
	const float CapsuleHalfHeight,
	UObject* MaterialOuter)
{
	if (!Root || !Text)
	{
		return;
	}

	const float WorldSize = WorldSizeFor(Style);
	const float CharW = WorldSize * 0.52f;
	const float TextW = FMath::Max(CharW * FMath::Max(DisplayName.Len(), 1), WorldSize * 1.6f);
	const float TextH = WorldSize * 1.15f;
	const float PadX = WorldSize * 0.38f;
	const float PadZ = WorldSize * 0.28f;
	const float BorderThickness = WorldSize * 0.14f;
	const float InnerW = TextW + PadX * 2.f;
	const float InnerH = TextH + PadZ * 2.f;
	const float OuterW = InnerW + BorderThickness * 2.f;
	const float OuterH = InnerH + BorderThickness * 2.f;
	const float PlateCenterZ = WorldSize * 0.45f;
	const float Depth = 1.6f; // cm along local X (toward camera)

	Root->SetRelativeLocation(FVector(0.f, 0.f, CapsuleHalfHeight + HeightAboveCapsuleCm));
	Root->SetRelativeRotation(FRotator::ZeroRotator);

	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(WorldSize);
	Text->SetTextRenderColor(TextColorFor(Style));
	Text->SetText(FText::FromString(DisplayName));
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	Text->SetRelativeLocation(FVector(Depth * 1.2f, 0.f, PlateCenterZ));
	Text->SetRelativeRotation(FRotator::ZeroRotator);

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UObject* Outer = MaterialOuter ? MaterialOuter : Text;
	UMaterialInterface* BorderMat = SolidMaterials::CreateSolidColor(
		Outer, BorderColorFor(Style), TEXT("NameLabelBorder"), 0.85f);
	UMaterialInterface* BackgroundMat = SolidMaterials::CreateSolidColor(
		Outer, BackgroundColorFor(Style), TEXT("NameLabelBackground"), 0.95f);

	// Thin cubes: X = depth, Y = width, Z = height (front face toward +X / camera).
	SolidNameLabelPrivate::SetupPlate(
		Border,
		Cube,
		BorderMat,
		FVector(-Depth * 0.5f, 0.f, PlateCenterZ),
		FVector(Depth / 100.f, OuterW / 100.f, OuterH / 100.f));

	SolidNameLabelPrivate::SetupPlate(
		Background,
		Cube,
		BackgroundMat,
		FVector(Depth * 0.35f, 0.f, PlateCenterZ),
		FVector(Depth / 100.f, InnerW / 100.f, InnerH / 100.f));
}

void SolidNameLabel::FaceViewCamera(USceneComponent* LabelRoot, const UWorld* World)
{
	if (!LabelRoot || !World)
	{
		return;
	}

	const APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector LabelLoc = LabelRoot->GetComponentLocation();
	FVector ToCam = CamLoc - LabelLoc;
	if (ToCam.IsNearlyZero())
	{
		return;
	}

	// TextRender / plate front face +X; point X at the camera.
	LabelRoot->SetWorldRotation(ToCam.GetSafeNormal().Rotation());
}
