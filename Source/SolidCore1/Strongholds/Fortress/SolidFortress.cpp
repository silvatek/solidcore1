#include "SolidFortress.h"
#include "SolidGameState.h"
#include "Strongholds/Fortress/Actors/SolidFortressExterior.h"
#include "Strongholds/Fortress/Actors/SolidFortressInterior.h"
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

		/** Inner south-west corner, in centimetres. */
		FVector2D Position(const float X, const float Y)
		{
			return FVector2D(X, Y);
		}

		/** Inner face, in centimetres. */
		FVector2D Size(const float Width, const float Depth)
		{
			return FVector2D(Width, Depth);
		}
	}

	int32 FSolidFortressBuilding::Columns() const
	{
		if (Floors.Num() == 0)
		{
			return 0;
		}
		switch (Floors[0])
		{
		case ESolidStrongholdRoom::Medium:
			return 4;
		case ESolidStrongholdRoom::Small:
			return 2;
		default:
			return 0;
		}
	}

	int32 FSolidFortressBuilding::Rows() const
	{
		if (Floors.Num() == 0)
		{
			return 0;
		}
		switch (Floors[0])
		{
		case ESolidStrongholdRoom::Medium:
		case ESolidStrongholdRoom::Small:
			return 3;
		default:
			return 0;
		}
	}

	float FSolidFortressBuilding::SpanWidthCm() const
	{
		return (static_cast<float>(Columns()) + 0.5f) * SolidStrongholdGrid::FlatToFlatCm;
	}

	float FSolidFortressBuilding::SpanDepthCm() const
	{
		const float Side = SolidStrongholdGrid::SideCm();
		return static_cast<float>(Rows() - 1) * SolidStrongholdGrid::RowStepCm() + 2.f * Side;
	}

	float FSolidFortressBuilding::WidthCm() const
	{
		return FMath::Max(InnerWidthCm, SpanWidthCm());
	}

	float FSolidFortressBuilding::DepthCm() const
	{
		return FMath::Max(InnerDepthCm, SpanDepthCm());
	}

	float FSolidFortressBuilding::HeightCm(const float InParapetCm) const
	{
		const int32 Storeys = FMath::Max(Floors.Num(), 1);
		return static_cast<float>(Storeys) * FSolidStronghold::StoreyCm + InParapetCm;
	}

	bool FSolidFortressBuilding::HasStair() const
	{
		return Floors.Num() >= 2;
	}

	float FSolidFortressBuilding::DoorFromInnerCm() const
	{
		return HasStair() ? LandingRunCm * 0.5f : 0.f;
	}

	void FSolidFortressBuilding::CollectHexes(TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		const float SpanWidth = SpanWidthCm();
		const float SpanDepth = SpanDepthCm();
		// One floor sits in the middle. A stair keeps the hexes at the landing end of the room block.
		FVector2D GridMin(
			InnerMinCm.X + (WidthCm() - SpanWidth) * 0.5f,
			InnerMinCm.Y + (DepthCm() - SpanDepth) * 0.5f);
		if (HasStair())
		{
			GridMin = InnerMinCm;
			const float StairCm = FPlan{}.StairWidthCm;
			if (Facing == ESolidStrongholdOrientation::SouthFace)
			{
				GridMin.Y += StairCm;
			}
			else if (Facing == ESolidStrongholdOrientation::WestFace)
			{
				GridMin.X += StairCm;
			}
		}
		const FVector2D Origin = SolidFortressPrivate::RoomOriginCm(GridMin);
		const int32 RowCount = Rows();
		const int32 ColumnCount = Columns();
		for (int32 Row = -2; Row <= RowCount + 2; ++Row)
		{
			for (int32 Col = -2; Col <= ColumnCount + 2; ++Col)
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
		AddBuildings(ESolidStrongholdSize::Humble);
	}

	void FSolidFortress::AddBuilding(
		const FVector2D PositionCm,
		const FVector2D FootprintCm,
		const ESolidStrongholdOrientation InFacing,
		const std::initializer_list<ESolidStrongholdRoom> InFloors)
	{
		FSolidFortressBuilding Building;
		Building.Floors = InFloors;
		Building.Facing = InFacing;
		Building.InnerMinCm = PositionCm;
		Building.InnerWidthCm = FootprintCm.X;
		Building.InnerDepthCm = FootprintCm.Y;
		if (Building.HasStair())
		{
			const float StairCm = FPlan{}.StairWidthCm;
			if (InFacing == ESolidStrongholdOrientation::NorthFace || InFacing == ESolidStrongholdOrientation::SouthFace)
			{
				Building.InnerDepthCm += StairCm;
			}
			else
			{
				Building.InnerWidthCm += StairCm;
			}
		}
		Buildings.Add(Building);
	}

	void FSolidFortress::AdditionalCourtyard(const float DepthCm, const float WidthCm)
	{
		YardDepthCm = DepthCm;
		YardWidthCm = WidthCm;
	}

	void FSolidFortress::AddBuildings(const ESolidStrongholdSize InSize)
	{
		using enum ESolidStrongholdRoom;
		using enum ESolidStrongholdOrientation;

		Size = InSize;
		Buildings.Reset();
		using SolidFortressPrivate::Position;
		using SolidFortressPrivate::Size;
		const float Wall = FSolidStronghold::WallCm;
		const float MediumWidth = 5.f * SolidStrongholdGrid::SideCm();
		const float MediumLength = 4.5f * SolidStrongholdGrid::FlatToFlatCm;
		switch (InSize)
		{
		case ESolidStrongholdSize::Humble:
		case ESolidStrongholdSize::Average:
			// The east yard plus the east curtain equals the building's run, so the outer wall is twice as wide.
			AdditionalCourtyard(FSolidStronghold::StoreyCm, MediumLength + Wall);
			AddBuilding(Position(Wall, Wall), Size(MediumLength, MediumWidth), NorthFace, { Small, Medium });
			break;
		case ESolidStrongholdSize::Meagre:
		case ESolidStrongholdSize::Impressive:
		case ESolidStrongholdSize::Grand:
		case ESolidStrongholdSize::Stately:
		case ESolidStrongholdSize::Imperious:
		default:
			AdditionalCourtyard(FSolidStronghold::StoreyCm, 0.f);
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

	float FSolidFortress::OuterWidthCm() const
	{
		float East = FSolidStronghold::WallCm;
		for (const FSolidFortressBuilding& One : Buildings)
		{
			East = FMath::Max(East, One.InnerMinCm.X + One.WidthCm() + FSolidStronghold::WallCm);
		}
		if (YardWidthCm <= 0.f)
		{
			return East;
		}
		return East + YardWidthCm + FSolidStronghold::WallCm;
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
