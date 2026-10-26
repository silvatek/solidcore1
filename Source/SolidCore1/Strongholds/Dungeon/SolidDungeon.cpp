#include "SolidDungeon.h"
#include "SolidDungeonExterior.h"
#include "SolidDungeonInterior.h"
#include "SolidDungeonLevel.h"
#include "SolidGameState.h"
#include "Strongholds/SolidStronghold.h"

namespace SolidStronghold
{
	const FSolidDungeon& FSolidDungeon::Instance()
	{
		static const FSolidDungeon Dungeon;
		return Dungeon;
	}

	bool FSolidDungeon::Supported(const ESolidStrongholdSize InSize) const
	{
		return InSize == ESolidStrongholdSize::Meagre;
	}

	ESolidStrongholdTrip FSolidDungeon::Trip() const
	{
		return ESolidStrongholdTrip::Cave;
	}

	bool FSolidDungeon::AcceptsEnter(
		const USolidGameState& State,
		const FSolidStronghold* Active,
		const FIntPoint OpeningCell) const
	{
		return State.bStrongholdsEnabled
			&& Active
			&& Active->Type == ESolidStrongholdType::Dungeon
			&& Active->Cell == OpeningCell;
	}

	UClass* FSolidDungeon::ExteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidDungeonExterior::StaticClass();
	}

	void FSolidDungeon::ConfigureExterior(AActor* Exterior) const
	{
		ASolidDungeonExterior* Rock = Cast<ASolidDungeonExterior>(Exterior);
		if (!Rock)
		{
			return;
		}
		Rock->SetStrongholdCell(Cell);
		Rock->SetFace(
			FaceWidthCm(),
			FaceHeightCm(),
			SlopeRunCm(),
			FaceForwardCm(Orientation),
			ShelfLateralCm(Orientation));
		Rock->BuildVisuals();
	}

	UClass* FSolidDungeon::InteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidDungeonInterior::StaticClass();
	}

	void FSolidDungeon::ConfigureInterior(
		AActor* Interior,
		const TArray<SolidStrongholdFurniture::FUsed>& Furnishings) const
	{
		ASolidDungeonInterior* Cave = Cast<ASolidDungeonInterior>(Interior);
		if (!Cave)
		{
			return;
		}
		Cave->SetStrongholdCell(Cell);
		Cave->SetFurnishings(Furnishings);
		Cave->BuildVisuals();
	}

	float FSolidDungeon::FaceWidthCm() const
	{
		return SolidStrongholds::OpeningWidthCm + 2.f * SolidStrongholds::FrameThicknessCm;
	}

	float FSolidDungeon::FaceHeightCm() const
	{
		return SolidStrongholds::OpeningHeightCm + SolidStrongholds::FrameThicknessCm;
	}

	float FSolidDungeon::SlopeRunCm() const
	{
		return 2.f * FaceHeightCm();
	}

	float FSolidDungeon::FaceForwardCm(const ESolidStrongholdOrientation InOrientation) const
	{
		const float HalfCell = CellExtentAlongExitCm(InOrientation) * 0.5f;
		return FMath::Max(0.f, HalfCell - SolidStrongholds::MouthBorderGapCm);
	}

	float FSolidDungeon::ShelfLateralCm(const ESolidStrongholdOrientation InOrientation) const
	{
		constexpr float PointSpacing = 200.f;
		const float HalfStep = PointSpacing * 0.5f;
		switch (InOrientation)
		{
		case ESolidStrongholdOrientation::North:
		case ESolidStrongholdOrientation::West:
			return -HalfStep;
		case ESolidStrongholdOrientation::South:
		case ESolidStrongholdOrientation::East:
		default:
			return HalfStep;
		}
	}

	float FSolidDungeon::LeaveStandCm() const
	{
		return 250.f;
	}

	FVector2D FSolidDungeon::LeaveStandXY(
		const FVector2D CellCenterXY,
		const ESolidStrongholdOrientation InOrientation) const
	{
		const float YawRad = FMath::DegreesToRadians(SolidStronghold::OrientationYawDeg(InOrientation));
		const float Ahead = FaceForwardCm(InOrientation) + LeaveStandCm();
		const FVector2D Forward(FMath::Cos(YawRad), FMath::Sin(YawRad));
		const FVector2D Left(-Forward.Y, Forward.X);
		return CellCenterXY + Forward * Ahead + Left * ShelfLateralCm(InOrientation);
	}

	float FSolidDungeon::ExitReachCm() const
	{
		return FaceForwardCm(Orientation) + LeaveStandCm();
	}

	float FSolidDungeon::ExitLateralCm() const
	{
		return ShelfLateralCm(Orientation);
	}

	void FSolidDungeon::CollectGroundHexes(
		const ESolidStrongholdSize InSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Supported(InSize))
		{
			return;
		}
		SolidDungeonLevel::CollectMeagreHexes(OutHexes);
	}

	TUniquePtr<FSolidStronghold> FSolidDungeon::Clone() const
	{
		return MakeUnique<FSolidDungeon>(*this);
	}

	FSolidNameSignPlace FSolidDungeon::NameSign() const
	{
		FSolidNameSignPlace Place;
		Place.FrameEdgeCm = FVector(
			FaceForwardCm(Orientation),
			ShelfLateralCm(Orientation) + FaceWidthCm() * 0.5f,
			0.f);
		Place.Outward = FVector2D(0.f, 1.f);
		Place.YawDeg = 0.f;
		return Place;
	}
}
