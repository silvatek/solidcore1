#include "SolidNameLabel.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void SolidNameLabel::Configure(
	UTextRenderComponent* Label,
	const FString& DisplayName,
	const EStyle Style,
	const float CapsuleHalfHeight)
{
	if (!Label)
	{
		return;
	}

	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextBottom);
	Label->SetWorldSize(WorldSizeFor(Style));
	Label->SetTextRenderColor(ColorFor(Style));
	Label->SetText(FText::FromString(DisplayName));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetCastShadow(false);
	Label->SetRelativeLocation(FVector(0.f, 0.f, CapsuleHalfHeight + HeightAboveCapsuleCm));
	Label->SetRelativeRotation(FRotator::ZeroRotator);
}

void SolidNameLabel::FaceViewCamera(UTextRenderComponent* Label, const UWorld* World)
{
	if (!Label || !World)
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

	const FVector LabelLoc = Label->GetComponentLocation();
	FVector ToCam = CamLoc - LabelLoc;
	if (ToCam.IsNearlyZero())
	{
		return;
	}

	// TextRender draws facing +X; point X at the camera so the front is readable.
	Label->SetWorldRotation(ToCam.GetSafeNormal().Rotation());
}
