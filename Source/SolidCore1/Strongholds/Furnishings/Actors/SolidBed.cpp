#include "SolidBed.h"

#include "Components/StaticMeshComponent.h"

ASolidBed::ASolidBed()
{
	BedMesh = MakePart(TEXT("Bed"));
	Pillow = MakePart(TEXT("Pillow"));
}

int32 ASolidBed::HexCount() const
{
	return 2;
}

float ASolidBed::LengthCm() const
{
	return SolidStrongholdGrid::FlatToFlatCm * 2.f - 20.f;
}

float ASolidBed::WidthCm() const
{
	return SolidStrongholdGrid::SideCm() - SolidStrongholdGrid::LineThicknessCm * 1.5f;
}

float ASolidBed::HeightCm() const
{
	return 42.f;
}

float ASolidBed::PillowLengthCm() const
{
	return 52.f;
}

float ASolidBed::PillowWidthCm() const
{
	return WidthCm() * 0.70f;
}

float ASolidBed::PillowHeightCm() const
{
	return 14.f;
}

FLinearColor ASolidBed::PillowColor() const
{
	return FLinearColor(0.94f, 0.93f, 0.90f);
}

void ASolidBed::BuildVisuals()
{
	const float Lift = FloorLiftCm();
	UStaticMesh* Cube = BasicCube();
	PlacePart(
		BedMesh,
		Cube,
		FVector(0.f, 0.f, Lift + HeightCm() * 0.5f),
		FRotator::ZeroRotator,
		FVector(LengthCm(), WidthCm(), HeightCm()),
		FVector(100.f, 100.f, 100.f),
		BeigeColor(),
		TEXT("BedBody"));
	PlacePart(
		Pillow,
		Cube,
		FVector(LengthCm() * 0.5f - PillowLengthCm() * 0.5f - 8.f, 0.f, Lift + HeightCm() + PillowHeightCm() * 0.5f),
		FRotator::ZeroRotator,
		FVector(PillowLengthCm(), PillowWidthCm(), PillowHeightCm()),
		FVector(100.f, 100.f, 100.f),
		PillowColor(),
		TEXT("BedPillow"));
}
