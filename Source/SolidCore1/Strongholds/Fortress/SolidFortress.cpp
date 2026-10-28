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

		struct FRoomPlan
		{
			ESolidStrongholdRoom Size = ESolidStrongholdRoom::Small;
			int32 Columns = 0;
			int32 Rows = 0;
			bool bKeep = false;
			FVector2D InnerMinCm = FVector2D::ZeroVector;
			float InnerWidthCm = 0.f;
			float InnerDepthCm = 0.f;
		};

		FRoomPlan PlanRoom(const ESolidStrongholdRoom Size, const int32 Columns, const int32 Rows)
		{
			FRoomPlan Plan;
			Plan.Size = Size;
			Plan.Columns = Columns;
			Plan.Rows = Rows;
			return Plan;
		}

		FSolidFortressBuilding Building(const FRoomPlan& Plan)
		{
			FSolidFortressBuilding Room;
			Room.Room = Plan.Size;
			Room.bKeep = Plan.bKeep;
			Room.Columns = Plan.Columns;
			Room.Rows = Plan.Rows;
			Room.InnerMinCm = Plan.InnerMinCm;
			Room.InnerWidthCm = Plan.InnerWidthCm;
			Room.InnerDepthCm = Plan.InnerDepthCm;
			return Room;
		}

		void AddRoom(FSolidFortress& Fortress, const FRoomPlan& Plan)
		{
			Fortress.Buildings.Add(Building(Plan));
		}

		/** South-west corner of the next room, whose wall meets this room's north wall. */
		FVector2D NorthOf(const FRoomPlan& Room)
		{
			const FSolidFortressBuilding Built = Building(Room);
			return FVector2D(
				Room.InnerMinCm.X,
				Room.InnerMinCm.Y + Built.DepthCm() + 2.f * FSolidStronghold::WallCm);
		}

		/** South-west corner of the next room, whose wall meets this room's east wall. */
		FVector2D EastOf(const FRoomPlan& Room)
		{
			const FSolidFortressBuilding Built = Building(Room);
			return FVector2D(
				Room.InnerMinCm.X + Built.WidthCm() + 2.f * FSolidStronghold::WallCm,
				Room.InnerMinCm.Y);
		}

		void PlaceHumble(FSolidFortress& Fortress)
		{
			FRoomPlan Small = PlanRoom(ESolidStrongholdRoom::Small, 2, 3);
			FRoomPlan Keep = PlanRoom(ESolidStrongholdRoom::Medium, 4, 3);
			Keep.bKeep = true;
			const FRoomPlan Row[] = { Small, Keep };
			float X = FSolidStronghold::WallCm;
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(Row); ++Index)
			{
				FRoomPlan Room = Row[Index];
				Room.InnerMinCm = FVector2D(X, FSolidStronghold::WallCm);
				AddRoom(Fortress, Room);
				X += Building(Room).WidthCm() + FSolidStronghold::WallCm;
				if (Index + 1 < UE_ARRAY_COUNT(Row))
				{
					X += Fortress.GapCm + FSolidStronghold::WallCm;
				}
			}
		}

		void PlaceAverage(FSolidFortress& Fortress)
		{
			FRoomPlan Keep = PlanRoom(ESolidStrongholdRoom::Medium, 4, 3);
			Keep.bKeep = true;
			Keep.InnerMinCm = FVector2D(FSolidStronghold::WallCm, FSolidStronghold::WallCm);
			Keep.InnerWidthCm = Building(Keep).SpanWidthCm() + 100.f;

			FRoomPlan West = PlanRoom(ESolidStrongholdRoom::Small, 2, 3);
			West.InnerMinCm = NorthOf(Keep);

			AddRoom(Fortress, West);
			AddRoom(Fortress, Keep);

			FRoomPlan East = PlanRoom(ESolidStrongholdRoom::Small, 2, 3);
			East.InnerMinCm = EastOf(Keep);
			East.InnerWidthCm = Fortress.OuterDepthCm() - FSolidStronghold::WallCm - East.InnerMinCm.X;
			AddRoom(Fortress, East);
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
			SolidFortressPrivate::PlaceAverage(*this);
			break;
		case ESolidStrongholdSize::Humble:
		case ESolidStrongholdSize::Meagre:
		case ESolidStrongholdSize::Impressive:
		case ESolidStrongholdSize::Grand:
		case ESolidStrongholdSize::Stately:
		case ESolidStrongholdSize::Imperious:
		default:
			SolidFortressPrivate::PlaceHumble(*this);
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
