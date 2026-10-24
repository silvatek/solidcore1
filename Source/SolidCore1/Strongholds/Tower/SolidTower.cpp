#include "SolidTower.h"
#include "SolidGameState.h"
#include "Strongholds/Tower/Actors/SolidTowerExterior.h"
#include "Strongholds/Tower/Actors/SolidTowerInterior.h"
#include "Strongholds/SolidStronghold.h"

namespace SolidStronghold
{
	float HeightCm(const FPlan& Plan)
	{
		return static_cast<float>(Plan.FloorCount) * Plan.FloorPitchCm;
	}

	float ClearHeightCm(const FPlan& Plan)
	{
		return Plan.FloorPitchCm - Plan.SlabCm;
	}

	float GroundInteriorRadiusCm(const FPlan& Plan)
	{
		return Plan.GroundExteriorRadiusCm - Plan.WallCm;
	}

	float TopInteriorRadiusCm(const FPlan& Plan)
	{
		return Plan.TopExteriorRadiusCm - Plan.WallCm;
	}

	float StairRunCm(const FPlan& Plan)
	{
		return static_cast<float>(Plan.StairTreadCount) * Plan.StairTreadCm;
	}

	float RiserCm(const FPlan& Plan)
	{
		if (Plan.StairRiserCount <= 0)
		{
			return 0.f;
		}
		return Plan.FloorPitchCm / static_cast<float>(Plan.StairRiserCount);
	}

	FSolidTower::FSolidTower()
	{
		Type = ESolidStrongholdType::Tower;
		BindPlan();
	}

	const FSolidTower& FSolidTower::Instance()
	{
		static const FSolidTower Tower;
		return Tower;
	}

	void FSolidTower::BindPlan()
	{
		StoredPlan = MakePlan(Size);
	}

	float FSolidTower::FloorZCm(const int32 Floor) const
	{
		return SolidStronghold::FloorZCm(StoredPlan, Floor);
	}

	float FSolidTower::InteriorRadiusCm(const float ZCm) const
	{
		return SolidStronghold::InteriorRadiusCm(StoredPlan, ZCm);
	}

	void FSolidTower::CollectFloorHexes(const int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		SolidStronghold::CollectKeptHexes(StoredPlan, Floor, OutHexes);
		if (!IsImperiousPlan() && StoredPlan.SourceFloor <= 0)
		{
			return;
		}
		const int32 Want = ImperiousFloorHexCount(StoredPlan.SourceFloor + Floor);
		if (Want <= 0 || OutHexes.Num() == 0)
		{
			return;
		}

		TArray<bool> Keep;
		Keep.Init(true, OutHexes.Num());
		int32 Alive = OutHexes.Num();
		if (Floor == 0 && Alive > Want)
		{
			const FVector2D DoorPoint = DoorApproachPoint();
			int32 DoorCell = INDEX_NONE;
			float BestDist = TNumericLimits<float>::Max();
			for (int32 Index = 0; Index < OutHexes.Num(); ++Index)
			{
				const float Dist = FVector2D::DistSquared(OutHexes[Index].Center, DoorPoint);
				if (Dist < BestDist)
				{
					BestDist = Dist;
					DoorCell = Index;
				}
			}
			if (DoorCell != INDEX_NONE)
			{
				Keep[DoorCell] = false;
				--Alive;
			}
		}

		while (Alive > Want)
		{
			int32 Drop = INDEX_NONE;
			float BestDist = -1.f;
			int32 BestRow = TNumericLimits<int32>::Lowest();
			int32 BestCol = TNumericLimits<int32>::Lowest();
			for (int32 Index = 0; Index < OutHexes.Num(); ++Index)
			{
				if (!Keep[Index])
				{
					continue;
				}
				Keep[Index] = false;
				const bool bConnected = FloorHexesConnect(OutHexes, Keep, Alive - 1);
				Keep[Index] = true;
				if (!bConnected)
				{
					continue;
				}
				const SolidStrongholdGrid::FHex& Hex = OutHexes[Index];
				const float Dist = Hex.Center.SizeSquared();
				const bool bFarther = Dist > BestDist + 0.01f
					|| (FMath::Abs(Dist - BestDist) <= 0.01f
						&& (Hex.Row > BestRow || (Hex.Row == BestRow && Hex.Col > BestCol)));
				if (Drop == INDEX_NONE || bFarther)
				{
					Drop = Index;
					BestDist = Dist;
					BestRow = Hex.Row;
					BestCol = Hex.Col;
				}
			}
			if (Drop == INDEX_NONE)
			{
				break;
			}
			Keep[Drop] = false;
			--Alive;
		}

		TArray<SolidStrongholdGrid::FHex> Room;
		Room.Reserve(Alive);
		for (int32 Index = 0; Index < OutHexes.Num(); ++Index)
		{
			if (Keep[Index])
			{
				Room.Add(OutHexes[Index]);
			}
		}
		OutHexes = Room;
	}

