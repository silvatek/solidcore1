#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidWallgate.h"
#include "SolidTownBuildings.h"
#include "SolidTownSign.h"
#include "SolidTownSigns.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

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
	TArray<FSolidWorldLocation> Locations;
	WorldMap->CollectLocations(Locations);
	for (const FSolidWorldLocation& Location : Locations)
	{
		if (SolidTowns::NameFor(Location.Id).IsEmpty())
		{
			continue;
		}
		++NamedTowns;
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
		TestTrue(TEXT("cell projects"), WorldMap->CellToWorldXY(Placement.Cell, WorldMin, WorldMax, CellXY));
		const SolidTowns::FTownInfo* Town = SolidTowns::Find(Placement.LocationId);
		TestNotNull(TEXT("town"), Town);
		if (Town)
		{
			FVector2D Middle = CellXY;
			FSolidWorldLocation TownLocation;
			if (WorldMap->FindLocation(Town->Id, TownLocation) && TownLocation.Cells.Num() > 0)
			{
				FVector2D Sum = FVector2D::ZeroVector;
				int32 Found = 0;
				for (const FIntPoint& TownCell : TownLocation.Cells)
				{
					FVector2D TownXY = FVector2D::ZeroVector;
					if (WorldMap->CellToWorldXY(TownCell, WorldMin, WorldMax, TownXY))
					{
						Sum += TownXY;
						++Found;
					}
				}
				if (Found > 0)
				{
					Middle = Sum / static_cast<float>(Found);
				}
			}
			FVector2D Anchor = Middle;
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
	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (Row == 1 && Col == 2)
			{
				Line.AppendChar(USolidWorldChart::LocationMarker);
			}
			else if (Row == 3 && Col == 5)
			{
				Line.AppendChar(TEXT('9'));
			}
			else if ((Row == 3 && Col == 9) || (Row == 8 && Col == 9))
			{
				Line.AppendChar(USolidWorldChart::LocationMarker);
			}
			else
			{
				Line.AppendChar(USolidWorldChart::GrassMarker);
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("! 2,3 = Town Iglin\n");
	Text += TEXT("! 4,10 = Town Kanfold\n");
	Text += TEXT("! 9,10 = Town Kanfold\n");

	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	TestTrue(TEXT("synthetic loads"), WorldMap->LoadFromString(Text));

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
	FVector2D KanfoldNorth = FVector2D::ZeroVector;
	FVector2D KanfoldSouth = FVector2D::ZeroVector;
	TestTrue(TEXT("north Kanfold cell"), WorldMap->CellToWorldXY(FIntPoint(9, 3), WorldMin, WorldMax, KanfoldNorth));
	TestTrue(TEXT("south Kanfold cell"), WorldMap->CellToWorldXY(FIntPoint(9, 8), WorldMin, WorldMax, KanfoldSouth));
	TestTrue(TEXT("Kanfold sign is centered between its cells"),
		Placements[1].WorldXY.Equals((KanfoldNorth + KanfoldSouth) * 0.5f, 0.1f));

	FVector2D IglinXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Iglin cell"), WorldMap->CellToWorldXY(FIntPoint(2, 1), WorldMin, WorldMax, IglinXY));
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
			TestTrue(TEXT("wall-gate sign is east of the gate"),
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

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			Line.AppendChar((Row == 9 && Col == 16) ? USolidWorldChart::LocationMarker : USolidWorldChart::GrassMarker);
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("! 10,17 = Town Dragongate\n");

	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	TestTrue(TEXT("synthetic loads"), WorldMap->LoadFromString(Text));
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
	TestTrue(TEXT("cell projects"), WorldMap->CellToWorldXY(FIntPoint(16, 9), WorldMin, WorldMax, CellXY));
	const float CellNorth = (WorldMax.Y - WorldMin.Y) / static_cast<float>(USolidWorldChart::MapHeight) * 0.5f;
	const FVector2D Expected = CellXY + FVector2D(SolidTowns::DragongateSignEastCm, CellNorth);
	TestTrue(TEXT("sign stands east of the gate on the north edge"), Placements[0].WorldXY.Equals(Expected, 0.1f));
	TestTrue(TEXT("sign is north of the cell center"), Placements[0].WorldXY.Y > CellXY.Y + TowerNorthCm);
	TestTrue(TEXT("sign is east of the cell center"), Placements[0].WorldXY.X > CellXY.X);
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
	Params.MaxBuildings = 64;
	Params.MinSeparationCm = 50.f;
	Params.ClearRadiusAroundTownCenterCm = 450.f;

	TArray<SolidTownBuildings::FPlacement> Open;
	TestTrue(TEXT("packed some buildings"),
		SolidTownBuildings::CollectPlacements(Map, 77, Params, Open) > 0);
	if (Open.Num() == 0)
	{
		return false;
	}

	const FVector2D SignXY = Open[0].CenterXY;
	TArray<FVector2D> ExtraClear;
	ExtraClear.Add(SignXY);

	TArray<SolidTownBuildings::FPlacement> Cleared;
	SolidTownBuildings::CollectPlacements(Map, 77, Params, Cleared, ExtraClear);
	for (const SolidTownBuildings::FPlacement& Placement : Cleared)
	{
		TestTrue(TEXT("buildings stay clear of the sign"),
			FVector2D::Distance(Placement.CenterXY, SignXY) + 0.5f >= Params.ClearRadiusAroundTownCenterCm);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
