#include "SolidFortress.h"
#include "SolidCore1.h"
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

		int32 RoomColumns(const ESolidStrongholdRoom Room)
		{
			switch (Room)
			{
			case ESolidStrongholdRoom::Large:
			case ESolidStrongholdRoom::Medium:
				return 4;
			case ESolidStrongholdRoom::Small:
				return 2;
			default:
				return 0;
			}
		}

		int32 RoomRows(const ESolidStrongholdRoom Room)
		{
			switch (Room)
			{
			case ESolidStrongholdRoom::Large:
				return 5;
			case ESolidStrongholdRoom::Medium:
			case ESolidStrongholdRoom::Small:
				return 3;
			default:
				return 0;
			}
		}
	}

	int32 FSolidFortressBuilding::Columns() const
	{
		if (Floors.Num() == 0)
		{
			return 0;
		}
		return SolidFortressPrivate::RoomColumns(Floors[0]);
	}

	int32 FSolidFortressBuilding::Rows() const
	{
		if (Floors.Num() == 0)
		{
			return 0;
		}
		return SolidFortressPrivate::RoomRows(Floors[0]);
	}

	float FSolidFortressBuilding::SpanWidthCm() const
	{
		const float ColumnSpan = (static_cast<float>(Columns()) + 0.5f) * SolidStrongholdGrid::FlatToFlatCm;
		const float Side = SolidStrongholdGrid::SideCm();
		const float RowSpan = static_cast<float>(Rows() - 1) * SolidStrongholdGrid::RowStepCm() + 2.f * Side;
		if (Facing == ESolidStrongholdOrientation::EastFace
			|| Facing == ESolidStrongholdOrientation::WestFace)
		{
			return RowSpan;
		}
		return ColumnSpan;
	}

	float FSolidFortressBuilding::SpanDepthCm() const
	{
		const float ColumnSpan = (static_cast<float>(Columns()) + 0.5f) * SolidStrongholdGrid::FlatToFlatCm;
		const float Side = SolidStrongholdGrid::SideCm();
		const float RowSpan = static_cast<float>(Rows() - 1) * SolidStrongholdGrid::RowStepCm() + 2.f * Side;
		if (Facing == ESolidStrongholdOrientation::EastFace
			|| Facing == ESolidStrongholdOrientation::WestFace)
		{
			return ColumnSpan;
		}
		return RowSpan;
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
		CollectFloorHexes(0, OutHexes);
	}

	void FSolidFortressBuilding::CollectFloorHexes(
		const int32 FloorIndex,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Floors.IsValidIndex(FloorIndex))
		{
			return;
		}
		const int32 ColumnCount = SolidFortressPrivate::RoomColumns(Floors[FloorIndex]);
		const int32 RowCount = SolidFortressPrivate::RoomRows(Floors[FloorIndex]);
		if (ColumnCount <= 0 || RowCount <= 0)
		{
			return;
		}
		const float ColumnSpan = (static_cast<float>(ColumnCount) + 0.5f) * SolidStrongholdGrid::FlatToFlatCm;
		const float RowSpan = static_cast<float>(RowCount - 1) * SolidStrongholdGrid::RowStepCm() + 2.f * SolidStrongholdGrid::SideCm();
		const bool bTurned = Facing == ESolidStrongholdOrientation::EastFace
			|| Facing == ESolidStrongholdOrientation::WestFace;
		const float SpanWidth = bTurned ? RowSpan : ColumnSpan;
		const float SpanDepth = bTurned ? ColumnSpan : RowSpan;
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
		const FVector2D Origin = bTurned
			? SolidFortressPrivate::RoomOriginCm(FVector2D::ZeroVector)
			: SolidFortressPrivate::RoomOriginCm(GridMin);
		for (int32 Row = -2; Row <= RowCount + 2; ++Row)
		{
			for (int32 Col = -2; Col <= ColumnCount + 2; ++Col)
			{
				const FVector2D Local = SolidStrongholdGrid::HexCenter(Col, Row, Origin);
				if (bTurned
					? !SolidFortressPrivate::VerticesInside(Local, FVector2D::ZeroVector, FVector2D(ColumnSpan, RowSpan))
					: !SolidFortressPrivate::VerticesInside(Local, GridMin, FVector2D(SpanWidth, SpanDepth)))
				{
					continue;
				}
				FVector2D Center = Local;
				if (Facing == ESolidStrongholdOrientation::WestFace)
				{
					Center = GridMin + FVector2D(RowSpan - Local.Y, Local.X);
				}
				else if (Facing == ESolidStrongholdOrientation::EastFace)
				{
					Center = GridMin + FVector2D(Local.Y, Local.X);
				}
				SolidStrongholdGrid::FHex Hex;
				Hex.Center = Center;
				Hex.Col = Col;
				Hex.Row = Row;
				if (Facing == ESolidStrongholdOrientation::WestFace)
				{
					Hex.YawDeg = 90.f;
				}
				else if (Facing == ESolidStrongholdOrientation::EastFace)
				{
					Hex.bReflect = true;
				}
				OutHexes.Add(Hex);
			}
		}
	}

	FSolidFortress::FSolidFortress()
	{
		Type = ESolidStrongholdType::Fortress;
		ParapetCm = SolidStrongholdGrid::FlatToFlatCm;
		Size = ESolidStrongholdSize::Humble;
		BindPlan();
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
		if (InFacing == ESolidStrongholdOrientation::EastFace
			|| InFacing == ESolidStrongholdOrientation::WestFace)
		{
			const float AlongDoor = Building.InnerWidthCm;
			Building.InnerWidthCm = Building.InnerDepthCm;
			Building.InnerDepthCm = AlongDoor;
		}
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
		ApplyNorthAnchor(Building);
		Buildings.Add(Building);
	}

	void FSolidFortress::ApplyNorthAnchor(FSolidFortressBuilding& Building)
	{
		if (PendingNorthOf == INDEX_NONE)
		{
			return;
		}
		const FSolidFortressBuilding& One = Buildings[PendingNorthOf];
		PendingNorthOf = INDEX_NONE;
		const float East = One.InnerMinCm.X + One.WidthCm();
		float MaxEast = East;
		float MinWest = One.InnerMinCm.X;
		for (const FSolidFortressBuilding& Each : Buildings)
		{
			MinWest = FMath::Min(MinWest, Each.InnerMinCm.X);
			MaxEast = FMath::Max(MaxEast, Each.InnerMinCm.X + Each.WidthCm());
		}
		const bool bEastWall = FMath::IsNearlyEqual(East, MaxEast, 0.05f)
			&& !FMath::IsNearlyEqual(One.InnerMinCm.X, MinWest, 0.05f);
		Building.InnerMinCm.X = bEastWall
			? East - Building.WidthCm()
			: One.InnerMinCm.X;
	}

	FVector2D FSolidFortress::EastOf(const int32 BuildingIndex) const
	{
		const FSolidFortressBuilding& One = Buildings[BuildingIndex];
		const float Wall = FSolidStronghold::WallCm;
		return FVector2D(
			One.InnerMinCm.X + One.WidthCm() + Wall + GapCm + Wall,
			One.InnerMinCm.Y);
	}

	FVector2D FSolidFortress::NorthOf(const int32 BuildingIndex)
	{
		PendingNorthOf = BuildingIndex;
		const FSolidFortressBuilding& One = Buildings[BuildingIndex];
		const float Wall = FSolidStronghold::WallCm;
		return FVector2D(
			One.InnerMinCm.X,
			One.InnerMinCm.Y + One.DepthCm() + Wall + GapCm + Wall);
	}

	void FSolidFortress::AddMediumHigh(
		const FVector2D PositionCm,
		const ESolidStrongholdOrientation InFacing,
		const std::initializer_list<ESolidStrongholdRoom> InFloors)
	{
		const float MediumWidth = 5.f * SolidStrongholdGrid::SideCm();
		// The medium span, plus the top landing. The ground landing is the first run of that span.
		const float MediumLength = 4.5f * SolidStrongholdGrid::FlatToFlatCm + LandingRunCm;
		AddBuilding(PositionCm, FVector2D(MediumLength, MediumWidth), InFacing, InFloors);
	}

	void FSolidFortress::AddLargeHigh(
		const FVector2D PositionCm,
		const ESolidStrongholdOrientation InFacing,
		const std::initializer_list<ESolidStrongholdRoom> InFloors)
	{
		const float LargeDepth = 8.f * SolidStrongholdGrid::SideCm();
		const float LargeLength = 4.5f * SolidStrongholdGrid::FlatToFlatCm + LandingRunCm;
		AddBuilding(PositionCm, FVector2D(LargeLength, LargeDepth), InFacing, InFloors);
	}

	void FSolidFortress::AddMediumLow(
		const FVector2D PositionCm,
		const ESolidStrongholdOrientation InFacing)
	{
		const float MediumWidth = 4.5f * SolidStrongholdGrid::FlatToFlatCm;
		const float MediumDepth = 5.f * SolidStrongholdGrid::SideCm();
		AddBuilding(PositionCm, FVector2D(MediumWidth, MediumDepth), InFacing, { ESolidStrongholdRoom::Medium });
	}

	void FSolidFortress::AddSmallLow(
		const FVector2D PositionCm,
		const ESolidStrongholdOrientation InFacing)
	{
		const float SmallWidth = 2.5f * SolidStrongholdGrid::FlatToFlatCm;
		const float SmallDepth = 5.f * SolidStrongholdGrid::SideCm();
		AddBuilding(PositionCm, FVector2D(SmallWidth, SmallDepth), InFacing, { ESolidStrongholdRoom::Small });
	}

	void FSolidFortress::AddCourtyard(const FVector2D PositionCm, const FVector2D SizeCm)
	{
		FSolidFortressBuilding Gap;
		Gap.InnerMinCm = PositionCm;
		Gap.InnerWidthCm = SizeCm.X;
		Gap.InnerDepthCm = SizeCm.Y;
		ApplyNorthAnchor(Gap);
		Buildings.Add(Gap);
	}

	void FSolidFortress::AddBuildings(const ESolidStrongholdSize InSize)
	{
		using enum ESolidStrongholdRoom;
		using enum ESolidStrongholdOrientation;

		Size = InSize;
		Buildings.Reset();
		PendingNorthOf = INDEX_NONE;
		using SolidFortressPrivate::Position;
		using SolidFortressPrivate::Size;
		const float Wall = FSolidStronghold::WallCm;
		const float MediumWidth = 5.f * SolidStrongholdGrid::SideCm();
		const FVector2D MediumGap = Size(MediumWidth, MediumWidth);
		switch (InSize)
		{
		case ESolidStrongholdSize::Meagre:
			UE_LOG(LogSolid, Error, TEXT("Meagre is not a fortress size."));
			break;
		case ESolidStrongholdSize::Humble:
			AddMediumHigh(Position(Wall, Wall), NorthFace, { Small, Medium });
			AddCourtyard(NorthOf(0), MediumGap);
			AddCourtyard(EastOf(0), MediumGap);
			break;
		case ESolidStrongholdSize::Average:
			AddMediumHigh(Position(Wall, Wall), NorthFace, { Small, Medium });
			AddSmallLow(EastOf(0), NorthFace);
			AddCourtyard(NorthOf(0), MediumGap);
			AddCourtyard(EastOf(1), MediumGap);
			break;
		case ESolidStrongholdSize::Impressive:
			AddMediumHigh(Position(Wall, Wall), NorthFace, { Medium, Medium, Medium });
			AddSmallLow(EastOf(0), NorthFace);
			AddCourtyard(EastOf(1), MediumGap);
			AddSmallLow(EastOf(2), NorthFace);
			AddCourtyard(NorthOf(0), MediumGap);
			break;
		case ESolidStrongholdSize::Grand:
			AddLargeHigh(Position(Wall, Wall), NorthFace, { Large, Medium, Medium });
			AddMediumHigh(EastOf(0), NorthFace, { Medium, Medium });
			AddSmallLow(EastOf(1), NorthFace);
			AddSmallLow(EastOf(2), NorthFace);
			AddCourtyard(NorthOf(0), MediumGap);
			AddCourtyard(EastOf(3), MediumGap);
			break;
		case ESolidStrongholdSize::Stately:
			AddLargeHigh(Position(Wall, Wall), NorthFace, { Large, Large, Medium });
			AddMediumHigh(EastOf(0), NorthFace, { Medium, Medium });
			AddMediumLow(EastOf(1), NorthFace);
			AddSmallLow(EastOf(2), NorthFace);
			AddSmallLow(EastOf(3), NorthFace);
			AddCourtyard(NorthOf(0), MediumGap);
			AddCourtyard(EastOf(4), MediumGap);
			break;
		case ESolidStrongholdSize::Imperious:
			AddLargeHigh(Position(Wall, Wall), NorthFace, { Large, Large, Large });
			AddSmallLow(EastOf(0), NorthFace);
			AddMediumHigh(EastOf(1), NorthFace, { Medium, Medium });
			AddCourtyard(EastOf(2), MediumGap);
			AddMediumHigh(EastOf(3), WestFace, { Medium, Medium });
			AddSmallLow(NorthOf(4), WestFace);
			AddCourtyard(NorthOf(5), MediumGap);
			AddSmallLow(NorthOf(6), WestFace);
			break;
		}
	}

	const FSolidFortress& FSolidFortress::Instance()
	{
		static const FSolidFortress Fortress;
		return Fortress;
	}

	void FSolidFortress::BindPlan()
	{
		AddBuildings(Size);
	}

	bool FSolidFortress::Supported(const ESolidStrongholdSize InSize) const
	{
		return InSize != ESolidStrongholdSize::Meagre;
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
		return East;
	}

	float FSolidFortress::OuterDepthCm() const
	{
		float North = FSolidStronghold::WallCm;
		for (const FSolidFortressBuilding& One : Buildings)
		{
			North = FMath::Max(North, One.InnerMinCm.Y + One.DepthCm() + FSolidStronghold::WallCm);
		}
		return North;
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
		FSolidFortress Sized;
		Sized.AddBuildings(InSize);
		int32 Shift = 0;
		for (const FSolidFortressBuilding& One : Sized.Buildings)
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
