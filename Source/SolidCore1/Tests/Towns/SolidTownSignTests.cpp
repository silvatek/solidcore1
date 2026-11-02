#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Terrain/SolidTerrainTestHelpers.h"
#include "SolidWallgate.h"
#include "SolidTownBuildings.h"
#include "SolidTownSign.h"
#include "SolidTownSigns.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool TownSignCellWorld(
		const USolidWorldChart& Chart,
		const FIntPoint Cell,
		const FVector2D& WorldMin,
		const FVector2D& WorldMax,
		FVector2D& OutWorld)
	{
		int32 X = 0;
		int32 Y = 0;
		USolidWorldChart::PointUnderCell(Cell.X, Cell.Y, X, Y);
		return Chart.TerrainPointToWorld(FIntPoint(X, Y), WorldMin, WorldMax, OutWorld);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownSignPlacementsTest,
	"SolidCore1.Vegetation.TownSignPlacements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownSignPlacementsTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("world map"), WorldMap);
	TestTrue(TEXT("LoadDefault"), WorldMap->LoadDefault());

	const FVector2D WorldMin(-51200.f, -51200.f);
	const FVector2D WorldMax(51200.f, 51200.f);

	int32 NamedTowns = 0;
	for (const SolidTowns::FTownInfo& Town : SolidTowns::Definitions)
	{
		FIntPoint Cell;
		if (WorldMap->TownCell(Town.Id, Cell))
		{
			++NamedTowns;
		}
	}

	TArray<SolidTownSigns::FPlacement> Placements;
	const int32 Count = SolidTownSigns::CollectPlacements(WorldMap, WorldMin, WorldMax, Placements);
	TestEqual(TEXT("one sign per named town"), Count, NamedTowns);
	TestEqual(TEXT("placements filled"), Placements.Num(), NamedTowns);
	if (Placements.Num() != NamedTowns || Placements.Num() == 0)
	{
		return false;
	}

	int32 KanfoldSigns = 0;
	bool bSawRathen = false;
	for (int32 Index = 0; Index < Placements.Num(); ++Index)
	{
		const SolidTownSigns::FPlacement& Placement = Placements[Index];
		if (Index > 0)
		{
			TestTrue(TEXT("location id order"), Placement.LocationId >= Placements[Index - 1].LocationId);
		}
		TestEqual(TEXT("town name"), Placement.Name, SolidTowns::NameFor(Placement.LocationId));
		TestEqual(
			TEXT("welcome label"),
			SolidTownSigns::MakeLabel(Placement.Name),
			FString::Printf(TEXT("%s\n%s"), SolidTownSigns::WelcomeLine, *Placement.Name));

		FVector2D CellXY = FVector2D::ZeroVector;
		TestTrue(TEXT("cell projects"), TownSignCellWorld(*WorldMap, Placement.Cell, WorldMin, WorldMax, CellXY));
		const SolidTowns::FTownInfo* Town = SolidTowns::Find(Placement.LocationId);
		TestNotNull(TEXT("town"), Town);
		if (Town)
		{
			FVector2D Anchor = CellXY;
			if (SolidTowns::IsWallGate(Town->Id))
			{
				FVector2D Gate = FVector2D::ZeroVector;
				TestTrue(
					TEXT("the wall gate sign uses its marker"),
					SolidWallgate::GateCenterXY(WorldMap, Town->Id, WorldMin, WorldMax, Gate));
				Anchor = Gate;
			}
			TestTrue(TEXT("placement is the town anchor plus the town offset"),
				Placement.WorldXY.Equals(Anchor + Town->SignOffsetCm, 0.1f));
			TestTrue(TEXT("placement uses the town yaw"),
				FMath::IsNearlyEqual(Placement.YawDeg, Town->SignYawDeg, 0.1f));
		}

		if (Placement.LocationId == 2)
		{
			++KanfoldSigns;
		}
		if (Placement.LocationId == 6 && Placement.Name == TEXT("Rathen"))
		{
			bSawRathen = true;
		}

		for (int32 Other = Index + 1; Other < Placements.Num(); ++Other)
		{
			TestTrue(TEXT("signs are not stacked"),
				FVector2D::DistSquared(Placement.WorldXY, Placements[Other].WorldXY) > 1.f);
		}
	}
	TestEqual(TEXT("Kanfold has one sign"), KanfoldSigns, 1);
	TestTrue(TEXT("Rathen has a sign"), bSawRathen);

	TestEqual(TEXT("null map yields none"),
		SolidTownSigns::CollectPlacements(nullptr, WorldMin, WorldMax, Placements), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownSignSkipsEmptyNameTest,
	"SolidCore1.Vegetation.TownSignSkipsEmptyName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownSignSkipsEmptyNameTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("chart"), WorldMap))
	{
		return false;
	}
	WorldMap->Init(ESolidBiome::Grassland);

	auto AddTown = [](USolidWorldChart& Chart, const int32 CellX, const int32 CellY, const TCHAR* Name)
	{
		FSolidWorldChartMark Mark;
		Mark.Kind = ESolidWorldChartMarkKind::Town;
		Mark.Name = Name;
		int32 TerrainX = 0;
		int32 TerrainY = 0;
		USolidWorldChart::PointUnderCell(CellX, CellY, TerrainX, TerrainY);
		Mark.TerrainPoint = FIntPoint(TerrainX, TerrainY);
		Chart.Marks.Add(Mark);
	};
	AddTown(*WorldMap, 2, 1, TEXT("Iglin"));
	AddTown(*WorldMap, 9, 3, TEXT("Kanfold"));

	const FVector2D WorldMin(0.f, 0.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	TArray<SolidTownSigns::FPlacement> Placements;
	TestEqual(TEXT("one sign for Iglin and one for Kanfold"),
		SolidTownSigns::CollectPlacements(WorldMap, WorldMin, WorldMax, Placements), 2);
	if (Placements.Num() < 2)
	{
		return false;
	}
	TestEqual(TEXT("first is Iglin"), Placements[0].Name, FString(TEXT("Iglin")));
	TestEqual(TEXT("second is Kanfold"), Placements[1].Name, FString(TEXT("Kanfold")));
	FVector2D KanfoldXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Kanfold cell"), TownSignCellWorld(*WorldMap, FIntPoint(9, 3), WorldMin, WorldMax, KanfoldXY));
	TestTrue(TEXT("Kanfold sign sits on its marker"),
		Placements[1].WorldXY.Equals(KanfoldXY, 0.1f));

	FVector2D IglinXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Iglin cell"), TownSignCellWorld(*WorldMap, FIntPoint(2, 1), WorldMin, WorldMax, IglinXY));
	TestTrue(TEXT("Iglin sign sits on its cell"), Placements[0].WorldXY.Equals(IglinXY, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownSignVisualsTest,
	"SolidCore1.Vegetation.TownSignVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownSignVisualsTest::RunTest(const FString& Parameters)
{
	ASolidTownSign* Sign = NewObject<ASolidTownSign>();
	TestNotNull(TEXT("sign"), Sign);
	Sign->SetTownName(TEXT("Iglin"));
	const FString IglinLabel = FString::Printf(TEXT("%s\nIglin"), SolidTownSigns::WelcomeLine);
	TestEqual(TEXT("label"), Sign->GetLabel(), IglinLabel);
	TestTrue(TEXT("the greeting is the first line"), Sign->GetLabel().StartsWith(FString(SolidTownSigns::WelcomeLine) + TEXT("\n")));
	TestTrue(TEXT("the town name is the second line"), Sign->GetLabel().EndsWith(TEXT("Iglin")));
	const FString LongName = SolidTownSigns::MakeLabel(TEXT("AVeryLongTownName"));
	TestTrue(TEXT("a longer name widens the board"),
		SolidTownSigns::BoardWidthCm(LongName) > SolidTownSigns::BoardWidthCm(IglinLabel));
	TestTrue(TEXT("sign faces south"), FMath::IsNearlyEqual(SolidTownSigns::FacingYawDeg, -90.f));
	UStaticMeshComponent* Pole = Sign->GetPoleMesh();
	UStaticMeshComponent* Board = Sign->GetBoardMesh();
	TestNotNull(TEXT("pole"), Pole);
	TestNotNull(TEXT("board"), Board);
	if (Pole)
	{
		TestEqual(
			TEXT("the pole does not block a pawn"),
			static_cast<int32>(Pole->GetCollisionResponseToChannel(ECC_Pawn)),
			static_cast<int32>(ECR_Ignore));
	}
	if (Board)
	{
		TestEqual(
			TEXT("the board does not block a pawn"),
			static_cast<int32>(Board->GetCollisionResponseToChannel(ECC_Pawn)),
			static_cast<int32>(ECR_Ignore));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownSignWallgateTest,
	"SolidCore1.Vegetation.TownSignDragongate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownSignWallgateTest::RunTest(const FString& Parameters)
{
	const float TowerNorthCm = SolidWallgate::WallThicknessCm * 0.5f
		+ SolidWallgate::TowerFootprintYCm
		- SolidWallgate::ConnectCm;
	const float HalfCellCm = 102400.f / static_cast<float>(USolidWorldChart::MapHeight) * 0.5f;
	TestTrue(TEXT("the sign clears the towers"), SolidTowns::DragongateSignNorthCm > TowerNorthCm);
	TestTrue(TEXT("the sign sits on the north edge of the cell"),
		FMath::IsNearlyEqual(SolidTowns::DragongateSignNorthCm, HalfCellCm, 0.1f));

	int32 WallGateCount = 0;
	for (const SolidTowns::FTownInfo& Town : SolidTowns::Definitions)
	{
		if (Town.Type == SolidTowns::ETownType::WallGate)
		{
			++WallGateCount;
			TestEqual(TEXT("the wall gate is Dragongate"), FString(Town.Name), FString(TEXT("Dragongate")));
			const float HalfBoardCm = SolidTownSigns::BoardWidthCm(SolidTownSigns::MakeLabel(Town.Name)) * 0.5f;
			TestTrue(TEXT("wall-gate sign is west of the gate"),
				Town.SignOffsetCm.X - HalfBoardCm > SolidWallgate::GapCm * 0.5f);
			TestTrue(TEXT("wall-gate sign stays on the north edge"),
				FMath::IsNearlyEqual(Town.SignOffsetCm.Y, SolidTowns::DragongateSignNorthCm, 0.1f)
				&& Town.SignOffsetCm.Y > TowerNorthCm);
			TestTrue(TEXT("wall-gate sign faces north"),
				FMath::IsNearlyEqual(Town.SignYawDeg, SolidTowns::SignFacesNorthYawDeg, 0.1f));
			TestTrue(TEXT("north is the opposite of south"),
				FMath::IsNearlyEqual(SolidTowns::SignFacesNorthYawDeg, SolidTowns::SignFacesSouthYawDeg + 180.f, 0.1f));
		}
		else
		{
			TestEqual(
				TEXT("other towns stay towns"),
				static_cast<uint8>(Town.Type),
				static_cast<uint8>(SolidTowns::ETownType::Town));
			TestTrue(TEXT("other signs stay on the cell"), Town.SignOffsetCm.IsNearlyZero());
			TestTrue(TEXT("other signs face south"),
				FMath::IsNearlyEqual(Town.SignYawDeg, SolidTownSigns::FacingYawDeg, 0.1f));
		}
	}
	TestEqual(TEXT("one wall gate"), WallGateCount, 1);

	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("chart"), WorldMap))
	{
		return false;
	}
	WorldMap->Init(ESolidBiome::Grassland);
	FSolidWorldChartMark GateMark;
	GateMark.Kind = ESolidWorldChartMarkKind::Town;
	GateMark.Name = TEXT("Dragongate");
	int32 GateTerrainX = 0;
	int32 GateTerrainY = 0;
	USolidWorldChart::PointUnderCell(16, 9, GateTerrainX, GateTerrainY);
	GateMark.TerrainPoint = FIntPoint(GateTerrainX, GateTerrainY);
	WorldMap->Marks.Add(GateMark);
	const FVector2D WorldMin(-51200.f, -51200.f);
	const FVector2D WorldMax(51200.f, 51200.f);
	TArray<SolidTownSigns::FPlacement> Placements;
	TestEqual(TEXT("one Dragongate sign"),
		SolidTownSigns::CollectPlacements(WorldMap, WorldMin, WorldMax, Placements), 1);
	if (Placements.Num() < 1)
	{
		return false;
	}

	FVector2D CellXY = FVector2D::ZeroVector;
	TestTrue(TEXT("cell projects"), TownSignCellWorld(*WorldMap, FIntPoint(16, 9), WorldMin, WorldMax, CellXY));
	const float CellNorth = (WorldMax.Y - WorldMin.Y) / static_cast<float>(USolidWorldChart::MapHeight) * 0.5f;
	const FVector2D Expected = CellXY + FVector2D(SolidTowns::DragongateSignWestCm, CellNorth);
	TestTrue(TEXT("sign stands west of the gate on the north edge"), Placements[0].WorldXY.Equals(Expected, 0.1f));
	TestTrue(TEXT("sign is north of the cell center"), Placements[0].WorldXY.Y > CellXY.Y + TowerNorthCm);
	TestTrue(TEXT("sign is west of the cell center"), Placements[0].WorldXY.X > CellXY.X);
	TestTrue(TEXT("spawn yaw faces north"),
		FMath::IsNearlyEqual(Placements[0].YawDeg, SolidTowns::SignFacesNorthYawDeg, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownSignBuildingClearTest,
	"SolidCore1.Vegetation.TownSignBuildingClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownSignBuildingClearTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("map"), Map);

	SolidTownBuildings::FScatterParams Params;
	Params.Density = 0.8f;
	Params.MinSeparationCm = 50.f;

	TArray<SolidTownBuildings::FPlacement> Open;
	TestTrue(TEXT("packed some buildings"),
		SolidTownBuildings::CollectPlacements(Map, 77, Params, Open) > 0);
	if (Open.Num() == 0)
	{
		return false;
	}

	SolidTownSigns::FPlacement Sign;
	Sign.Name = TEXT("Iglin");
	Sign.WorldXY = Open[0].CenterXY;
	Sign.YawDeg = SolidTownSigns::FacingYawDeg;
	TArray<SolidTownSigns::FPlacement> Signs;
	Signs.Add(Sign);
	const float HalfWidth = SolidTownSigns::BoardWidthCm(SolidTownSigns::MakeLabel(Sign.Name)) * 0.5f;
	const float HalfDepth = FMath::Max(SolidTownSigns::BoardDepthCm, SolidTownSigns::PoleThicknessCm) * 0.5f;

	TArray<SolidTownBuildings::FPlacement> Cleared;
	SolidTownBuildings::CollectPlacements(Map, 77, Params, Cleared, Signs);
	for (const SolidTownBuildings::FPlacement& Placement : Cleared)
	{
		const FVector2D Delta = Placement.CenterXY - Sign.WorldXY;
		const FVector Local = FRotator(0.f, -Sign.YawDeg, 0.f).RotateVector(FVector(Delta.X, Delta.Y, 0.f));
		TestTrue(TEXT("a building stays off the sign"),
			FMath::Abs(Local.X) > HalfDepth || FMath::Abs(Local.Y) > HalfWidth);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
