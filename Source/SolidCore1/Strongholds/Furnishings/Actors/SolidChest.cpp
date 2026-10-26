#include "SolidChest.h"

#include "Components/StaticMeshComponent.h"

ASolidChest::ASolidChest()
{
	ChestMesh = MakePart(TEXT("Chest"));
	Lid = MakePart(TEXT("Lid"));
}

SolidStrongholdFurniture::EKind ASolidChest::GetKind() const
{
	return SolidStrongholdFurniture::EKind::Chest;
}

float ASolidChest::LengthCm() const
{
	return 76.f;
}

float ASolidChest::WidthCm() const
{
	return 48.f;
}

float ASolidChest::BodyHeightCm() const
{
	return 36.f;
}

float ASolidChest::LidRadiusCm() const
{
	return WidthCm() * 0.5f;
}

void ASolidChest::BuildVisuals()
{
	const float Lift = FloorLiftCm();
	PlacePart(
		ChestMesh,
		BasicCube(),
		FVector(0.f, 0.f, Lift + BodyHeightCm() * 0.5f),
		FRotator::ZeroRotator,
		FVector(LengthCm(), WidthCm(), BodyHeightCm()),
		FVector(100.f, 100.f, 100.f),
		BeigeColor(),
		TEXT("ChestBody"));
	// Engine cylinder axis is local Z. Turn that axis along the chest so the dome is the lid.
	const FRotator LidRotation = FRotationMatrix::MakeFromZ(FVector::ForwardVector).Rotator();
	PlacePart(
		Lid,
		BasicCylinder(),
		FVector(0.f, 0.f, Lift + BodyHeightCm()),
		LidRotation,
		FVector(LidRadiusCm() * 2.f, LidRadiusCm() * 2.f, LengthCm()),
		FVector(100.f, 100.f, 100.f),
		BeigeColor(),
		TEXT("ChestLid"));
}
