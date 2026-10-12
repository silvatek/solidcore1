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
	const SolidStrongholdGrid::FHex* FindStarterHex(
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const int32 Col,
		const int32 Row)
	{
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			if (Hex.Col == Col && Hex.Row == Row)
			{
				return &Hex;
			}
		}
		return nullptr;
	}

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

	TArray<SolidStrongholdFurniture::FPlacement> Placements;
	SolidStrongholdFurniture::MeagreStarter(Placements);
	TestEqual(TEXT("three starter pieces"), Placements.Num(), 3);
	if (Placements.Num() != 3)
	{
		return false;
	}
	TestTrue(TEXT("first is the fireplace"), Placements[0].Kind == SolidStrongholdFurniture::EKind::Fireplace);
	TestTrue(TEXT("second is the bed"), Placements[1].Kind == SolidStrongholdFurniture::EKind::Bed);
	TestTrue(TEXT("third is the chest"), Placements[2].Kind == SolidStrongholdFurniture::EKind::Chest);

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidStrongholdGrid::CollectMeagreHexes(Hexes);

	auto CellKey = [](const int32 Col, const int32 Row) -> int64
	{
		return (static_cast<int64>(Col) << 32) | static_cast<uint32>(Row);
	};
	TSet<int64> Used;
	for (const SolidStrongholdFurniture::FPlacement& Placement : Placements)
	{
		const SolidStrongholdGrid::FHex* Anchor = FindStarterHex(Hexes, Placement.Col, Placement.Row);
		const SolidStrongholdGrid::FHex* Span = FindStarterHex(Hexes, Placement.SpanCol, Placement.SpanRow);
		TestTrue(TEXT("anchor cell is on the floor"), Anchor != nullptr);
		TestTrue(TEXT("span cell is on the floor"), Span != nullptr);
		if (!Anchor || !Span)
		{
			return false;
		}
		const SolidStrongholdGrid::FCell Captain = SolidStrongholdGrid::MeagreCaptainCell();
		TestFalse(
			TEXT("pieces stay off the captain's cell"),
			(Anchor->Col == Captain.Col && Anchor->Row == Captain.Row)
				|| (Span->Col == Captain.Col && Span->Row == Captain.Row));

		const int32 Cells = SolidStrongholdFurniture::HexCount(Placement.Kind);
		if (Cells == 1)
		{
			TestEqual(TEXT("one-hex piece uses one cell"), Placement.Col, Placement.SpanCol);
			TestEqual(TEXT("one-hex piece uses one row"), Placement.Row, Placement.SpanRow);
		}
		else
		{
			TestTrue(
				TEXT("bed cells share an edge"),
				FMath::IsNearlyEqual(FVector2D::Distance(Anchor->Center, Span->Center), Flat, 0.05f));
			TestEqual(TEXT("bed stays on one row"), Placement.Row, Placement.SpanRow);
			TestEqual(TEXT("bed covers the next column"), FMath::Abs(Placement.SpanCol - Placement.Col), 1);
		}

		const int64 AnchorKey = CellKey(Placement.Col, Placement.Row);
		const int64 SpanKey = CellKey(Placement.SpanCol, Placement.SpanRow);
		TestFalse(TEXT("pieces do not share the anchor"), Used.Contains(AnchorKey));
		Used.Add(AnchorKey);
		if (SpanKey != AnchorKey)
		{
			TestFalse(TEXT("pieces do not share the span"), Used.Contains(SpanKey));
			Used.Add(SpanKey);
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
