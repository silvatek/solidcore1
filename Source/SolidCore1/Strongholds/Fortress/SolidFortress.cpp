#include "SolidFortress.h"
#include "SolidGameState.h"
#include "Strongholds/Fortress/Actors/SolidFortressExterior.h"
#include "Strongholds/Fortress/Actors/SolidFortressInterior.h"

namespace SolidStronghold
{
	namespace SolidFortressPrivate
	{
		FVector2D RoomOriginCm(const FVector2D& InnerMin)
		{
			return FVector2D(
				InnerMin.X + SolidStrongholdGrid::FlatToFlatCm * 0.5f,
				InnerMin.Y + SolidStrongholdGrid::SideCm());
		}

		bool VerticesInside(const FVector2D& Center, const FVector2D& InnerMin, const FVector2D& InnerSize)
		{
			FVector2D Vertices[6];
			SolidStrongholdGrid::HexVertices(Center, Vertices);
			constexpr float Slack = 0.05f;
			const float MaxX = InnerMin.X + InnerSize.X;
			const float MaxY = InnerMin.Y + InnerSize.Y;
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const FVector2D& Point = Vertices[Index];
				if (Point.X < InnerMin.X - Slack
					|| Point.X > MaxX + Slack
					|| Point.Y < InnerMin.Y - Slack
					|| Point.Y > MaxY + Slack)
				{
					return false;
				}
			}
			return true;
		}
	}

	float FSolidFortressBuilding::WidthCm() const
	{
		return (static_cast<float>(Columns) + 0.5f) * SolidStrongholdGrid::FlatToFlatCm;
	}

	float FSolidFortressBuilding::DepthCm() const
	{
		const float Side = SolidStrongholdGrid::SideCm();
		return static_cast<float>(Rows - 1) * SolidStrongholdGrid::RowStepCm() + 2.f * Side;
	}

	void FSolidFortressBuilding::CollectHexes(TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		const FVector2D InnerSize(WidthCm(), DepthCm());
		const FVector2D Origin = SolidFortressPrivate::RoomOriginCm(InnerMinCm);
		for (int32 Row = -2; Row <= Rows + 2; ++Row)
		{
			for (int32 Col = -2; Col <= Columns + 2; ++Col)
			{
				const FVector2D Center = SolidStrongholdGrid::HexCenter(Col, Row, Origin);
				if (!SolidFortressPrivate::VerticesInside(Center, InnerMinCm, InnerSize))
				{
					continue;
				}
				SolidStrongholdGrid::FHex Hex;
				Hex.Center = Center;
				Hex.Col = Col;
				Hex.Row = Row;
				OutHexes.Add(Hex);
			}
		}
	}

	FSolidFortress::FSolidFortress()
	{
		Type = ESolidStrongholdType::Fortress;
		ParapetCm = SolidStrongholdGrid::FlatToFlatCm;
		YardDepthCm = FSolidStronghold::StoreyCm;

		FSolidFortressBuilding Room;
		Room.Columns = 2;
		Room.Rows = 3;
		FSolidFortressBuilding Keep;
		Keep.bKeep = true;
		Keep.Columns = 4;
		Keep.Rows = 3;
		Buildings.Add(Room);
		Buildings.Add(Keep);

		float X = FSolidStronghold::WallCm;
		for (int32 Index = 0; Index < Buildings.Num(); ++Index)
		{
			FSolidFortressBuilding& One = Buildings[Index];
			One.InnerMinCm = FVector2D(X, FSolidStronghold::WallCm);
			X += One.WidthCm() + FSolidStronghold::WallCm;
			if (Index + 1 < Buildings.Num())
			{
				X += GapCm + FSolidStronghold::WallCm;
			}
		}
	}

	const FSolidFortress& FSolidFortress::Instance()
	{
		static const FSolidFortress Fortress;
		return Fortress;
	}

	bool FSolidFortress::Supported(const ESolidStrongholdSize InSize) const
	{
		return InSize == ESolidStrongholdSize::Humble;
	}

	ESolidStrongholdTrip FSolidFortress::Trip() const
	{
		return ESolidStrongholdTrip::None;
	}

	bool FSolidFortress::AcceptsEnter(
		const USolidGameState& State,
		const FSolidStronghold* Active,
		const FIntPoint OpeningCell) const
	{
		return Supported(Size)
			&& State.bStrongholdsEnabled
			&& Active
			&& Active->Type == ESolidStrongholdType::Fortress
			&& Active->Cell == OpeningCell;
	}

	float FSolidFortress::ExitReachCm() const
	{
		if (!Supported(Size))
		{
			return FSolidStronghold::ExitReachCm();
		}
		return OuterDepthCm() * 0.5f + SolidStrongholdGrid::FlatToFlatCm;
	}

	void FSolidFortress::RememberExit(const USolidWorldChart* Map)
	{
		SolidStrongholds::FGrassFace Face;
		const float GrassYaw = (Map && SolidStrongholds::FindGrassFace(Map, Cell, Face))
			? Face.FacingYawDeg
			: 0.f;
		Orientation = OrientationFromYaw(GrassYaw);
	}

	UClass* FSolidFortress::ExteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidFortressExterior::StaticClass();
	}

	void FSolidFortress::ConfigureExterior(AActor* Exterior) const
	{
		ASolidFortressExterior* Shell = Cast<ASolidFortressExterior>(Exterior);
		if (!Shell || !Supported(Size))
		{
			return;
		}
		Shell->SetFortress(*this);
		Shell->SetStrongholdCell(Cell);
		Shell->BuildVisuals();
	}

	UClass* FSolidFortress::InteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidFortressInterior::StaticClass();
	}

	void FSolidFortress::ConfigureInterior(AActor* Interior) const
	{
		ASolidFortressInterior* Room = Cast<ASolidFortressInterior>(Interior);
		if (!Room || !Supported(Size))
		{
			return;
		}
		Room->SetFortress(*this);
		Room->SetStrongholdCell(Cell);
		Room->SetFurnishings(Furnishings);
		Room->BuildVisuals();
	}

	FSolidNameSignPlace FSolidFortress::NameSign() const
	{
		FSolidNameSignPlace Place;
		Place.FrameEdgeCm = FVector(-OuterDepthCm() * 0.5f, OpeningCm * 0.5f, 0.f);
		Place.Outward = FVector2D(0.f, 1.f);
		Place.YawDeg = 180.f;
		return Place;
	}

	float FSolidFortress::ExteriorYawDeg(const USolidWorldChart* Map) const
	{
		SolidStrongholds::FGrassFace Face;
		const float GrassYaw = (Map && SolidStrongholds::FindGrassFace(Map, Cell, Face))
			? Face.FacingYawDeg
			: 0.f;
		return FMath::UnwindDegrees(GrassYaw + 180.f);
	}

	const FSolidFortressBuilding* FSolidFortress::Keep() const
	{
		for (const FSolidFortressBuilding& One : Buildings)
		{
			if (One.bKeep)
			{
				return &One;
			}
		}
		return nullptr;
	}

	float FSolidFortress::OuterWidthCm() const
	{
		float Width = FSolidStronghold::WallCm;
		for (int32 Index = 0; Index < Buildings.Num(); ++Index)
		{
			Width += Buildings[Index].WidthCm() + FSolidStronghold::WallCm;
			if (Index + 1 < Buildings.Num())
			{
				Width += GapCm + FSolidStronghold::WallCm;
			}
		}
		return Width;
	}

	float FSolidFortress::OuterDepthCm() const
	{
		float Depth = 0.f;
		for (const FSolidFortressBuilding& One : Buildings)
		{
			Depth = FMath::Max(Depth, One.DepthCm());
		}
		return FSolidStronghold::WallCm + Depth + FSolidStronghold::WallCm + YardDepthCm + FSolidStronghold::WallCm;
	}

	float FSolidFortress::CurtainHeightCm() const
	{
		return FSolidStronghold::StoreyCm + ParapetCm;
	}

	FVector2D FSolidFortress::OutsideGateXY(const USolidWorldChart* Map) const
	{
		const float Reach = OuterDepthCm() * 0.5f + SolidStrongholdGrid::FlatToFlatCm;
		const FVector World = FRotator(0.f, ExteriorYawDeg(Map), 0.f).RotateVector(FVector(-Reach, 0.f, 0.f));
		return Location + FVector2D(World.X, World.Y);
	}

	void FSolidFortress::CollectGroundHexes(
		const ESolidStrongholdSize InSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Supported(InSize))
		{
			return;
		}
		int32 Shift = 0;
		for (const FSolidFortressBuilding& One : Buildings)
		{
			TArray<SolidStrongholdGrid::FHex> Room;
			One.CollectHexes(Room);
			for (SolidStrongholdGrid::FHex& Hex : Room)
			{
				Hex.Col += Shift;
			}
			OutHexes.Append(Room);
			Shift += 100;
		}
	}

	TUniquePtr<FSolidStronghold> FSolidFortress::Clone() const
	{
		return MakeUnique<FSolidFortress>(*this);
	}
}
