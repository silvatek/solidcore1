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

		void CollectRoom(
			const FVector2D& InnerMin,
			const FVector2D& InnerSize,
			const int32 Columns,
			const int32 Rows,
			TArray<SolidStrongholdGrid::FHex>& OutHexes)
		{
			OutHexes.Reset();
			const FVector2D Origin = RoomOriginCm(InnerMin);
			for (int32 Row = -2; Row <= Rows + 2; ++Row)
			{
				for (int32 Col = -2; Col <= Columns + 2; ++Col)
				{
					const FVector2D Center = SolidStrongholdGrid::HexCenter(Col, Row, Origin);
					if (!VerticesInside(Center, InnerMin, InnerSize))
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
	}

	const FSolidFortress& FSolidFortress::Instance()
	{
		static const FSolidFortress Fortress;
		return Fortress;
	}

	bool FSolidFortress::Supported(const ESolidStrongholdSize Size) const
	{
		return Size == ESolidStrongholdSize::Humble;
	}

	ESolidStrongholdTrip FSolidFortress::Trip() const
	{
		return ESolidStrongholdTrip::None;
	}

	float FSolidFortress::RoomWidthCm(const int32 Columns) const
	{
		return (static_cast<float>(Columns) + 0.5f) * SolidStrongholdGrid::FlatToFlatCm;
	}

	float FSolidFortress::RoomDepthCm() const
	{
		const float Side = SolidStrongholdGrid::SideCm();
		return static_cast<float>(RoomRows - 1) * SolidStrongholdGrid::RowStepCm() + 2.f * Side;
	}

	FVector2D FSolidFortress::AnnexInnerMinCm() const
	{
		return FVector2D(WallCm, WallCm);
	}

	FVector2D FSolidFortress::KeepInnerMinCm() const
	{
		const float AnnexOuterEast = AnnexInnerMinCm().X + RoomWidthCm(AnnexColumns) + WallCm;
		return FVector2D(AnnexOuterEast + GapCm + WallCm, WallCm);
	}

	float FSolidFortress::OuterWidthCm() const
	{
		return KeepInnerMinCm().X + RoomWidthCm(KeepColumns) + WallCm;
	}

	float FSolidFortress::OuterDepthCm() const
	{
		return WallCm + RoomDepthCm() + WallCm + YardDepthCm + WallCm;
	}

	float FSolidFortress::CurtainHeightCm() const
	{
		return StoreyCm + ParapetCm;
	}

	void FSolidFortress::CollectKeepHexes(
		const ESolidStrongholdSize Size,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Supported(Size))
		{
			return;
		}
		SolidFortressPrivate::CollectRoom(
			KeepInnerMinCm(),
			FVector2D(RoomWidthCm(KeepColumns), RoomDepthCm()),
			KeepColumns,
			RoomRows,
			OutHexes);
	}

	void FSolidFortress::CollectAnnexHexes(
		const ESolidStrongholdSize Size,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Supported(Size))
		{
			return;
		}
		SolidFortressPrivate::CollectRoom(
			AnnexInnerMinCm(),
			FVector2D(RoomWidthCm(AnnexColumns), RoomDepthCm()),
			AnnexColumns,
			RoomRows,
			OutHexes);
	}

	void FSolidFortress::CollectGroundHexes(
		const ESolidStrongholdSize Size,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		TArray<SolidStrongholdGrid::FHex> Keep;
		TArray<SolidStrongholdGrid::FHex> Annex;
		CollectKeepHexes(Size, Keep);
		CollectAnnexHexes(Size, Annex);
		OutHexes.Append(Keep);
		OutHexes.Append(Annex);
	}

	TUniquePtr<FSolidStronghold> FSolidFortress::Clone() const
	{
		return MakeUnique<FSolidFortress>(*this);
	}
}
