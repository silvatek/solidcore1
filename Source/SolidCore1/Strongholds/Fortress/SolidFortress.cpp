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

		/**
		 * One building in a size's list.
		 * X and Y are the inner south-west corner, in centimetres. Facing is the doorway.
		 * Inner width and depth stay 0 when that face is the hex grid.
		 */
		struct FRoomPlan
		{
			ESolidStrongholdRoom Size = ESolidStrongholdRoom::Small;
			ESolidStrongholdOrientation Facing = ESolidStrongholdOrientation::North;
			float X = 0.f;
			float Y = 0.f;
			int32 Columns = 0;
			int32 Rows = 0;
			bool bKeep = false;
			float InnerWidthCm = 0.f;
			float InnerDepthCm = 0.f;
		};

		using enum ESolidStrongholdRoom;
		using enum ESolidStrongholdOrientation;

		// Size, doorway, X, Y, columns, rows. Keep and a wider face are written when they apply.
		static const FRoomPlan HumbleRooms[] =
		{
			{ .Size = Small,  .Facing = North, .X = 50.f,  .Y = 50.f, .Columns = 2, .Rows = 3 },
			{ .Size = Medium, .Facing = North, .X = 450.f, .Y = 50.f, .Columns = 4, .Rows = 3, .bKeep = true },
		};

		static const FRoomPlan AverageRooms[] =
		{
			{ .Size = Small,  .Facing = East,  .X = 50.f,  .Y = 496.4f, .Columns = 2, .Rows = 3 },
			{ .Size = Medium, .Facing = North, .X = 50.f,  .Y = 50.f,   .Columns = 4, .Rows = 3, .bKeep = true, .InnerWidthCm = 640.f },
			{ .Size = Small,  .Facing = North, .X = 790.f, .Y = 50.f,   .Columns = 2, .Rows = 3, .InnerWidthCm = 402.8f },
		};

		FSolidFortressBuilding Building(const FRoomPlan& Plan)
		{
			FSolidFortressBuilding Room;
			Room.Room = Plan.Size;
			Room.Facing = Plan.Facing;
			Room.bKeep = Plan.bKeep;
			Room.Columns = Plan.Columns;
			Room.Rows = Plan.Rows;
			Room.InnerMinCm = FVector2D(Plan.X, Plan.Y);
			Room.InnerWidthCm = Plan.InnerWidthCm;
			Room.InnerDepthCm = Plan.InnerDepthCm;
			return Room;
		}

		void AddRooms(FSolidFortress& Fortress, const FRoomPlan* Rooms, const int32 Count)
		{
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Fortress.Buildings.Add(Building(Rooms[Index]));
			}
		}
	}

	float FSolidFortressBuilding::SpanWidthCm() const
	{
		return (static_cast<float>(Columns) + 0.5f) * SolidStrongholdGrid::FlatToFlatCm;
	}

	float FSolidFortressBuilding::SpanDepthCm() const
	{
		const float Side = SolidStrongholdGrid::SideCm();
		return static_cast<float>(Rows - 1) * SolidStrongholdGrid::RowStepCm() + 2.f * Side;
	}

	float FSolidFortressBuilding::WidthCm() const
	{
		return FMath::Max(InnerWidthCm, SpanWidthCm());
	}

	float FSolidFortressBuilding::DepthCm() const
	{
		return FMath::Max(InnerDepthCm, SpanDepthCm());
	}

	void FSolidFortressBuilding::CollectHexes(TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		const float SpanWidth = SpanWidthCm();
		const float SpanDepth = SpanDepthCm();
		const FVector2D GridMin(
			InnerMinCm.X + (WidthCm() - SpanWidth) * 0.5f,
			InnerMinCm.Y + (DepthCm() - SpanDepth) * 0.5f);
		const FVector2D Origin = SolidFortressPrivate::RoomOriginCm(GridMin);
		for (int32 Row = -2; Row <= Rows + 2; ++Row)
		{
			for (int32 Col = -2; Col <= Columns + 2; ++Col)
			{
				const FVector2D Center = SolidStrongholdGrid::HexCenter(Col, Row, Origin);
				if (!SolidFortressPrivate::VerticesInside(Center, GridMin, FVector2D(SpanWidth, SpanDepth)))
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
		Place(Size);
	}

	void FSolidFortress::Place(const ESolidStrongholdSize InSize)
	{
		Size = InSize;
		Buildings.Reset();
		switch (InSize)
		{
		case ESolidStrongholdSize::Average:
			SolidFortressPrivate::AddRooms(
				*this,
				SolidFortressPrivate::AverageRooms,
				UE_ARRAY_COUNT(SolidFortressPrivate::AverageRooms));
			break;
		case ESolidStrongholdSize::Humble:
		case ESolidStrongholdSize::Meagre:
		case ESolidStrongholdSize::Impressive:
		case ESolidStrongholdSize::Grand:
		case ESolidStrongholdSize::Stately:
		case ESolidStrongholdSize::Imperious:
		default:
			SolidFortressPrivate::AddRooms(
				*this,
				SolidFortressPrivate::HumbleRooms,
				UE_ARRAY_COUNT(SolidFortressPrivate::HumbleRooms));
			break;
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
		float East = FSolidStronghold::WallCm;
		for (const FSolidFortressBuilding& One : Buildings)
		{
			East = FMath::Max(East, One.InnerMinCm.X + One.WidthCm() + FSolidStronghold::WallCm);
		}
		return East;
	}

	float FSolidFortress::OuterDepthCm() const
	{
		float North = FSolidStronghold::WallCm;
		for (const FSolidFortressBuilding& One : Buildings)
		{
			North = FMath::Max(North, One.InnerMinCm.Y + One.DepthCm() + FSolidStronghold::WallCm);
		}
		return North + YardDepthCm + FSolidStronghold::WallCm;
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
