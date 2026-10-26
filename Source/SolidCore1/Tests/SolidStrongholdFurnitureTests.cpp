#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/SolidGameState.h"
#include "Strongholds/Dungeon/SolidDungeonInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "Strongholds/Furnishings/Actors/SolidBed.h"
#include "Strongholds/Furnishings/Actors/SolidChest.h"
#include "Strongholds/Furnishings/Actors/SolidFireplace.h"
#include "Strongholds/Furnishings/Actors/SolidFurnishing.h"
#include "Strongholds/Furnishings/Actors/SolidSmallTable.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool ScaleMatches(const UStaticMeshComponent* Part, const FVector& SizeCm)
	{
		if (!Part)
		{
			return false;
		}
		const FVector Scale = Part->GetRelativeScale3D() * 100.f;
		return Scale.Equals(SizeCm, 0.05f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdFurnitureMeasureTest,
	"SolidCore1.Stronghold.Furniture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdFurnitureMeasureTest::RunTest(const FString& Parameters)
{
	const ASolidFireplace* Fire = NewObject<ASolidFireplace>();
	const ASolidBed* BedKind = NewObject<ASolidBed>();
	const ASolidChest* ChestKind = NewObject<ASolidChest>();
	const ASolidSmallTable* TableKind = NewObject<ASolidSmallTable>();
	TestNotNull(TEXT("fireplace actor"), Fire);
	TestNotNull(TEXT("bed actor"), BedKind);
	TestNotNull(TEXT("chest actor"), ChestKind);
	TestNotNull(TEXT("small table actor"), TableKind);
	if (!Fire || !BedKind || !ChestKind || !TableKind)
	{
		return false;
	}

	TestEqual(TEXT("fireplace is one hex"), Fire->HexCount(), 1);
	TestEqual(TEXT("bed is two hexes"), BedKind->HexCount(), 2);
	TestEqual(TEXT("chest is one hex"), ChestKind->HexCount(), 1);
	TestEqual(TEXT("small table is one hex"), TableKind->HexCount(), 1);

	const float Flat = SolidStrongholdGrid::FlatToFlatCm;
	const float Across = SolidStrongholdGrid::VertexToVertexCm();
	TestTrue(TEXT("fireplace sits inside one hex"), Fire->AcrossCm() < Flat);
	TestTrue(TEXT("fireplace still fills most of the hex"), Fire->AcrossCm() > Flat * 0.7f);
	TestTrue(
		TEXT("embers are a low disc"),
		Fire->EmberHeightCm() < Fire->HearthCm());
	TestTrue(
		TEXT("embers lost about 60 percent of their height"),
		FMath::IsNearlyEqual(Fire->EmberHeightCm(), 8.f, 0.01f));
	TestTrue(
		TEXT("embers sit inside the hearth"),
		Fire->EmberRadiusCm() * 2.f < Fire->AcrossCm());
	TestTrue(
		TEXT("embers cover most of the hearth"),
		Fire->EmberRadiusCm() * 2.f > Fire->AcrossCm() * 0.7f);

	TestTrue(TEXT("bed is longer than one hex"), BedKind->LengthCm() > Flat);
	TestTrue(TEXT("bed fits in two hexes"), BedKind->LengthCm() <= Flat * 2.f);
	TestTrue(TEXT("bed is narrower than a hex"), BedKind->WidthCm() < Across);
	TestTrue(
		TEXT("bed passes through the shared flat"),
		BedKind->WidthCm() < SolidStrongholdGrid::SideCm());
	TestTrue(TEXT("pillow is shorter than the bed"), BedKind->PillowLengthCm() < BedKind->LengthCm());
	TestTrue(TEXT("pillow is narrower than the bed"), BedKind->PillowWidthCm() < BedKind->WidthCm());

	TestTrue(TEXT("chest length fits in one hex"), ChestKind->LengthCm() < Flat);
	TestTrue(TEXT("chest width fits in one hex"), ChestKind->WidthCm() < Flat);
	TestTrue(
		TEXT("lid matches the chest width"),
		FMath::IsNearlyEqual(ChestKind->LidRadiusCm() * 2.f, ChestKind->WidthCm(), 0.01f));

	TestTrue(
		TEXT("small table top is 80 cm square"),
		FMath::IsNearlyEqual(TableKind->TopCm(), 80.f, 0.01f));
	TestTrue(
		TEXT("small table top is 8 cm thick"),
		FMath::IsNearlyEqual(TableKind->TopThicknessCm(), 8.f, 0.01f));
	TestTrue(
		TEXT("small table leg is an 8 cm post"),
		FMath::IsNearlyEqual(TableKind->LegCm(), 8.f, 0.01f));
	TestTrue(
		TEXT("small table leg is 62 cm tall"),
		FMath::IsNearlyEqual(TableKind->LegHeightCm(), 62.f, 0.01f));
	TestTrue(
		TEXT("small table leg is 12 cm in from the edge"),
		FMath::IsNearlyEqual(TableKind->LegInsetCm(), 12.f, 0.01f));
	TestTrue(
		TEXT("small table top stays inside one hex"),
		TableKind->TopCm() * FMath::Sqrt(2.f) < SolidStrongholdGrid::FlatToFlatCm);

	const FLinearColor Stone = Fire->StoneColor();
	const FLinearColor Ember = Fire->EmberColor();
	const FLinearColor Beige = BedKind->BeigeColor();
	const FLinearColor Pillow = BedKind->PillowColor();
	TestTrue(TEXT("hearth is light grey"), FMath::Abs(Stone.R - Stone.G) < 0.05f && Stone.R > 0.65f && Stone.R < 0.9f);
	TestTrue(TEXT("embers are red"), Ember.R > 0.5f && Ember.R > Ember.G * 2.f && Ember.R > Ember.B * 2.f);
	TestTrue(TEXT("wood is beige"), Beige.R > Beige.G && Beige.G > Beige.B && Beige.B > 0.3f);
	TestTrue(TEXT("pillow is white"), Pillow.R > 0.9f && Pillow.G > 0.9f && Pillow.B > 0.85f);

	const TArray<SolidStrongholdFurniture::FItem> Recipe = SolidStrongholdFurniture::StartingCatalog();
	TestEqual(TEXT("catalog has the four pieces"), Recipe.Num(), 4);
	if (Recipe.Num() != 4)
	{
		return false;
	}
	TestEqual(TEXT("fireplace name"), FString(Recipe[0].Name), FString(TEXT("Fireplace")));
	TestEqual(TEXT("bed name"), FString(Recipe[1].Name), FString(TEXT("Bed")));
	TestEqual(TEXT("chest name"), FString(Recipe[2].Name), FString(TEXT("Chest")));
	TestEqual(TEXT("small table name"), FString(Recipe[3].Name), FString(TEXT("Small Table")));
	TestTrue(TEXT("fireplace actor"), Recipe[0].ActorClass == ASolidFireplace::StaticClass());
	TestTrue(TEXT("bed actor"), Recipe[1].ActorClass == ASolidBed::StaticClass());
	TestTrue(TEXT("chest actor"), Recipe[2].ActorClass == ASolidChest::StaticClass());
	TestTrue(TEXT("small table actor"), Recipe[3].ActorClass == ASolidSmallTable::StaticClass());
	TestFalse(TEXT("the fireplace is not storage"), SolidStrongholdFurniture::IsStorage(&Recipe[0]));
	TestFalse(TEXT("the bed is not storage"), SolidStrongholdFurniture::IsStorage(&Recipe[1]));
	TestTrue(TEXT("the chest is storage"), SolidStrongholdFurniture::IsStorage(&Recipe[2]));
	TestFalse(TEXT("the small table is not storage"), SolidStrongholdFurniture::IsStorage(&Recipe[3]));
	TestEqual(TEXT("fireplace stops at one"), Recipe[0].Limit, 1);
	TestEqual(TEXT("bed stops at three"), Recipe[1].Limit, 3);
	TestEqual(TEXT("chest stops at one"), Recipe[2].Limit, 1);
	TestEqual(TEXT("small table stops at ten"), Recipe[3].Limit, 10);
	TestTrue(TEXT("a new game enables the four kinds"), Recipe[0].bEnabled && Recipe[1].bEnabled && Recipe[2].bEnabled && Recipe[3].bEnabled);
	TestFalse(TEXT("a missing piece is not storage"), SolidStrongholdFurniture::IsStorage(nullptr));
	for (int32 Index = 0; Index < Recipe.Num(); ++Index)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Index);
		TestTrue(TEXT("recipe row is stable"), Item == SolidStrongholdFurniture::ItemAt(Index));
		TestEqual(
			TEXT("actor finds its catalog index"),
			SolidStrongholdFurniture::IndexOfActor(Recipe[Index].ActorClass),
			Index);
	}
	TestTrue(TEXT("a missing index has no item"), SolidStrongholdFurniture::ItemAt(-1) == nullptr);

	USolidGameState* State = NewObject<USolidGameState>();
	TestNotNull(TEXT("play state"), State);
	if (!State)
	{
		return false;
	}
	TestEqual(TEXT("the session copies the recipe"), State->FurnishingCatalog.Num(), Recipe.Num());
	State->FurnishingCatalog[3].bEnabled = false;
	const TArray<SolidStrongholdFurniture::FItem> Fresh = SolidStrongholdFurniture::StartingCatalog();
	TestTrue(TEXT("the recipe stays enabled"), Fresh.Num() == 4 && Fresh[3].bEnabled);
	TestFalse(TEXT("the session can disable a kind"), State->FurnishingCatalog[3].bEnabled);

	FSolidStronghold MeagreFortress;
	MeagreFortress.Type = ESolidStrongholdType::Fortress;
	MeagreFortress.Size = ESolidStrongholdSize::Meagre;
	TArray<SolidStrongholdFurniture::FUsed> Empty;
	SolidStrongholdFurniture::UsedFurnishings(MeagreFortress, Empty);
	TestEqual(TEXT("a meagre fortress has no pieces yet"), Empty.Num(), 0);

	FSolidStronghold HumbleDungeon;
	HumbleDungeon.Type = ESolidStrongholdType::Dungeon;
	HumbleDungeon.Size = ESolidStrongholdSize::Humble;
	SolidStrongholdFurniture::UsedFurnishings(HumbleDungeon, Empty);
	TestEqual(TEXT("a humble dungeon has no pieces yet"), Empty.Num(), 0);

	TArray<SolidStrongholdFurniture::FUsed> Placements;
	SolidStrongholdFurniture::UsedFurnishings(
		FSolidStronghold(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre),
		Placements);
	TestEqual(TEXT("three starter pieces"), Placements.Num(), 3);
	if (Placements.Num() != 3)
	{
		return false;
	}
	const SolidStrongholdFurniture::FItem* First = SolidStrongholdFurniture::ItemAt(Placements[0].ItemIndex);
	const SolidStrongholdFurniture::FItem* Second = SolidStrongholdFurniture::ItemAt(Placements[1].ItemIndex);
	const SolidStrongholdFurniture::FItem* Third = SolidStrongholdFurniture::ItemAt(Placements[2].ItemIndex);
	TestTrue(TEXT("first links to the fireplace"), First && First->ActorClass == ASolidFireplace::StaticClass());
	TestTrue(TEXT("second links to the bed"), Second && Second->ActorClass == ASolidBed::StaticClass());
	TestTrue(TEXT("third links to the chest"), Third && Third->ActorClass == ASolidChest::StaticClass());
	TestTrue(TEXT("the cave chest is the storage piece"), SolidStrongholdFurniture::IsStorage(Third));

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidDungeonLevel::CollectMeagreHexes(Hexes);

	auto CellKey = [](const int32 Col, const int32 Row) -> int64
	{
		return (static_cast<int64>(Col) << 32) | static_cast<uint32>(Row);
	};
	TSet<int64> UsedCells;
	const SolidStrongholdGrid::FCell Captain = SolidDungeonLevel::MeagreCaptainCell();
	const SolidStrongholdGrid::FCell Entrance = SolidDungeonLevel::MeagreEntranceCell();
	for (const SolidStrongholdFurniture::FUsed& Placement : Placements)
	{
		TestEqual(TEXT("cave pieces sit on the single floor"), Placement.Level, 0);
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Placement.ItemIndex);
		TestTrue(TEXT("used piece links into the catalog"), Item != nullptr);
		if (!Item)
		{
			return false;
		}
		const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(Hexes, Placement.X, Placement.Y);
		const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(Hexes, Placement);
		TestTrue(TEXT("anchor cell is on the floor"), Anchor != nullptr);
		TestTrue(TEXT("span cell is on the floor"), Span != nullptr);
		if (!Anchor || !Span)
		{
			return false;
		}
		const bool bOnCaptain = (Anchor->Col == Captain.Col && Anchor->Row == Captain.Row)
			|| (Span->Col == Captain.Col && Span->Row == Captain.Row);
		const bool bOnEntrance = (Anchor->Col == Entrance.Col && Anchor->Row == Entrance.Row)
			|| (Span->Col == Entrance.Col && Span->Row == Entrance.Row);
		TestFalse(TEXT("pieces stay off the captain's cell"), bOnCaptain);
		TestFalse(TEXT("pieces stay off the entrance cell"), bOnEntrance);

		const int32 Cells = SolidStrongholdFurniture::HexCount(Item->Kind);
		if (Cells == 1)
		{
			TestEqual(TEXT("one-hex piece uses one cell"), Anchor->Col, Span->Col);
			TestEqual(TEXT("one-hex piece uses one row"), Anchor->Row, Span->Row);
		}
		else
		{
			TestTrue(
				TEXT("bed cells share an edge"),
				FMath::IsNearlyEqual(FVector2D::Distance(Anchor->Center, Span->Center), Flat, 0.05f));
			TestEqual(TEXT("bed stays on one row"), Anchor->Row, Span->Row);
			TestEqual(TEXT("bed covers the next column"), FMath::Abs(Span->Col - Anchor->Col), 1);
		}

		const int64 AnchorKey = CellKey(Anchor->Col, Anchor->Row);
		const int64 SpanKey = CellKey(Span->Col, Span->Row);
		TestFalse(TEXT("pieces do not share the anchor"), UsedCells.Contains(AnchorKey));
		UsedCells.Add(AnchorKey);
		if (SpanKey != AnchorKey)
		{
			TestFalse(TEXT("pieces do not share the span"), UsedCells.Contains(SpanKey));
			UsedCells.Add(SpanKey);
		}
	}

	ASolidDungeonInterior* Interior = NewObject<ASolidDungeonInterior>();
	TestNotNull(TEXT("interior"), Interior);
	if (Interior)
	{
		Interior->PlaceStarterFurnishings();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdFurnitureVisualTest,
	"SolidCore1.Stronghold.FurnitureVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdFurnitureVisualTest::RunTest(const FString& Parameters)
{
	ASolidFireplace* Fireplace = NewObject<ASolidFireplace>();
	ASolidBed* Bed = NewObject<ASolidBed>();
	ASolidChest* Chest = NewObject<ASolidChest>();
	ASolidSmallTable* Table = NewObject<ASolidSmallTable>();
	TestNotNull(TEXT("fireplace"), Fireplace);
	TestNotNull(TEXT("bed"), Bed);
	TestNotNull(TEXT("chest"), Chest);
	TestNotNull(TEXT("small table"), Table);
	if (!Fireplace || !Bed || !Chest || !Table)
	{
		return false;
	}
	Fireplace->BuildVisuals();
	Bed->BuildVisuals();
	Chest->BuildVisuals();
	Table->BuildVisuals();

	UStaticMeshComponent* Hearth = Fireplace->GetHearth();
	UStaticMeshComponent* Embers = Fireplace->GetEmbers();
	UStaticMeshComponent* BedBody = Bed->GetBedMesh();
	UStaticMeshComponent* Pillow = Bed->GetPillow();
	UStaticMeshComponent* ChestBody = Chest->GetChestMesh();
	UStaticMeshComponent* Lid = Chest->GetLid();
	TestNotNull(TEXT("hearth"), Hearth);
	TestNotNull(TEXT("embers"), Embers);
	TestNotNull(TEXT("bed"), BedBody);
	TestNotNull(TEXT("pillow"), Pillow);
	TestNotNull(TEXT("chest"), ChestBody);
	TestNotNull(TEXT("lid"), Lid);
	if (!Hearth || !Embers || !BedBody || !Pillow || !ChestBody || !Lid)
	{
		return false;
	}

	UStaticMesh* HearthMesh = Hearth->GetStaticMesh();
	UStaticMesh* EmberMesh = Embers->GetStaticMesh();
	UStaticMesh* BedMesh = BedBody->GetStaticMesh();
	UStaticMesh* PillowMesh = Pillow->GetStaticMesh();
	UStaticMesh* ChestMesh = ChestBody->GetStaticMesh();
	UStaticMesh* LidMesh = Lid->GetStaticMesh();
	TestNotNull(TEXT("hearth mesh"), HearthMesh);
	TestNotNull(TEXT("ember mesh"), EmberMesh);
	TestNotNull(TEXT("bed mesh"), BedMesh);
	TestNotNull(TEXT("pillow mesh"), PillowMesh);
	TestNotNull(TEXT("chest mesh"), ChestMesh);
	TestNotNull(TEXT("lid mesh"), LidMesh);
	TestNotNull(TEXT("hearth material"), Hearth->GetMaterial(0));
	TestNotNull(TEXT("pillow material"), Pillow->GetMaterial(0));

	TestTrue(
		TEXT("hearth scale is one hex across"),
		ScaleMatches(Hearth, FVector(
			Fireplace->AcrossCm(),
			Fireplace->AcrossCm(),
			Fireplace->HearthCm())));
	TestTrue(
		TEXT("embers are a short cylinder"),
		ScaleMatches(Embers, FVector(
			Fireplace->EmberRadiusCm() * 2.f,
			Fireplace->EmberRadiusCm() * 2.f,
			Fireplace->EmberHeightCm())));
	TestTrue(TEXT("embers sit on the hearth"), Embers->GetRelativeLocation().Z > Hearth->GetRelativeLocation().Z);

	TestTrue(
		TEXT("bed scale covers two hexes"),
		ScaleMatches(BedBody, FVector(
			Bed->LengthCm(),
			Bed->WidthCm(),
			Bed->HeightCm())));
	TestTrue(
		TEXT("pillow scale"),
		ScaleMatches(Pillow, FVector(
			Bed->PillowLengthCm(),
			Bed->PillowWidthCm(),
			Bed->PillowHeightCm())));
	TestTrue(TEXT("pillow sits on the bed"), Pillow->GetRelativeLocation().Z > BedBody->GetRelativeLocation().Z);

	TestTrue(
		TEXT("chest body scale"),
		ScaleMatches(ChestBody, FVector(
			Chest->LengthCm(),
			Chest->WidthCm(),
			Chest->BodyHeightCm())));
	TestTrue(
		TEXT("lid scale is a cylinder along the chest"),
		ScaleMatches(Lid, FVector(
			Chest->LidRadiusCm() * 2.f,
			Chest->LidRadiusCm() * 2.f,
			Chest->LengthCm())));
	TestTrue(
		TEXT("lid axis runs along the chest"),
		Lid->GetRelativeRotation().Quaternion().GetAxisZ().Equals(FVector::ForwardVector, 0.02f));
	TestTrue(
		TEXT("lid is centered on the top of the chest"),
		FMath::IsNearlyEqual(
			Lid->GetRelativeLocation().Z,
			ChestBody->GetRelativeLocation().Z + Chest->BodyHeightCm() * 0.5f,
			0.05f));

	UStaticMeshComponent* Slab = Table->GetTop();
	TestNotNull(TEXT("small table top"), Slab);
	if (!Slab)
	{
		return false;
	}
	TestTrue(TEXT("small table shows the top"), Slab->IsVisible());
	TestTrue(
		TEXT("small table top scale"),
		ScaleMatches(Slab, FVector(
			Table->TopCm(),
			Table->TopCm(),
			Table->TopThicknessCm())));
	const float Lift = SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm;
	const float Offset = Table->LegOffsetCm();
	const float LegZ = Lift + Table->LegHeightCm() * 0.5f;
	const FVector2D Corners[ASolidSmallTable::LegCount] = {
		FVector2D(Offset, Offset),
		FVector2D(Offset, -Offset),
		FVector2D(-Offset, Offset),
		FVector2D(-Offset, -Offset),
	};
	TestTrue(
		TEXT("small table top sits on the legs"),
		FMath::IsNearlyEqual(
			Slab->GetRelativeLocation().Z,
			Lift + Table->LegHeightCm() + Table->TopThicknessCm() * 0.5f,
			0.05f));
	TestTrue(TEXT("small table top is centered on the hex"), Slab->GetRelativeLocation().X == 0.f && Slab->GetRelativeLocation().Y == 0.f);
	for (int32 Index = 0; Index < ASolidSmallTable::LegCount; ++Index)
	{
		UStaticMeshComponent* Leg = Table->GetLeg(Index);
		TestNotNull(TEXT("small table leg"), Leg);
		if (!Leg)
		{
			return false;
		}
		TestTrue(TEXT("small table shows its legs"), Leg->IsVisible());
		UStaticMesh* LegMesh = Leg->GetStaticMesh();
		TestNotNull(TEXT("small table leg mesh"), LegMesh);
		TestTrue(
			TEXT("small table leg scale"),
			ScaleMatches(Leg, FVector(
				Table->LegCm(),
				Table->LegCm(),
				Table->LegHeightCm())));
		TestTrue(
			TEXT("small table leg stands on the floor"),
			Leg->GetRelativeLocation().Equals(FVector(Corners[Index].X, Corners[Index].Y, LegZ), 0.05f));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
