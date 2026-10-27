#include "SolidFortress.h"

#include "Strongholds/Tower/SolidTower.h"

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
		const FPlan Tower;
		WallCm = Tower.WallCm;
		StoreyCm = Tower.FloorPitchCm;
		OpeningCm = Tower.DoorWidthCm;
		ParapetCm = SolidStrongholdGrid::FlatToFlatCm;
		YardDepthCm = StoreyCm;

		FSolidFortressBuilding Annex;
		Annex.Kind = ESolidFortressBuilding::Annex;
		Annex.Columns = 2;
		Annex.Rows = 3;
		FSolidFortressBuilding Keep;
		Keep.Kind = ESolidFortressBuilding::Keep;
		Keep.Columns = 4;
		Keep.Rows = 3;
		Buildings.Add(Annex);
		Buildings.Add(Keep);

		float X = WallCm;
		for (int32 Index = 0; Index < Buildings.Num(); ++Index)
		{
			FSolidFortressBuilding& One = Buildings[Index];
			One.InnerMinCm = FVector2D(X, WallCm);
			X += One.WidthCm() + WallCm;
			if (Index + 1 < Buildings.Num())
			{
				X += GapCm + WallCm;
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

	const FSolidFortressBuilding* FSolidFortress::Building(const ESolidFortressBuilding Kind) const
	{
		for (const FSolidFortressBuilding& One : Buildings)
		{
			if (One.Kind == Kind)
			{
				return &One;
			}
		}
		return nullptr;
	}

	float FSolidFortress::OuterWidthCm() const
	{
		float Width = WallCm;
		for (int32 Index = 0; Index < Buildings.Num(); ++Index)
		{
			Width += Buildings[Index].WidthCm() + WallCm;
			if (Index + 1 < Buildings.Num())
			{
				Width += GapCm + WallCm;
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
		return WallCm + Depth + WallCm + YardDepthCm + WallCm;
	}

	float FSolidFortress::CurtainHeightCm() const
	{
		return StoreyCm + ParapetCm;
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
		for (const FSolidFortressBuilding& One : Buildings)
		{
			TArray<SolidStrongholdGrid::FHex> Room;
			One.CollectHexes(Room);
			OutHexes.Append(Room);
		}
	}

	TUniquePtr<FSolidStronghold> FSolidFortress::Clone() const
	{
		return MakeUnique<FSolidFortress>(*this);
	}
}