	float FSolidTower::ExteriorRadiusCm(const float ZCm) const
	{
		const float Height = FMath::Max(HeightCm(), 1.f);
		const float Along = FMath::Clamp(ZCm / Height, 0.f, 1.f);
		return FMath::Lerp(StoredPlan.GroundExteriorRadiusCm, StoredPlan.TopExteriorRadiusCm, Along);
	}

	bool FSolidTower::HasDoor(const int32 Floor) const
	{
		return Floor == 0;
	}

	FVector2D FSolidTower::DoorOutward() const
	{
		return FVector2D(FMath::Cos(StoredPlan.DoorAngleRad), FMath::Sin(StoredPlan.DoorAngleRad));
	}

	FVector FSolidTower::DoorSlabCenterCm() const
	{
		const float Radius = StoredPlan.GroundExteriorRadiusCm;
		const float X = -(Radius - DoorSlabDepthCm * 0.5f + DoorProudCm);
		return FVector(X, 0.f, StoredPlan.DoorHeightCm * 0.5f);
	}

	FVector2D FSolidTower::DoorHexCenter() const
	{
		TArray<SolidStrongholdGrid::FHex> Hexes;
		SolidStronghold::CollectKeptHexes(StoredPlan, 0, Hexes);
		const FVector2D DoorPoint = DoorApproachPoint();
		FVector2D Best = FVector2D::ZeroVector;
		float BestDist = TNumericLimits<float>::Max();
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			const float Dist = FVector2D::DistSquared(Hex.Center, DoorPoint);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				Best = Hex.Center;
			}
		}
		return Best;
	}

	bool FSolidTower::FloorHexesConnect(
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const TArray<bool>& Keep,
		const int32 Alive) const
	{
		int32 Start = INDEX_NONE;
		for (int32 Index = 0; Index < Hexes.Num(); ++Index)
		{
			if (Keep[Index])
			{
				Start = Index;
				break;
			}
		}
		if (Start == INDEX_NONE)
		{
			return Alive == 0;
		}

		TArray<int32> Queue;
		TArray<bool> Seen;
		Seen.Init(false, Hexes.Num());
		Queue.Add(Start);
		Seen[Start] = true;
		int32 Reached = 0;
		int32 Cursor = 0;
		while (Cursor < Queue.Num())
		{
			const int32 At = Queue[Cursor];
			++Cursor;
			++Reached;
			for (int32 Other = 0; Other < Hexes.Num(); ++Other)
			{
				if (!Keep[Other] || Seen[Other] || !HexesShareSide(Hexes[At], Hexes[Other]))
				{
					continue;
				}
				Seen[Other] = true;
				Queue.Add(Other);
			}
		}
		return Reached == Alive;
	}

	FVector2D FSolidTower::DoorApproachPoint() const
	{
		return DoorOutward() * FMath::Max(GroundInteriorRadiusCm() - 30.f, 0.f);
	}

	bool FSolidTower::IsImperiousPlan() const
	{
		return StoredPlan.FloorCount == Imperious.FloorCount
			&& FMath::IsNearlyEqual(StoredPlan.GroundExteriorRadiusCm, Imperious.GroundExteriorRadiusCm, 0.01f)
			&& FMath::IsNearlyEqual(StoredPlan.TopExteriorRadiusCm, Imperious.TopExteriorRadiusCm, 0.01f);
	}

	int32 FSolidTower::ImperiousFloorHexCount(const int32 Floor) const
	{
		static const int32 CountByFloor[] = { 20, 20, 20, 12, 12, 12, 12, 6, 6, 6 };
		if (Floor < 0 || Floor >= UE_ARRAY_COUNT(CountByFloor))
		{
			return 0;
		}
		return CountByFloor[Floor];
	}

	bool FSolidTower::HexesShareSide(const SolidStrongholdGrid::FHex& A, const SolidStrongholdGrid::FHex& B) const
	{
		return FMath::IsNearlyEqual(
			FVector2D::Distance(A.Center, B.Center),
			SolidStrongholdGrid::FlatToFlatCm,
			15.f);
	}

	bool FSolidTower::Supported(const ESolidStrongholdSize InSize) const
	{
		FSolidTowerBand Band;
		return FindTowerBand(InSize, Band);
	}

	ESolidStrongholdTrip FSolidTower::Trip() const
	{
		return ESolidStrongholdTrip::Tower;
	}

	bool FSolidTower::AcceptsEnter(
		const USolidGameState& State,
		const FSolidStronghold* Active,
		const FIntPoint OpeningCell) const
	{
		return State.bStrongholdsEnabled
			&& Active
			&& Active->Type == ESolidStrongholdType::Tower
			&& Active->Cell == OpeningCell;
	}

	UClass* FSolidTower::ExteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidTowerExterior::StaticClass();
	}

	void FSolidTower::ConfigureExterior(AActor* Exterior) const
	{
		ASolidTowerExterior* Shell = Cast<ASolidTowerExterior>(Exterior);
		if (!Shell)
		{
			return;
		}
		if (!Supported(Size))
		{
			return;
		}
		Shell->SetTower(*this);
		Shell->SetStrongholdCell(Cell);
		Shell->BuildVisuals();
	}

	UClass* FSolidTower::InteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidTowerInterior::StaticClass();
	}

	void FSolidTower::ConfigureInterior(
		AActor* Interior,
		const TArray<SolidStrongholdFurniture::FUsed>& Furnishings) const
	{
		ASolidTowerInterior* Room = Cast<ASolidTowerInterior>(Interior);
		if (!Room)
		{
			return;
		}
		Room->SetStrongholdCell(Cell);
		Room->SetTower(*this);
		Room->BuildVisuals();
		Room->PlaceFurnishings(Furnishings);
	}

	float FSolidTower::ExteriorYawDeg(const USolidWorldChart* Map) const
	{
		SolidStrongholds::FGrassFace Face;
		const float GrassYaw = (Map && SolidStrongholds::FindGrassFace(Map, Cell, Face))
			? Face.FacingYawDeg
			: 0.f;
		return ExteriorYawDeg(GrassYaw);
	}

	float FSolidTower::ExteriorYawDeg(const float GrassFacingYawDeg) const
	{
		return FMath::UnwindDegrees(GrassFacingYawDeg + 180.f);
	}

	FVector2D FSolidTower::OutsideDoorXY(const FVector2D& CellCenterXY, const float GrassFacingYawDeg) const
	{
		const float YawRad = FMath::DegreesToRadians(GrassFacingYawDeg);
		const FVector2D Grass(FMath::Cos(YawRad), FMath::Sin(YawRad));
		const float Distance = StoredPlan.GroundExteriorRadiusCm + DoorProudCm + DoorStandClearCm;
		return CellCenterXY + Grass * Distance;
	}

	float FSolidTower::ExitReachCm() const
	{
		if (!Supported(Size))
		{
			return DoorStandClearCm;
		}
		return Plan().GroundExteriorRadiusCm
			+ DoorProudCm
			+ DoorStandClearCm;
	}

	void FSolidTower::RememberExit(const USolidWorldChart* Map)
	{
		SolidStrongholds::FGrassFace Face;
		const float GrassYaw = (Map && SolidStrongholds::FindGrassFace(Map, Cell, Face))
			? Face.FacingYawDeg
			: 0.f;
		Orientation = OrientationFromYaw(GrassYaw);
	}

	void FSolidTower::CollectGroundHexes(
		const ESolidStrongholdSize InSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Supported(InSize))
		{
			return;
		}
		FSolidTower Sized;
		Sized.Size = InSize;
		Sized.BindPlan();
		Sized.CollectFloorHexes(0, OutHexes);
	}

	TUniquePtr<FSolidStronghold> FSolidTower::Clone() const
	{
		return MakeUnique<FSolidTower>(*this);
	}

	FSolidNameSignPlace FSolidTower::NameSign() const
	{
		const FVector Door = DoorSlabCenterCm();
		FSolidNameSignPlace Place;
		Place.FrameEdgeCm = FVector(Door.X, Plan().DoorWidthCm * 0.5f, 0.f);
		Place.Outward = FVector2D(0.f, 1.f);
		Place.YawDeg = 180.f;
		return Place;
	}

	float SizeAlong(const ESolidStrongholdSize Size)
	{
		const float From = static_cast<float>(ESolidStrongholdSize::Average);
		const float To = static_cast<float>(ESolidStrongholdSize::Imperious);
		return (static_cast<float>(Size) - From) / (To - From);
	}

	int32 LerpCount(const int32 From, const int32 To, const float Along)
	{
		const float Blended = FMath::Lerp(static_cast<float>(From), static_cast<float>(To), Along);
		return FMath::FloorToInt(Blended + 0.5f);
	}

	FPlan LerpPlan(const FPlan& From, const FPlan& To, const float Along)
	{
		FPlan Plan;
		Plan.FloorCount = LerpCount(From.FloorCount, To.FloorCount, Along);
		Plan.FloorPitchCm = FMath::Lerp(From.FloorPitchCm, To.FloorPitchCm, Along);
		Plan.SlabCm = FMath::Lerp(From.SlabCm, To.SlabCm, Along);
		Plan.WallCm = FMath::Lerp(From.WallCm, To.WallCm, Along);
		Plan.GroundExteriorRadiusCm = FMath::Lerp(From.GroundExteriorRadiusCm, To.GroundExteriorRadiusCm, Along);
		Plan.TopExteriorRadiusCm = FMath::Lerp(From.TopExteriorRadiusCm, To.TopExteriorRadiusCm, Along);
		Plan.DoorWidthCm = FMath::Lerp(From.DoorWidthCm, To.DoorWidthCm, Along);
		Plan.DoorHeightCm = FMath::Lerp(From.DoorHeightCm, To.DoorHeightCm, Along);
		Plan.StairWidthCm = FMath::Lerp(From.StairWidthCm, To.StairWidthCm, Along);
		Plan.StairRiserCount = LerpCount(From.StairRiserCount, To.StairRiserCount, Along);
		Plan.StairTreadCount = LerpCount(From.StairTreadCount, To.StairTreadCount, Along);
		Plan.StairTreadCm = FMath::Lerp(From.StairTreadCm, To.StairTreadCm, Along);
		Plan.DoorAngleRad = FMath::Lerp(From.DoorAngleRad, To.DoorAngleRad, Along);
		Plan.StairPastDoorRad = FMath::Lerp(From.StairPastDoorRad, To.StairPastDoorRad, Along);
		return Plan;
	}

	FPlan LegacyLowPlan()
	{
		FPlan Small;
		Small.FloorCount = 4;
		Small.GroundExteriorRadiusCm = 400.f;
		Small.TopExteriorRadiusCm = 280.f;
		return Small;
	}

	FPlan MakeFloorBand(const int32 SourceFloor, const int32 FloorCount)
	{
		const FPlan Shaft;
		FPlan Plan = Shaft;
		Plan.SourceFloor = SourceFloor;
		Plan.FloorCount = FloorCount;
		const float Height = FMath::Max(HeightCm(Shaft), 1.f);
		const float BottomAlong = static_cast<float>(Plan.SourceFloor) * Shaft.FloorPitchCm / Height;
		const float TopAlong = static_cast<float>(Plan.SourceFloor + Plan.FloorCount) * Shaft.FloorPitchCm / Height;
		Plan.GroundExteriorRadiusCm = FMath::Lerp(Shaft.GroundExteriorRadiusCm, Shaft.TopExteriorRadiusCm, BottomAlong);
		Plan.TopExteriorRadiusCm = FMath::Lerp(Shaft.GroundExteriorRadiusCm, Shaft.TopExteriorRadiusCm, TopAlong);
		return Plan;
	}

	FPlan MakePlan(const ESolidStrongholdSize Size)
	{
		SolidStronghold::FSolidTowerBand Band;
		if (SolidStronghold::FindTowerBand(Size, Band))
		{
			return MakeFloorBand(SolidStronghold::BandSourceFloor(Band), SolidStronghold::BandFloorCount(Band));
		}
		const FPlan Large;
		return LerpPlan(LegacyLowPlan(), Large, SizeAlong(Size));
	}

	const FPlan Average = MakePlan(ESolidStrongholdSize::Average);

	const FPlan Impressive = MakePlan(ESolidStrongholdSize::Impressive);

	const FPlan Grand = MakePlan(ESolidStrongholdSize::Grand);

	const FPlan Stately = MakePlan(ESolidStrongholdSize::Stately);

	const FPlan Imperious = MakePlan(ESolidStrongholdSize::Imperious);

	bool IsFloor(const FPlan& Plan, const int32 Floor)
	{
		return Floor >= 0 && Floor < Plan.FloorCount;
	}

	float FloorZCm(const FPlan& Plan, const int32 Floor)
	{
		return static_cast<float>(Floor) * Plan.FloorPitchCm;
	}

	float InteriorRadiusCm(const FPlan& Plan, const float ZCm)
	{
		const float Height = FMath::Max(HeightCm(Plan), 1.f);
		const float Along = FMath::Clamp(ZCm / Height, 0.f, 1.f);
		return FMath::Lerp(GroundInteriorRadiusCm(Plan), TopInteriorRadiusCm(Plan), Along);
	}

	float FlightSweepRad(const FPlan& Plan, const int32 Floor)
	{
		if (Floor < 0 || Floor >= Plan.FloorCount - 1)
		{
			return 0.f;
		}
		const float Centerline = InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)) - Plan.StairWidthCm * 0.5f;
		if (Centerline <= 0.f)
		{
			return 0.f;
		}
		return StairRunCm(Plan) / Centerline;
	}

	bool HasLanding(const FPlan& Plan, const int32 Floor)
	{
		return IsFloor(Plan, Floor) && Floor > 0 && FlightSweepRad(Plan, Floor - 1) > 0.f;
	}

	float LandingSweepRad(const FPlan& Plan, const int32 Floor)
	{
		if (!HasLanding(Plan, Floor))
		{
			return 0.f;
		}
		const float Centerline = InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)) - Plan.StairWidthCm * 0.5f;
		if (Centerline <= 0.f)
		{
			return 0.f;
		}
		return LandingRunCm / Centerline;
	}

	float FlightStartRad(const FPlan& Plan, const int32 Floor)
	{
		float Start = Plan.DoorAngleRad + Plan.StairPastDoorRad;
		const int32 Last = FMath::Clamp(Floor, 0, FMath::Max(Plan.FloorCount - 1, 0));
		for (int32 Index = 0; Index < Last; ++Index)
		{
			Start += FlightSweepRad(Plan, Index);
			Start += LandingSweepRad(Plan, Index + 1);
		}
		return Start;
	}

	FWedge FlightWedge(const FPlan& Plan, const int32 Floor)
	{
		FWedge Wedge;
		if (Floor < 0 || Floor >= Plan.FloorCount - 1)
		{
			return Wedge;
		}
		Wedge.bOpen = true;
		Wedge.StartRad = FlightStartRad(Plan, Floor);
		Wedge.SweepRad = FlightSweepRad(Plan, Floor);
		if (Wedge.SweepRad <= 0.f)
		{
			Wedge.bOpen = false;
		}
		return Wedge;
	}

	FWedge GapWedge(const FPlan& Plan, const int32 Floor)
	{
		if (Floor <= 0)
		{
			return FWedge{};
		}
		return FlightWedge(Plan, Floor - 1);
	}

	int32 FloorNumber(const int32 Floor)
	{
		return Floor + 1;
	}

	FWedge LandingWedge(const FPlan& Plan, const int32 Floor)
	{
		FWedge Wedge;
		const float Sweep = LandingSweepRad(Plan, Floor);
		if (Sweep <= 0.f)
		{
			return Wedge;
		}
		Wedge.bOpen = true;
		Wedge.StartRad = FlightStartRad(Plan, Floor - 1) + FlightSweepRad(Plan, Floor - 1);
		Wedge.SweepRad = Sweep;
		return Wedge;
	}

	SolidStronghold::FSolidFloorSign FloorSignFor(const FPlan& Plan, const int32 Floor)
	{
		SolidStronghold::FSolidFloorSign Sign;
		const FWedge Landing = LandingWedge(Plan, Floor);
		if (!Landing.bOpen)
		{
			return Sign;
		}
		const float Angle = Landing.StartRad + Landing.SweepRad * 0.5f;
		const float SignZ = FloorZCm(Plan, Floor) + FloorSignLiftCm;
		const float Radius = FMath::Max(InteriorRadiusCm(Plan, SignZ) - FloorSignInsetCm, 0.f);
		const float InwardYaw = Angle + PI;
		Sign.Place(
			FloorNumber(Floor),
			FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SignZ),
			FMath::RadiansToDegrees(InwardYaw));
		return Sign;
	}

	float WrapTau(const float Angle)
	{
		const float Tau = 2.f * PI;
		float Wrapped = FMath::Fmod(Angle, Tau);
		if (Wrapped < 0.f)
		{
			Wrapped += Tau;
		}
		return Wrapped;
	}

	bool AngleInSweep(const float Angle, const float StartRad, const float SweepRad)
	{
		if (SweepRad <= 0.f)
		{
			return false;
		}
		return WrapTau(Angle - StartRad) < SweepRad;
	}

	float RailInnerRadiusCm(const FPlan& Plan, const float ZCm)
	{
		return FMath::Max(InteriorRadiusCm(Plan, ZCm) - Plan.StairWidthCm, 0.f);
	}

	void CollectFlightRail(
		const FPlan& Plan,
		const int32 Floor,
		TArray<SolidStronghold::FSolidHandrail::FPiece>& Out)
	{
		Out.Reset();
		const FWedge Flight = FlightWedge(Plan, Floor);
		if (!Flight.bOpen)
		{
			return;
		}
		const float FloorZ = FloorZCm(Plan, Floor);
		const float Climb = static_cast<float>(Plan.StairTreadCount) * RiserCm(Plan);
		SolidStronghold::FSolidHandrail Rail;
		Rail.CollectArc(
			Flight.StartRad,
			Flight.SweepRad,
			RailInnerRadiusCm(Plan, FloorZ),
			[FloorZ, Climb](const float Along)
			{
				return FloorZ + Along * Climb;
			});
		Out = Rail.Pieces();
	}

	void CollectHoleRail(
		const FPlan& Plan,
		const int32 Floor,
		TArray<SolidStronghold::FSolidHandrail::FPiece>& Out)
	{
		Out.Reset();
		const FWedge Gap = GapWedge(Plan, Floor);
		if (!Gap.bOpen)
		{
			return;
		}
		const float FloorZ = FloorZCm(Plan, Floor);
		const float Inner = RailInnerRadiusCm(Plan, FloorZ);
		const float Outer = InteriorRadiusCm(Plan, FloorZ);
		SolidStronghold::FSolidHandrail Rail;
		Rail.CollectArc(
			Gap.StartRad,
			Gap.SweepRad,
			Inner,
			[FloorZ](const float Along)
			{
				return FloorZ + Along * 0.f;
			});
		const float OuterPost = Outer - SolidStronghold::FSolidHandrail::BarCm * 0.5f;
		if (OuterPost > Inner + SolidStronghold::FSolidHandrail::BarCm)
		{
			const float Angle = Gap.StartRad;
			Rail.AddBar(
				SolidStronghold::FSolidHandrail::Point(Angle, Inner, FloorZ + SolidStronghold::FSolidHandrail::HeightCm),
				SolidStronghold::FSolidHandrail::Point(Angle, OuterPost, FloorZ + SolidStronghold::FSolidHandrail::HeightCm),
				true);
			Rail.AddPost(
				SolidStronghold::FSolidHandrail::Point(Angle, OuterPost, FloorZ),
				FMath::RadiansToDegrees(Angle),
				true);
		}
		Out = Rail.Pieces();
	}

	bool CenterMeetsWedge(
		const FPlan& Plan,
		const FVector2D& Center,
		const float InteriorRadius,
		const FWedge& Wedge)
	{
		if (!Wedge.bOpen)
		{
			return false;
		}
		if (!AngleInSweep(FMath::Atan2(Center.Y, Center.X), Wedge.StartRad, Wedge.SweepRad))
		{
			return false;
		}
		const float Distance = Center.Size();
		const float Reach = SolidStrongholdGrid::SideCm();
		return Distance + Reach > InteriorRadius - Plan.StairWidthCm
			&& Distance - Reach < InteriorRadius;
	}

	FVector2D GridNudgeCm(const FPlan& Plan, const int32 Floor)
	{
		if (!IsFloor(Plan, Floor))
		{
			return FVector2D::ZeroVector;
		}
		const FPlan* Defined[] = { &Average, &Impressive, &Grand, &Stately, &Imperious };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Defined); ++Index)
		{
			const FPlan& Known = *Defined[Index];
			if (Plan.FloorCount == Known.FloorCount
				&& FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, Known.GroundExteriorRadiusCm, 0.5f)
				&& FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, Known.TopExteriorRadiusCm, 0.5f))
			{
				return HexGridNudge[Index].Shift[Floor];
			}
		}
		return FVector2D::ZeroVector;
	}

	FVector2D CellCenter(const int32 Col, const int32 Row, const FVector2D& Origin)
	{
		return SolidStrongholdGrid::HexCenter(Col, Row, Origin);
	}

	FVector2D CellCenter(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return CellCenter(Col, Row, GridNudgeCm(Plan, Floor));
	}

	int32 HexSpan(const FPlan& Plan)
	{
		const float Radius = FMath::Max(GroundInteriorRadiusCm(Plan), TopInteriorRadiusCm(Plan));
		const float Reach = Radius + SolidStrongholdGrid::SideCm();
		const int32 Cols = FMath::CeilToInt(Reach / SolidStrongholdGrid::FlatToFlatCm) + 1;
		const float RowStep = FMath::Max(SolidStrongholdGrid::RowStepCm(), 1.f);
		const int32 Rows = FMath::CeilToInt(Reach / RowStep) + 1;
		return FMath::Max(Cols, Rows);
	}

	bool HexFitsFloor(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin)
	{
		if (!IsFloor(Plan, Floor))
		{
			return false;
		}
		const FVector2D Center = CellCenter(Col, Row, Origin);
		FVector2D Vertices[6];
		SolidStrongholdGrid::HexVertices(Center, Vertices);
		const float Radius = InteriorRadiusCm(Plan, FloorZCm(Plan, Floor));
		for (const FVector2D& Vertex : Vertices)
		{
			if (Vertex.Size() > Radius + 0.01f)
			{
				return false;
			}
		}
		return true;
	}

	bool HexFitsFloor(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexFitsFloor(Plan, Floor, Col, Row, GridNudgeCm(Plan, Floor));
	}

	bool HexOnStair(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin)
	{
		if (!HexFitsFloor(Plan, Floor, Col, Row, Origin))
		{
			return false;
		}
		return CenterMeetsWedge(
			Plan,
			CellCenter(Col, Row, Origin),
			InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)),
			FlightWedge(Plan, Floor));
	}

	bool HexOnStair(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexOnStair(Plan, Floor, Col, Row, GridNudgeCm(Plan, Floor));
	}

	bool HexOnGap(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin)
	{
		if (!HexFitsFloor(Plan, Floor, Col, Row, Origin))
		{
			return false;
		}
		return CenterMeetsWedge(
			Plan,
			CellCenter(Col, Row, Origin),
			InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)),
			GapWedge(Plan, Floor));
	}

	bool HexOnGap(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexOnGap(Plan, Floor, Col, Row, GridNudgeCm(Plan, Floor));
	}

	bool HexOnLanding(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin)
	{
		if (!HexFitsFloor(Plan, Floor, Col, Row, Origin))
		{
			return false;
		}
		return CenterMeetsWedge(
			Plan,
			CellCenter(Col, Row, Origin),
			InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)),
			LandingWedge(Plan, Floor));
	}

	bool HexOnLanding(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexOnLanding(Plan, Floor, Col, Row, GridNudgeCm(Plan, Floor));
	}

	bool HexKept(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin)
	{
		return HexFitsFloor(Plan, Floor, Col, Row, Origin)
			&& !HexOnStair(Plan, Floor, Col, Row, Origin)
			&& !HexOnGap(Plan, Floor, Col, Row, Origin)
			&& !HexOnLanding(Plan, Floor, Col, Row, Origin);
	}

	bool HexKept(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexKept(Plan, Floor, Col, Row, GridNudgeCm(Plan, Floor));
	}

	void CollectKeptHexes(
		const FPlan& Plan,
		const int32 Floor,
		TArray<SolidStrongholdGrid::FHex>& OutHexes,
		const FVector2D& Origin)
	{
		OutHexes.Reset();
		if (!IsFloor(Plan, Floor))
		{
			return;
		}
		const int32 Span = HexSpan(Plan);
		for (int32 Row = -Span; Row <= Span; ++Row)
		{
			for (int32 Col = -Span; Col <= Span; ++Col)
			{
				if (!HexKept(Plan, Floor, Col, Row, Origin))
				{
					continue;
				}
				SolidStrongholdGrid::FHex Hex;
				Hex.Center = CellCenter(Col, Row, Origin);
				Hex.Col = Col;
				Hex.Row = Row;
				OutHexes.Add(Hex);
			}
		}
	}

	void CollectKeptHexes(const FPlan& Plan, const int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutHexes)
	{
		CollectKeptHexes(Plan, Floor, OutHexes, GridNudgeCm(Plan, Floor));
	}
}
