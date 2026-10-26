#include "SolidSmallTable.h"

#include "Components/StaticMeshComponent.h"

ASolidSmallTable::ASolidSmallTable()
{
	Top = MakePart(TEXT("Top"));
	static const TCHAR* LegNames[LegCount] = {
		TEXT("Leg0"), TEXT("Leg1"), TEXT("Leg2"), TEXT("Leg3"),
	};
	Legs.Reserve(LegCount);
	for (int32 Index = 0; Index < LegCount; ++Index)
	{
		Legs.Add(MakePart(LegNames[Index]));
	}
}

SolidStrongholdFurniture::EKind ASolidSmallTable::GetKind() const
{
	return SolidStrongholdFurniture::EKind::SmallTable;
}

float ASolidSmallTable::TopCm() const
{
	return 80.f;
}

float ASolidSmallTable::TopThicknessCm() const
{
	return 8.f;
}

float ASolidSmallTable::LegCm() const
{
	return 8.f;
}

float ASolidSmallTable::LegHeightCm() const
{
	return 62.f;
}

float ASolidSmallTable::LegInsetCm() const
{
	return 12.f;
}

float ASolidSmallTable::LegOffsetCm() const
{
	return TopCm() * 0.5f - LegInsetCm();
}

UStaticMeshComponent* ASolidSmallTable::GetLeg(const int32 Index) const
{
	return Legs.IsValidIndex(Index) ? Legs[Index].Get() : nullptr;
}

void ASolidSmallTable::BuildVisuals()
{
	const float Lift = FloorLiftCm();
	UStaticMesh* Cube = BasicCube();
	const float Offset = LegOffsetCm();
	const FVector2D Corners[LegCount] = {
		FVector2D(Offset, Offset),
		FVector2D(Offset, -Offset),
		FVector2D(-Offset, Offset),
		FVector2D(-Offset, -Offset),
	};
	for (int32 Index = 0; Index < LegCount; ++Index)
	{
		PlacePart(
			GetLeg(Index),
			Cube,
			FVector(Corners[Index].X, Corners[Index].Y, Lift + LegHeightCm() * 0.5f),
			FRotator::ZeroRotator,
			FVector(LegCm(), LegCm(), LegHeightCm()),
			FVector(100.f, 100.f, 100.f),
			BeigeColor(),
			TEXT("SmallTableLeg"));
	}
	PlacePart(
		Top,
		Cube,
		FVector(0.f, 0.f, Lift + LegHeightCm() + TopThicknessCm() * 0.5f),
		FRotator::ZeroRotator,
		FVector(TopCm(), TopCm(), TopThicknessCm()),
		FVector(100.f, 100.f, 100.f),
		BeigeColor(),
		TEXT("SmallTableTop"));
}
