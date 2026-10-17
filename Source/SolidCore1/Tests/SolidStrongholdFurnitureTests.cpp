#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonInterior.h"
#include "SolidFurnishing.h"
#include "SolidStrongholdFurniture.h"
#include "SolidStrongholdGrid.h"
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
	TestEqual(TEXT("fireplace is one hex"), SolidStrongholdFurniture::HexCount(SolidStrongholdFurniture::EKind::Fireplace), 1);
	TestEqual(TEXT("bed is two hexes"), SolidStrongholdFurniture::HexCount(SolidStrongholdFurniture::EKind::Bed), 2);
	TestEqual(TEXT("chest is one hex"), SolidStrongholdFurniture::HexCount(SolidStrongholdFurniture::EKind::Chest), 1);

	const float Flat = SolidStrongholdGrid::FlatToFlatCm;
	const float Across = SolidStrongholdGrid::VertexToVertexCm();
	TestTrue(TEXT("fireplace sits inside one hex"), SolidStrongholdFurniture::FireplaceAcrossCm() < Flat);
	TestTrue(TEXT("fireplace still fills most of the hex"), SolidStrongholdFurniture::FireplaceAcrossCm() > Flat * 0.7f);
	TestTrue(
		TEXT("embers are a low disc"),
		SolidStrongholdFurniture::FireplaceEmberHeightCm() < SolidStrongholdFurniture::FireplaceHearthCm());
	TestTrue(
		TEXT("embers lost about 60 percent of their height"),
		FMath::IsNearlyEqual(SolidStrongholdFurniture::FireplaceEmberHeightCm(), 8.f, 0.01f));
	TestTrue(
		TEXT("embers sit inside the hearth"),
		SolidStrongholdFurniture::FireplaceEmberRadiusCm() * 2.f < SolidStrongholdFurniture::FireplaceAcrossCm());
	TestTrue(
		TEXT("embers cover most of the hearth"),
		SolidStrongholdFurniture::FireplaceEmberRadiusCm() * 2.f > SolidStrongholdFurniture::FireplaceAcrossCm() * 0.7f);

	TestTrue(TEXT("bed is longer than one hex"), SolidStrongholdFurniture::BedLengthCm() > Flat);
	TestTrue(TEXT("bed fits in two hexes"), SolidStrongholdFurniture::BedLengthCm() <= Flat * 2.f);
	TestTrue(TEXT("bed is narrower than a hex"), SolidStrongholdFurniture::BedWidthCm() < Across);
	TestTrue(
		TEXT("bed passes through the shared flat"),
		SolidStrongholdFurniture::BedWidthCm() < SolidStrongholdGrid::SideCm());
	TestTrue(TEXT("pillow is shorter than the bed"), SolidStrongholdFurniture::PillowLengthCm() < SolidStrongholdFurniture::BedLengthCm());
	TestTrue(TEXT("pillow is narrower than the bed"), SolidStrongholdFurniture::PillowWidthCm() < SolidStrongholdFurniture::BedWidthCm());

	TestTrue(TEXT("chest length fits in one hex"), SolidStrongholdFurniture::ChestLengthCm() < Flat);
	TestTrue(TEXT("chest width fits in one hex"), SolidStrongholdFurniture::ChestWidthCm() < Flat);
	TestTrue(
		TEXT("lid matches the chest width"),
		FMath::IsNearlyEqual(SolidStrongholdFurniture::ChestLidRadiusCm() * 2.f, SolidStrongholdFurniture::ChestWidthCm(), 0.01f));

	const FLinearColor Stone = SolidStrongholdFurniture::StoneColor();
	const FLinearColor Ember = SolidStrongholdFurniture::EmberColor();
	const FLinearColor Beige = SolidStrongholdFurniture::BeigeColor();
	const FLinearColor Pillow = SolidStrongholdFurniture::PillowColor();
	TestTrue(TEXT("hearth is grey"), FMath::Abs(Stone.R - Stone.G) < 0.05f && Stone.R > 0.3f && Stone.R < 0.6f);
	TestTrue(TEXT("embers are red"), Ember.R > 0.5f && Ember.R > Ember.G * 2.f && Ember.R > Ember.B * 2.f);
	TestTrue(TEXT("wood is beige"), Beige.R > Beige.G && Beige.G > Beige.B && Beige.B > 0.3f);
	TestTrue(TEXT("pillow is white"), Pillow.R > 0.9f && Pillow.G > 0.9f && Pillow.B > 0.85f);

	TestEqual(TEXT("catalog has the three pieces"), SolidStrongholdFurniture::CatalogCount, 3);
	TestEqual(TEXT("fireplace name"), FString(SolidStrongholdFurniture::Catalog[0].Name), FString(TEXT("Fireplace")));
	TestEqual(TEXT("bed name"), FString(SolidStrongholdFurniture::Catalog[1].Name), FString(TEXT("Bed")));
	TestEqual(TEXT("chest name"), FString(SolidStrongholdFurniture::Catalog[2].Name), FString(TEXT("Chest")));
	TestTrue(TEXT("fireplace kind"), SolidStrongholdFurniture::Catalog[0].Kind == SolidStrongholdFurniture::EKind::Fireplace);
	TestTrue(TEXT("bed kind"), SolidStrongholdFurniture::Catalog[1].Kind == SolidStrongholdFurniture::EKind::Bed);
	TestTrue(TEXT("chest kind"), SolidStrongholdFurniture::Catalog[2].Kind == SolidStrongholdFurniture::EKind::Chest);
	TestFalse(TEXT("the fireplace is not storage"), SolidStrongholdFurniture::IsStorage(&SolidStrongholdFurniture::Catalog[0]));
	TestFalse(TEXT("the bed is not storage"), SolidStrongholdFurniture::IsStorage(&SolidStrongholdFurniture::Catalog[1]));
	TestTrue(TEXT("the chest is storage"), SolidStrongholdFurniture::IsStorage(&SolidStrongholdFurniture::Catalog[2]));
	TestFalse(TEXT("a missing piece is not storage"), SolidStrongholdFurniture::IsStorage(nullptr));
	for (int32 Index = 0; Index < SolidStrongholdFurniture::CatalogCount; ++Index)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Index);
		TestTrue(TEXT("catalog link is the item itself"), Item == &SolidStrongholdFurniture::Catalog[Index]);
		TestEqual(
			TEXT("kind finds its catalog index"),
			SolidStrongholdFurniture::IndexOfKind(SolidStrongholdFurniture::Catalog[Index].Kind),
			Index);
	}
	TestTrue(TEXT("a missing index has no item"), SolidStrongholdFurniture::ItemAt(-1) == nullptr);

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
	SolidStrongholdFurniture::UsedFurnishings(SolidStronghold::Starting, Placements);
	TestEqual(TEXT("three starter pieces"), Placements.Num(), 3);
	if (Placements.Num() != 3)
	{
		return false;
	}
	const SolidStrongholdFurniture::FItem* First = SolidStrongholdFurniture::ItemAt(Placements[0].ItemIndex);
	const SolidStrongholdFurniture::FItem* Second = SolidStrongholdFurniture::ItemAt(Placements[1].ItemIndex);
	const SolidStrongholdFurniture::FItem* Third = SolidStrongholdFurniture::ItemAt(Placements[2].ItemIndex);
	TestTrue(TEXT("first links to the fireplace"), First && First->Kind == SolidStrongholdFurniture::EKind::Fireplace);
	TestTrue(TEXT("second links to the bed"), Second && Second->Kind == SolidStrongholdFurniture::EKind::Bed);
	TestTrue(TEXT("third links to the chest"), Third && Third->Kind == SolidStrongholdFurniture::EKind::Chest);
	TestTrue(TEXT("the cave chest is the storage piece"), SolidStrongholdFurniture::IsStorage(Third));

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidStrongholdGrid::CollectMeagreHexes(Hexes);

	auto CellKey = [](const int32 Col, const int32 Row) -> int64
	{
		return (static_cast<int64>(Col) << 32) | static_cast<uint32>(Row);
	};
	TSet<int64> UsedCells;
	const SolidStrongholdGrid::FCell Captain = SolidStrongholdGrid::MeagreCaptainCell();
	const SolidStrongholdGrid::FCell Entrance = SolidStrongholdGrid::MeagreEntranceCell();
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
	auto BuildPiece = [this](const SolidStrongholdFurniture::EKind Kind) -> ASolidFurnishing*
	{
		ASolidFurnishing* Piece = NewObject<ASolidFurnishing>();
		TestNotNull(TEXT("furnishing"), Piece);
		if (!Piece)
		{
			return nullptr;
		}
		Piece->SetKind(Kind);
		TestTrue(TEXT("kind sticks"), Piece->GetKind() == Kind);
		Piece->BuildVisuals();
		return Piece;
	};

	ASolidFurnishing* Fireplace = BuildPiece(SolidStrongholdFurniture::EKind::Fireplace);
	ASolidFurnishing* Bed = BuildPiece(SolidStrongholdFurniture::EKind::Bed);
	ASolidFurnishing* Chest = BuildPiece(SolidStrongholdFurniture::EKind::Chest);
	if (!Fireplace || !Bed || !Chest)
	{
		return false;
	}

	UStaticMeshComponent* Hearth = Fireplace->GetBodyMesh();
	UStaticMeshComponent* Embers = Fireplace->GetTopMesh();
	UStaticMeshComponent* BedBody = Bed->GetBodyMesh();
	UStaticMeshComponent* Pillow = Bed->GetTopMesh();
	UStaticMeshComponent* ChestBody = Chest->GetBodyMesh();
	UStaticMeshComponent* Lid = Chest->GetTopMesh();
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
			SolidStrongholdFurniture::FireplaceAcrossCm(),
			SolidStrongholdFurniture::FireplaceAcrossCm(),
			SolidStrongholdFurniture::FireplaceHearthCm())));
	TestTrue(
		TEXT("embers are a short cylinder"),
		ScaleMatches(Embers, FVector(
			SolidStrongholdFurniture::FireplaceEmberRadiusCm() * 2.f,
			SolidStrongholdFurniture::FireplaceEmberRadiusCm() * 2.f,
			SolidStrongholdFurniture::FireplaceEmberHeightCm())));
	TestTrue(TEXT("embers sit on the hearth"), Embers->GetRelativeLocation().Z > Hearth->GetRelativeLocation().Z);

	TestTrue(
		TEXT("bed scale covers two hexes"),
		ScaleMatches(BedBody, FVector(
			SolidStrongholdFurniture::BedLengthCm(),
			SolidStrongholdFurniture::BedWidthCm(),
			SolidStrongholdFurniture::BedHeightCm())));
	TestTrue(
		TEXT("pillow scale"),
		ScaleMatches(Pillow, FVector(
			SolidStrongholdFurniture::PillowLengthCm(),
			SolidStrongholdFurniture::PillowWidthCm(),
			SolidStrongholdFurniture::PillowHeightCm())));
	TestTrue(TEXT("pillow sits on the bed"), Pillow->GetRelativeLocation().Z > BedBody->GetRelativeLocation().Z);

	TestTrue(
		TEXT("chest body scale"),
		ScaleMatches(ChestBody, FVector(
			SolidStrongholdFurniture::ChestLengthCm(),
			SolidStrongholdFurniture::ChestWidthCm(),
			SolidStrongholdFurniture::ChestBodyHeightCm())));
	TestTrue(
		TEXT("lid scale is a cylinder along the chest"),
		ScaleMatches(Lid, FVector(
			SolidStrongholdFurniture::ChestLidRadiusCm() * 2.f,
			SolidStrongholdFurniture::ChestLidRadiusCm() * 2.f,
			SolidStrongholdFurniture::ChestLengthCm())));
	TestTrue(
		TEXT("lid axis runs along the chest"),
		Lid->GetRelativeRotation().Quaternion().GetAxisZ().Equals(FVector::ForwardVector, 0.02f));
	TestTrue(
		TEXT("lid is centered on the top of the chest"),
		FMath::IsNearlyEqual(
			Lid->GetRelativeLocation().Z,
			ChestBody->GetRelativeLocation().Z + SolidStrongholdFurniture::ChestBodyHeightCm() * 0.5f,
			0.05f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
