#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidTownBuildings.h"
#include "SolidTownSign.h"
#include "SolidTownSigns.h"
#include "SolidWorldMap.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTownSignPlacementsTest,
	"SolidCore1.Vegetation.TownSignPlacements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTownSignPlacementsTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* WorldMap = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("world map"), WorldMap);
	TestTrue(TEXT("LoadDefault"), WorldMap->LoadDefault());

	const FVector2D WorldMin(-51200.f, -51200.f);
	const FVector2D WorldMax(51200.f, 51200.f);

	TArray<SolidTownSigns::FPlacement> Placements;
	const int32 Count = SolidTownSigns::CollectPlacements(WorldMap, WorldMin, WorldMax, Placements);
	TestEqual(TEXT("six named cells"), Count, 6);
	TestEqual(TEXT("placements filled"), Placements.Num(), 6);
	if (Placements.Num() != 6)
	{
		return false;
	}

	const TCHAR* ExpectedNames[] = {
		TEXT("Iglin"), TEXT("Relion"), TEXT("Kanfold"), TEXT("Kanfold"), TEXT("Visolar"), TEXT("Jethan") };
	const int32 ExpectedIds[] = { 0, 1, 2, 2, 3, 4 };
	for (int32 Index = 0; Index < Placements.Num(); ++Index)
	{
		const SolidTownSigns::FPlacement& Placement = Placements[Index];
		TestEqual(TEXT("location id order"), Placement.LocationId, ExpectedIds[Index]);
		TestEqual(TEXT("town name"), Placement.Name, FString(ExpectedNames[Index]));
		TestEqual(
			TEXT("welcome label"),
			SolidTownSigns::MakeLabel(Placement.Name),
			FString::Printf(TEXT("Welcome to %s"), ExpectedNames[Index]));

		FVector2D CellXY = FVector2D::ZeroVector;
		TestTrue(TEXT("cell projects"), WorldMap->CellToWorldXY(Placement.Cell, WorldMin, WorldMax, CellXY));
		TestTrue(TEXT("placement is the cell center"), Placement.WorldXY.Equals(CellXY, 0.1f));

		for (int32 Other = Index + 1; Other < Placements.Num(); ++Other)
		{
			TestTrue(TEXT("signs are not stacked"),
				FVector2D::DistSquared(Placement.WorldXY, Placements[Other].WorldXY) > 1.f);
		}
	}

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
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 1 && Col == 2)
			{
				Line.AppendChar(TEXT('0'));
			}
			else if (Row == 3 && Col == 5)
			{
				Line.AppendChar(TEXT('9'));
			}
			else if ((Row == 3 && Col == 9) || (Row == 8 && Col == 9))
			{
				Line.AppendChar(TEXT('2'));
			}
			else
			{
				Line.AppendChar(TEXT('G'));
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("0 = Starting village, biome=Town\n");
	Text += TEXT("2 = Island city, biome=Town\n");

	USolidWorldMap* WorldMap = NewObject<USolidWorldMap>();
	TestTrue(TEXT("synthetic loads"), WorldMap->LoadFromString(Text));

	const FVector2D WorldMin(0.f, 0.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	TArray<SolidTownSigns::FPlacement> Placements;
	TestEqual(TEXT("index with no town definition is skipped"),
		SolidTownSigns::CollectPlacements(WorldMap, WorldMin, WorldMax, Placements), 3);
	if (Placements.Num() < 3)
	{
		return false;
	}
	TestEqual(TEXT("first is Iglin"), Placements[0].Name, FString(TEXT("Iglin")));
	TestEqual(TEXT("second Kanfold cell"), Placements[1].Name, FString(TEXT("Kanfold")));
	TestEqual(TEXT("third Kanfold cell"), Placements[2].Name, FString(TEXT("Kanfold")));
	TestTrue(TEXT("Kanfold cells stay apart"),
		FVector2D::Distance(Placements[1].WorldXY, Placements[2].WorldXY) > 100.f);

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
	TestEqual(TEXT("label"), Sign->GetLabel(), FString(TEXT("Welcome to Iglin")));
	Sign->BuildVisuals();

	UTextRenderComponent* Label = Sign->GetLabelText();
	TestNotNull(TEXT("white label"), Label);
	if (Label)
	{
		TestEqual(TEXT("white text"), Label->Text.ToString(), FString(TEXT("Welcome to Iglin")));
		TestEqual(TEXT("white color"), Label->TextRenderColor, FColor::White);
	}

	bool bSawBlack = false;
	for (int32 Index = 0; Index < ASolidTownSign::OutlineCount; ++Index)
	{
		UTextRenderComponent* Outline = Sign->GetOutlineText(Index);
		TestNotNull(TEXT("outline text"), Outline);
		if (!Outline)
		{
			continue;
		}
		TestEqual(TEXT("outline copies the label"), Outline->Text.ToString(), FString(TEXT("Welcome to Iglin")));
		TestEqual(TEXT("outline is black"), Outline->TextRenderColor, FColor::Black);
		if (Label)
		{
			const FVector Delta = Outline->GetRelativeLocation() - Label->GetRelativeLocation();
			TestTrue(TEXT("outline is offset from the white glyphs"),
				!FMath::IsNearlyZero(Delta.Y, 0.5f) || !FMath::IsNearlyZero(Delta.Z, 0.5f));
		}
		bSawBlack = true;
	}
	TestTrue(TEXT("black border present"), bSawBlack);

	UStaticMeshComponent* Pole = Sign->GetPoleMesh();
	UStaticMeshComponent* Board = Sign->GetBoardMesh();
	TestNotNull(TEXT("pole component"), Pole);
	TestNotNull(TEXT("board component"), Board);
	UStaticMesh* PoleMesh = Pole ? Pole->GetStaticMesh() : nullptr;
	UStaticMesh* BoardMesh = Board ? Board->GetStaticMesh() : nullptr;
	TestNotNull(TEXT("pole mesh"), PoleMesh);
	TestNotNull(TEXT("board mesh"), BoardMesh);
	if (Pole && Board)
	{
		TestTrue(TEXT("pole is thinner than the board is wide"),
			Pole->GetRelativeScale3D().X < Board->GetRelativeScale3D().Y);
		TestTrue(TEXT("board is a flat slab"),
			Board->GetRelativeScale3D().X < Board->GetRelativeScale3D().Y
			&& Board->GetRelativeScale3D().X < Board->GetRelativeScale3D().Z);
	}

	TestTrue(TEXT("sign faces south"), FMath::IsNearlyEqual(SolidTownSigns::FacingYawDeg, -90.f));
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
