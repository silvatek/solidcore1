#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Dungeon/SolidDungeonInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "Strongholds/Furnishings/SolidArrange.h"
#include "Strongholds/Furnishings/SolidArrangeSign.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Strongholds/SolidStronghold.h"
#include "Strongholds/Tower/Actors/SolidTowerInterior.h"
#include "Strongholds/Tower/SolidTower.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidArrangeModeTest,
	"SolidCore1.Stronghold.Arrange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidArrangeModeTest::RunTest(const FString& Parameters)
{
	FSolidArrange Arrange;
	FSolidArrange::FContext OpenWorld;
	OpenWorld.bInStronghold = false;
	TestFalse(TEXT("backslash does nothing in the open world"), Arrange.Toggle(OpenWorld));
	TestFalse(TEXT("the mode stays off outside"), Arrange.IsOn());

	FSolidArrange::FContext Stronghold;
	Stronghold.bInStronghold = true;
	TestTrue(TEXT("backslash turns the mode on in a stronghold"), Arrange.Toggle(Stronghold));
	TestTrue(TEXT("the mode is on"), Arrange.IsOn());

	FSolidArrange::FContext Overlay;
	Overlay.bInStronghold = true;
	Overlay.bOverlay = true;
	TestFalse(TEXT("an overlay does not turn the mode off"), Arrange.Toggle(Overlay));
	TestTrue(TEXT("the mode stays on under an overlay"), Arrange.IsOn());

	TestFalse(TEXT("the open world does not turn the mode off"), Arrange.Toggle(OpenWorld));
	TestTrue(TEXT("leaving is what ends it"), Arrange.IsOn());

	Arrange.Leave();
	TestFalse(TEXT("leave draws the mode off"), Arrange.IsOn());
	Arrange.Leave();
	TestFalse(TEXT("leave again stays off"), Arrange.IsOn());

	TestFalse(TEXT("an overlay does not turn the mode on"), Arrange.Toggle(Overlay));
	TestFalse(TEXT("the mode stays off under an overlay"), Arrange.IsOn());

	TestTrue(TEXT("backslash turns it on again"), Arrange.Toggle(Stronghold));
	TestTrue(TEXT("backslash turns it off"), Arrange.Toggle(Stronghold));
	TestFalse(TEXT("the mode is off"), Arrange.IsOn());

	TestEqual(
		TEXT("the plate reads Arrange furniture"),
		FString(FSolidArrangeSign::Label()),
		FString(TEXT("Arrange furniture")));
	TestEqual(
		TEXT("arrange mode reads Done arranging"),
		FString(FSolidArrangeSign::DoneLabel()),
		FString(TEXT("Done arranging")));

	const FSolidArrangeSign Cave = FSolidArrangeSign::ForCave();
	TestTrue(TEXT("the cave has a plate"), Cave.IsPresent());
	TestTrue(TEXT("the cave plate faces into the room"), FMath::IsNearlyEqual(Cave.YawDeg(), 180.f, 0.01f));
	TestTrue(TEXT("the cave plate sits at shoulder height"), FMath::IsNearlyEqual(Cave.Location().Z, 150.f, 0.01f));

	const FVector2D East = SolidStrongholdGrid::HexCenter(2, 0, SolidDungeonLevel::MeagreOrigin());
	TestTrue(TEXT("the cave plate is on the east side of its hex"), Cave.Location().X > East.X);
	TestTrue(
		TEXT("the cave plate stands inside the east flat"),
		Cave.Location().X < East.X + SolidStrongholdGrid::FlatToFlatCm * 0.5f);
	const FVector2D Mouth = SolidDungeonLevel::MeagreMouthMidpoint();
	TestTrue(
		TEXT("the cave plate is clear of the mouth"),
		FVector2D::Distance(FVector2D(Cave.Location().X, Cave.Location().Y), Mouth) > SolidDungeonLevel::DoorwayWidthCm);

	const FVector CaveForward = FRotator(0.f, Cave.YawDeg(), 0.f).Vector();
	TestTrue(TEXT("yaw 180 looks west, into the cave"), CaveForward.X < -0.9f);

	TArray<FSolidArrangeSign::FStroke> Strokes;
	Cave.CollectStrokes(Strokes);
	TestTrue(TEXT("the words are stroked"), Strokes.Num() > 80);
	const FVector Plate = Cave.PlateSizeCm();
	const float CornerCm = SolidDungeonLevel::ShellCm * 0.35f * 0.5f;
	const float OpenFaceCm = SolidStrongholdGrid::SideCm() - CornerCm * 2.f;
	TestTrue(TEXT("the cave plate clears the corner rocks"), Plate.Y <= OpenFaceCm + 0.05f);
	bool bInside = true;
	bool bProud = true;
	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();
	float MaxAbsY = 0.f;
	for (const FSolidArrangeSign::FStroke& Stroke : Strokes)
	{
		const float HalfY = Stroke.SizeCm.Y * 0.5f;
		const float HalfZ = Stroke.SizeCm.Z * 0.5f;
		bInside = bInside
			&& FMath::Abs(Stroke.CenterCm.Y) + HalfY <= Plate.Y * 0.5f + 0.05f
			&& FMath::Abs(Stroke.CenterCm.Z) + HalfZ <= Plate.Z * 0.5f + 0.05f;
		bProud = bProud && Stroke.CenterCm.X > 0.f;
		MinZ = FMath::Min(MinZ, Stroke.CenterCm.Z);
		MaxZ = FMath::Max(MaxZ, Stroke.CenterCm.Z);
		MaxAbsY = FMath::Max(MaxAbsY, FMath::Abs(Stroke.CenterCm.Y) + HalfY);
	}
	TestTrue(TEXT("the strokes stay inside the plate"), bInside);
	TestTrue(TEXT("the words keep cream on both sides"), MaxAbsY <= Plate.Y * 0.5f - 6.f);
	TestTrue(
		TEXT("the words clear the corner rocks"),
		MaxAbsY < SolidStrongholdGrid::SideCm() * 0.5f - CornerCm);
	TestTrue(TEXT("the strokes stand proud of the plate, into the room"), bProud);
	TestTrue(TEXT("the two lines sit one above the other"), MinZ < 0.f && MaxZ > 0.f);

	TArray<FSolidArrangeSign::FStroke> DoneStrokes;
	Cave.CollectStrokes(DoneStrokes, true);
	TestTrue(TEXT("Done arranging is stroked"), DoneStrokes.Num() > 80);
	TestNotEqual(TEXT("Done arranging is not the resting words"), DoneStrokes.Num(), Strokes.Num());
	bool bDoneInside = true;
	float DoneMinZ = TNumericLimits<float>::Max();
	float DoneMaxZ = TNumericLimits<float>::Lowest();
	float DoneMaxAbsY = 0.f;
	for (const FSolidArrangeSign::FStroke& Stroke : DoneStrokes)
	{
		const float HalfY = Stroke.SizeCm.Y * 0.5f;
		const float HalfZ = Stroke.SizeCm.Z * 0.5f;
		bDoneInside = bDoneInside
			&& FMath::Abs(Stroke.CenterCm.Y) + HalfY <= Plate.Y * 0.5f + 0.05f
			&& FMath::Abs(Stroke.CenterCm.Z) + HalfZ <= Plate.Z * 0.5f + 0.05f;
		DoneMinZ = FMath::Min(DoneMinZ, Stroke.CenterCm.Z);
		DoneMaxZ = FMath::Max(DoneMaxZ, Stroke.CenterCm.Z);
		DoneMaxAbsY = FMath::Max(DoneMaxAbsY, FMath::Abs(Stroke.CenterCm.Y) + HalfY);
	}
	TestTrue(TEXT("Done arranging stays inside the plate"), bDoneInside);
	TestTrue(TEXT("Done arranging keeps cream on both sides"), DoneMaxAbsY <= Plate.Y * 0.5f - 6.f);
	TestTrue(TEXT("Done arranging is two lines"), DoneMinZ < 0.f && DoneMaxZ > 0.f);

	const FIntPoint OriginCell = FIntPoint::ZeroValue;
	const TUniquePtr<FSolidStronghold> Owned = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		OriginCell,
		SolidStronghold::WorldXY(OriginCell));
	TestNotNull(TEXT("an Average tower"), Owned.Get());
	const SolidStronghold::FSolidTower* Tower = static_cast<const SolidStronghold::FSolidTower*>(Owned.Get());
	if (!Tower)
	{
		return false;
	}

	const FSolidArrangeSign TowerPlate = FSolidArrangeSign::ForTowerFloor(*Tower, 0);
	TestTrue(
		TEXT("a tower plate keeps the full letter size"),
		TowerPlate.PlateSizeCm().Y > Cave.PlateSizeCm().Y);
	TestFalse(TEXT("a floor below the tower has no plate"), FSolidArrangeSign::ForTowerFloor(*Tower, -1).IsPresent());
	TestFalse(
		TEXT("a floor above the tower has no plate"),
		FSolidArrangeSign::ForTowerFloor(*Tower, Tower->FloorCount()).IsPresent());

	for (int32 Floor = 0; Floor < Tower->FloorCount(); ++Floor)
	{
		const FSolidArrangeSign Sign = FSolidArrangeSign::ForTowerFloor(*Tower, Floor);
		TestTrue(*FString::Printf(TEXT("floor %d has a plate"), Floor), Sign.IsPresent());
		if (!Sign.IsPresent())
		{
			continue;
		}

		const FVector Location = Sign.Location();
		TestTrue(
			TEXT("the plate is at the floor-number height"),
			FMath::IsNearlyEqual(Location.Z, Tower->FloorZCm(Floor) + SolidStronghold::FloorSignLiftCm, 0.1f));

		const FVector2D Radial(Location.X, Location.Y);
		const FVector2D Inward = -Radial.GetSafeNormal();
		const FVector Forward = FRotator(0.f, Sign.YawDeg(), 0.f).Vector();
		TestTrue(
			TEXT("the plate faces the center of the tower"),
			FVector2D::DotProduct(FVector2D(Forward.X, Forward.Y), Inward) > 0.9f);

		const float DoorDot = FVector2D::DotProduct(Radial.GetSafeNormal(), Tower->DoorOutward());
		TestTrue(TEXT("the plate is clear of the door"), DoorDot < 0.25f);

		const SolidStronghold::FSolidFloorSign Number = SolidStronghold::FloorSignFor(Tower->Plan(), Floor);
		if (Number.IsPresent())
		{
			TestTrue(
				TEXT("the plate is clear of the floor number"),
				FVector::Dist(Location, Number.Location()) > 80.f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidArrangePlateTest,
	"SolidCore1.Stronghold.ArrangePlate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidArrangePlateTest::RunTest(const FString& Parameters)
{
	ASolidDungeonInterior* Cave = NewObject<ASolidDungeonInterior>();
	TestNotNull(TEXT("cave"), Cave);
	if (!Cave)
	{
		return false;
	}
	Cave->BuildVisuals();

	UStaticMeshComponent* Plate = Cave->GetArrangePlate();
	UInstancedStaticMeshComponent* CaveLetters = Cave->GetArrangeLetters();
	TestNotNull(TEXT("cave plate"), Plate);
	TestNotNull(TEXT("cave letters"), CaveLetters);
	if (!Plate || !CaveLetters)
	{
		return false;
	}

	UStaticMesh* PlateMesh = Plate->GetStaticMesh();
	TestNotNull(TEXT("cave plate mesh"), PlateMesh);
	TestTrue(TEXT("the words are instanced"), CaveLetters->GetInstanceCount() > 80);
	const int32 LetterCount = CaveLetters->GetInstanceCount();
	Cave->SetArrangeSignDone(true);
	TestTrue(TEXT("Done arranging is instanced"), CaveLetters->GetInstanceCount() > 80);
	TestNotEqual(TEXT("Done arranging replaces the resting words"), CaveLetters->GetInstanceCount(), LetterCount);
	Cave->SetArrangeSignDone(false);
	TestEqual(TEXT("leaving the mode restores Arrange furniture"), CaveLetters->GetInstanceCount(), LetterCount);
	Cave->BuildVisuals();
	TestEqual(TEXT("a second build does not stack the letters"), CaveLetters->GetInstanceCount(), LetterCount);
	TestTrue(TEXT("the plate is a pointer query"), Plate->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("the plate ignores the captain"), Plate->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	TestTrue(TEXT("the plate takes the pointer"), Plate->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
	TestEqual(TEXT("the cave has one plate"), Cave->ArrangeSignCount(), 1);

	const FIntPoint OriginCell = FIntPoint::ZeroValue;
	const TUniquePtr<FSolidStronghold> Owned = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		OriginCell,
		SolidStronghold::WorldXY(OriginCell));
	const SolidStronghold::FSolidTower* Tower = static_cast<const SolidStronghold::FSolidTower*>(Owned.Get());
	TestNotNull(TEXT("tower"), Tower);
	if (!Tower)
	{
		return false;
	}

	ASolidTowerInterior* Interior = NewObject<ASolidTowerInterior>();
	TestNotNull(TEXT("tower interior"), Interior);
	if (!Interior)
	{
		return false;
	}
	Interior->SetTower(*Tower);
	Interior->BuildVisuals();
	Interior->BuildVisuals();
	TestEqual(TEXT("each floor has a plate"), Interior->ArrangeSignCount(), Tower->FloorCount());
	for (int32 Index = 0; Index < Interior->ArrangeSignCount(); ++Index)
	{
		UStaticMeshComponent* FloorPlate = Interior->GetArrangeSignPlate(Index);
		TestNotNull(TEXT("floor plate"), FloorPlate);
		if (!FloorPlate)
		{
			continue;
		}
		TestTrue(
			TEXT("a floor plate is a pointer query"),
			FloorPlate->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
		TestTrue(
			TEXT("a floor plate ignores the captain"),
			FloorPlate->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	}
	UInstancedStaticMeshComponent* TowerLetters = Interior->GetArrangeLetters();
	TestNotNull(TEXT("tower letters"), TowerLetters);
	if (TowerLetters)
	{
		const int32 TowerLetterCount = TowerLetters->GetInstanceCount();
		TestTrue(TEXT("each floor's words are instanced"), TowerLetterCount > 80 * Tower->FloorCount());
		Interior->SetArrangeSignDone(true);
		TestTrue(
			TEXT("each floor reads Done arranging"),
			TowerLetters->GetInstanceCount() > 80 * Tower->FloorCount());
		TestNotEqual(
			TEXT("Done arranging replaces each floor"),
			TowerLetters->GetInstanceCount(),
			TowerLetterCount);
		Interior->SetArrangeSignDone(false);
		TestEqual(
			TEXT("leaving the mode restores each floor"),
			TowerLetters->GetInstanceCount(),
			TowerLetterCount);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidArrangePlaceTest,
	"SolidCore1.Stronghold.ArrangePlace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidArrangePlaceTest::RunTest(const FString& Parameters)
{
	const TArray<SolidStrongholdFurniture::FPlacedFurnishing> Starters = SolidStrongholdFurniture::StartingFurnishings();
	TestEqual(TEXT("a new game still has three pieces"), Starters.Num(), 3);
	if (Starters.Num() == 3)
	{
		TestEqual(TEXT("the fireplace starts on (2, 0)"), Starters[0].X, 2);
		TestEqual(TEXT("the fireplace starts on row 0"), Starters[0].Y, 0);
		TestEqual(TEXT("the bed starts on (0, -1)"), Starters[1].X, 0);
		TestEqual(TEXT("the bed starts on row -1"), Starters[1].Y, -1);
		TestEqual(TEXT("the chest starts on (2, 1)"), Starters[2].X, 2);
		TestEqual(TEXT("the chest starts on row 1"), Starters[2].Y, 1);
		TestTrue(TEXT("the starters face the first way"), Starters[0].RotationDeg == 0.f
			&& Starters[1].RotationDeg == 0.f && Starters[2].RotationDeg == 0.f);
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidDungeonLevel::CollectMeagreHexes(Hexes);
	const SolidStrongholdGrid::FCell Entrance = SolidDungeonLevel::MeagreEntranceCell();
	const SolidStrongholdGrid::FCell Captain = SolidDungeonLevel::MeagreCaptainCell();

	auto OpenCave = [&](FSolidArrange& Arrange)
	{
		FSolidArrange::FContext Context;
		Context.bInStronghold = true;
		Arrange.Toggle(Context);
		Arrange.Open(Starters, Hexes, Entrance, true, Captain, true, 0.f);
	};

	FSolidArrange Arrange;
	OpenCave(Arrange);
	TestTrue(TEXT("the mode is on for the floor"), Arrange.IsOn());
	TestTrue(
		TEXT("a closer furnishing is the one under the pointer"),
		FSolidArrange::FurnishingIsInFront(80.f, true, 140.f));
	TestFalse(
		TEXT("the floor hex behind is not named"),
		FSolidArrange::FurnishingIsInFront(140.f, true, 80.f));
	TestFalse(
		TEXT("a mesh on the floor point leaves that hex"),
		FSolidArrange::FurnishingIsInFront(80.f, true, 80.f));
	TestTrue(
		TEXT("a furnishing with no floor behind it is named"),
		FSolidArrange::FurnishingIsInFront(40.f, false, 0.f));
	TestEqual(TEXT("wheel up is one facing, the same way as Q"), FSolidArrange::WheelSteps(1.f), 1);
	TestEqual(TEXT("wheel down is the other way, the same as E"), FSolidArrange::WheelSteps(-1.f), -1);
	TestEqual(TEXT("a quiet wheel does not turn"), FSolidArrange::WheelSteps(0.f), 0);
	TestEqual(TEXT("a wheel spike stays within three facings"), FSolidArrange::WheelSteps(4.f), 3);

	Arrange.SetHover(2, 0);
	TestEqual(TEXT("the fireplace hex names the fireplace"), FString(Arrange.HoverName()), FString(TEXT("Fireplace")));
	const SolidStrongholdGrid::FHex* BedSpan = Arrange.PieceAt(1).SpanCell(Hexes);
	TestNotNull(TEXT("the bed has a second hex"), BedSpan);
	if (BedSpan)
	{
		Arrange.SetHover(BedSpan->Col, BedSpan->Row);
		TestEqual(TEXT("the bed's second hex names the bed"), FString(Arrange.HoverName()), FString(TEXT("Bed")));
		TArray<SolidStrongholdGrid::FHex> Marked;
		Arrange.HighlightHexes(Marked);
		TestEqual(TEXT("a bed highlights both hexes"), Marked.Num(), 2);
	}
	Arrange.SetHover(1, 0);
	TestTrue(TEXT("an empty hex has no name"), FString(Arrange.HoverName()).IsEmpty());
	TArray<SolidStrongholdGrid::FHex> OneHex;
	Arrange.HighlightHexes(OneHex);
	TestEqual(TEXT("an empty hex highlights that hex"), OneHex.Num(), 1);
	Arrange.ClearHover();
	Arrange.HighlightHexes(OneHex);
	TestEqual(TEXT("no hover leaves the floor unmarked"), OneHex.Num(), 0);
	Arrange.SetHover(9, 9);
	TestTrue(TEXT("a cell off the floor has no name"), FString(Arrange.HoverName()).IsEmpty());
	Arrange.HighlightHexes(OneHex);
	TestEqual(TEXT("a cell off the floor is not marked"), OneHex.Num(), 0);

	Arrange.SetHover(1, 0);
	TestEqual(
		TEXT("an empty hex opens the browser"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Opened));
	TestTrue(TEXT("the browser is open"), Arrange.IsBrowserOpen());
	TestEqual(TEXT("the browser remembers the column"), Arrange.BrowserColumn(), 1);
	TestEqual(TEXT("the browser remembers the row"), Arrange.BrowserRow(), 0);
	TestFalse(TEXT("nothing was lifted"), Arrange.IsHolding());
	Arrange.SetHover(Entrance.Col, Entrance.Row);
	TestEqual(
		TEXT("the entrance does not open the browser"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::None));
	TestEqual(TEXT("the entrance leaves the remembered column"), Arrange.BrowserColumn(), 1);
	TestEqual(TEXT("the entrance leaves the remembered row"), Arrange.BrowserRow(), 0);
	Arrange.SetHover(Captain.Col, Captain.Row);
	TestEqual(
		TEXT("the captain's hex does not open the browser"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::None));
	TestEqual(TEXT("the captain's hex leaves the remembered column"), Arrange.BrowserColumn(), 1);
	Arrange.SetHover(1, -2);
	TestEqual(
		TEXT("another empty hex keeps the browser open"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Opened));
	TestEqual(TEXT("the browser uses the new column"), Arrange.BrowserColumn(), 1);
	TestEqual(TEXT("the browser uses the new row"), Arrange.BrowserRow(), -2);
	TestTrue(TEXT("esc closes the browser"), Arrange.PutBack());
	TestTrue(TEXT("esc leaves the mode on"), Arrange.IsOn());
	TestFalse(TEXT("the browser is closed"), Arrange.IsBrowserOpen());
	Arrange.Turn(1);
	TestTrue(TEXT("Q does nothing until a piece is held"), Arrange.PieceAt(0).RotationDeg == 0.f);

	SolidStrongholdFurniture::FPlacedFurnishing FireplaceOnBed = Starters[0];
	FireplaceOnBed.X = 0;
	FireplaceOnBed.Y = -1;
	TestFalse(TEXT("the bed's hex is taken"), Arrange.CanPlace(FireplaceOnBed));

	Arrange.SetHover(0, -1);
	TestEqual(
		TEXT("confirm lifts the bed"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	TestTrue(TEXT("one piece is held"), Arrange.IsHolding());
	TestFalse(TEXT("lifting closes the browser"), Arrange.IsBrowserOpen());
	TestEqual(TEXT("the held bed keeps its name"), FString(Arrange.HoverName()), FString(TEXT("Bed")));
	TestTrue(TEXT("the bed's old hex is empty while it is held"), Arrange.CanPlace(FireplaceOnBed));

	FSolidArrange::FDraw Drawn;
	for (int32 Step = 0; Step < 6; ++Step)
	{
		Arrange.Turn(1);
	}
	TestTrue(TEXT("six facings restore the bed"), Arrange.DrawOf(1, Drawn));
	TestTrue(TEXT("six steps land on the starting facing"), FMath::IsNearlyEqual(Drawn.Yaw, 0.f, 0.01f));
	Arrange.Turn(FSolidArrange::WheelSteps(1.f));
	TestTrue(TEXT("the ghost turned"), Arrange.DrawOf(1, Drawn));
	TestTrue(TEXT("wheel up matches Q"), FMath::IsNearlyEqual(Drawn.Yaw, FSolidArrange::FacingDeg, 0.01f));
	Arrange.Turn(FSolidArrange::WheelSteps(-1.f));
	TestTrue(TEXT("the ghost turned back"), Arrange.DrawOf(1, Drawn));
	TestTrue(TEXT("wheel down matches E"), FMath::IsNearlyEqual(Drawn.Yaw, 0.f, 0.01f));

	Arrange.SetHover(0, 0);
	Arrange.Turn(3);
	SolidStrongholdFurniture::FPlacedFurnishing Ghost;
	TestTrue(TEXT("the turned bed is the ghost"), Arrange.DrawOf(1, Drawn));
	Ghost = Arrange.PieceAt(1);
	Ghost.X = 0;
	Ghost.Y = 0;
	Ghost.RotationDeg = Drawn.Yaw;
	TestFalse(TEXT("a bed on the captain's hex is refused"), Arrange.CanPlace(Ghost));
	TestEqual(
		TEXT("confirm does not put the bed on the captain"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Refused));
	TestTrue(TEXT("the bed stays held"), Arrange.IsHolding());

	Arrange.SetHover(Entrance.Col, Entrance.Row);
	TestEqual(
		TEXT("the entrance does not take the bed"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Refused));
	Arrange.SetHover(2, 1);
	TestEqual(
		TEXT("the chest's hex does not take the bed"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Refused));

	Arrange.ClearHover();
	TestEqual(
		TEXT("off the floor does not put the bed down"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::None));
	TestTrue(TEXT("the bed is still held off the floor"), Arrange.IsHolding());

	Arrange.SetHover(0, 0);
	Arrange.Turn(-3);
	TestEqual(
		TEXT("a legal confirm puts the bed down"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Placed));
	TestFalse(TEXT("the bed is down"), Arrange.IsHolding());
	TestFalse(TEXT("putting the piece down leaves the browser closed"), Arrange.IsBrowserOpen());
	TestEqual(TEXT("the bed's anchor is (0, 0)"), Arrange.PieceAt(1).X, 0);
	TestEqual(TEXT("the bed's row is 0"), Arrange.PieceAt(1).Y, 0);
	TestTrue(TEXT("the bed kept its facing"), FMath::IsNearlyEqual(Arrange.PieceAt(1).RotationDeg, 0.f, 0.01f));

	int32 LegalFacings = 0;
	int32 IllegalFacings = 0;
	for (int32 Step = 0; Step < 6; ++Step)
	{
		SolidStrongholdFurniture::FPlacedFurnishing Candidate = Starters[1];
		Candidate.X = 0;
		Candidate.Y = -2;
		Candidate.RotationDeg = static_cast<float>(Step) * FSolidArrange::FacingDeg;
		if (Arrange.CanPlace(Candidate))
		{
			++LegalFacings;
		}
		else
		{
			++IllegalFacings;
		}
	}
	TestTrue(TEXT("a bed can sit on the south edge"), LegalFacings > 0);
	TestTrue(TEXT("a bed facing off that edge is refused"), IllegalFacings > 0);

	FSolidArrange Restored;
	OpenCave(Restored);
	Restored.SetHover(2, 0);
	Restored.Confirm();
	Restored.Turn(2);
	Restored.SetHover(1, -2);
	TestTrue(TEXT("esc puts the fireplace back"), Restored.PutBack());
	TestTrue(TEXT("esc leaves the mode on"), Restored.IsOn());
	TestFalse(TEXT("esc is no longer holding"), Restored.IsHolding());
	TestEqual(TEXT("the fireplace is back on (2, 0)"), Restored.PieceAt(0).X, 2);
	TestEqual(TEXT("the fireplace is back on row 0"), Restored.PieceAt(0).Y, 0);
	TestTrue(TEXT("the fireplace faces the way it started"), Restored.PieceAt(0).RotationDeg == 0.f);

	Restored.SetHover(2, 0);
	Restored.Confirm();
	Restored.Turn(2);
	Restored.SetHover(1, -2);
	FSolidArrange::FContext Stronghold;
	Stronghold.bInStronghold = true;
	TestTrue(TEXT("the plate turns the mode off"), Restored.Toggle(Stronghold));
	TestFalse(TEXT("the mode is off"), Restored.IsOn());
	TestFalse(TEXT("leaving the mode is not holding"), Restored.IsHolding());
	TestEqual(TEXT("the fireplace was put back"), Restored.PieceAt(0).X, 2);
	TestTrue(TEXT("the facing was put back"), Restored.PieceAt(0).RotationDeg == 0.f);

	FSolidArrange Left;
	OpenCave(Left);
	Left.SetHover(2, 1);
	Left.Confirm();
	Left.Turn(1);
	Left.Leave();
	TestFalse(TEXT("leaving the stronghold ends the mode"), Left.IsOn());
	TestFalse(TEXT("the chest was not left held"), Left.IsHolding());
	TestEqual(TEXT("the chest is on (2, 1)"), Left.PieceAt(2).X, 2);
	TestEqual(TEXT("the chest is on row 1"), Left.PieceAt(2).Y, 1);
	TestTrue(TEXT("the chest faces the way it started"), Left.PieceAt(2).RotationDeg == 0.f);

	FSolidArrange Floor;
	OpenCave(Floor);
	const SolidStrongholdGrid::FHex* East = SolidStrongholdFurniture::FindCell(Hexes, 2, 0);
	TestNotNull(TEXT("hex (2, 0)"), East);
	if (East)
	{
		FSolidArrange::FTrace Hit;
		TestTrue(TEXT("a ray onto the fireplace hex hits the floor"),
			Floor.Trace(FVector(East->Center.X, East->Center.Y, 400.f), FVector(0.f, 0.f, -1.f), 80000.f, Hit));
		TestTrue(TEXT("that ray is the fireplace hex"), Hit.bCell && Hit.X == 2 && Hit.Y == 0);
		TestTrue(TEXT("a ray off the cave still meets the plane"),
			Floor.Trace(FVector(8000.f, 8000.f, 400.f), FVector(0.f, 0.f, -1.f), 80000.f, Hit));
		TestTrue(TEXT("that ray is off every cell"), Hit.bPlane && !Hit.bCell);
		TestFalse(TEXT("a ray away from the floor misses"),
			Floor.Trace(FVector(East->Center.X, East->Center.Y, 400.f), FVector(0.f, 0.f, 1.f), 80000.f, Hit));
	}

	const FIntPoint OriginCell = FIntPoint::ZeroValue;
	const TUniquePtr<FSolidStronghold> Owned = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		OriginCell,
		SolidStronghold::WorldXY(OriginCell));
	const SolidStronghold::FSolidTower* Tower = static_cast<const SolidStronghold::FSolidTower*>(Owned.Get());
	TestNotNull(TEXT("average tower"), Tower);
	if (!Tower)
	{
		return false;
	}
	ASolidTowerInterior* Interior = NewObject<ASolidTowerInterior>();
	TestNotNull(TEXT("tower interior"), Interior);
	if (!Interior)
	{
		return false;
	}
	Interior->SetTower(*Tower);
	TestEqual(TEXT("every tower floor can be named"), Interior->ArrangeFloorCount(), Tower->FloorCount());
	TestTrue(TEXT("the tower has an upper floor"), Tower->FloorCount() >= 2);
	float GroundZ = 0.f;
	float UpperZ = 0.f;
	TestTrue(TEXT("the ground floor has a height"), Interior->ArrangeFloorZ(0, GroundZ));
	TestTrue(TEXT("the next floor has a height"), Interior->ArrangeFloorZ(1, UpperZ));
	TestEqual(TEXT("feet on the ground use that floor"), Interior->ArrangeFloorUnderFeet(FVector(0.f, 0.f, GroundZ + 10.f)), 0);
	TestEqual(
		TEXT("feet on the next floor use that floor"),
		Interior->ArrangeFloorUnderFeet(FVector(0.f, 0.f, UpperZ + 10.f)),
		1);

	TArray<SolidStrongholdGrid::FHex> Ground;
	TestTrue(TEXT("the ground floor has hexes"), Interior->CollectArrangeFloor(0, Ground));
	int32 DoorX = 0;
	int32 DoorY = 0;
	const bool bDoorKept = Interior->ArrangeEntrance(0, DoorX, DoorY);
	bool bDoorInRoom = false;
	for (const SolidStrongholdGrid::FHex& Hex : Ground)
	{
		if (FVector2D::DistSquared(Hex.Center, Tower->DoorHexCenter()) < 1.f)
		{
			bDoorInRoom = true;
			if (bDoorKept)
			{
				TestEqual(TEXT("the entrance column is the door hex"), DoorX, Hex.Col);
				TestEqual(TEXT("the entrance row is the door hex"), DoorY, Hex.Row);
			}
		}
	}
	TestTrue(TEXT("a dropped door hex is not a furniture cell"), bDoorKept == bDoorInRoom);

	TArray<SolidStrongholdGrid::FHex> Upper;
	TestTrue(TEXT("the upper floor has its own hexes"), Interior->CollectArrangeFloor(1, Upper));
	TestTrue(TEXT("another floor is a different list"), Upper.Num() > 0);

	const int32 Table = 3;
	const SolidStrongholdFurniture::FItem* TableItem = SolidStrongholdFurniture::ItemAt(Table);
	TestNotNull(TEXT("small table is the fourth row"), TableItem);
	if (!TableItem)
	{
		return false;
	}
	TestEqual(TEXT("the fourth row is Small Table"), FString(TableItem->Name), FString(TEXT("Small Table")));

	FSolidArrange Column;
	OpenCave(Column);
	TestFalse(TEXT("a closed column ignores Up and Down"), Column.MoveCatalog(1, 4));
	Column.SetHover(1, 0);
	Column.Confirm();
	TestEqual(TEXT("the column starts on Fireplace"), Column.CatalogHighlight(), 0);
	TestTrue(TEXT("Down leaves Fireplace"), Column.MoveCatalog(1, 4));
	TestEqual(TEXT("Down lands on Bed"), Column.CatalogHighlight(), 1);
	TestTrue(TEXT("Up returns to Fireplace"), Column.MoveCatalog(-1, 4));
	TestEqual(TEXT("Up lands on Fireplace"), Column.CatalogHighlight(), 0);
	TestTrue(TEXT("Up wraps to Small Table"), Column.MoveCatalog(-1, 4));
	TestEqual(TEXT("the highlight is Small Table"), Column.CatalogHighlight(), 3);
	Column.SetHover(1, -2);
	Column.Confirm();
	TestEqual(TEXT("another empty hex keeps the highlight"), Column.CatalogHighlight(), 3);
	TestTrue(TEXT("the pointer on an empty hex is not a placed piece"), !Column.HoverIsPlaced());
	Column.SetHover(2, 0);
	TestTrue(TEXT("the pointer on the fireplace is a placed piece"), Column.HoverIsPlaced());

	TestEqual(
		TEXT("the fireplace is already at its limit"),
		static_cast<int32>(Column.LiftCatalog(0, Starters)),
		static_cast<int32>(FSolidArrange::EConfirm::Refused));
	TestTrue(TEXT("a full row leaves the column open"), Column.IsBrowserOpen());
	TestEqual(TEXT("a refused row adds nothing"), Column.PieceCount(), 3);

	TestEqual(
		TEXT("a second chest lifts"),
		static_cast<int32>(Column.LiftCatalog(2, Starters)),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	TestEqual(TEXT("the new chest has the grid"), Column.PieceAt(3).Slots.Num(), SolidStorage::Count);
	bool bEmptyChest = true;
	for (const FSolidItem& Slot : Column.PieceAt(3).Slots)
	{
		bEmptyChest = bEmptyChest && Slot.IsEmpty();
	}
	TestTrue(TEXT("the new chest starts empty"), bEmptyChest);
	TestTrue(TEXT("the new chest is discarded"), Column.PutBack());
	TestEqual(TEXT("the starters are unchanged"), Column.PieceCount(), 3);

	TArray<SolidStrongholdFurniture::FPlacedFurnishing> ThreeChests = Starters;
	SolidStrongholdFurniture::FPlacedFurnishing ExtraChest;
	ExtraChest.ItemIndex = 2;
	ThreeChests.Add(ExtraChest);
	ThreeChests.Add(ExtraChest);
	Column.SetHover(1, -2);
	Column.Confirm();
	TestEqual(
		TEXT("three chests stop a fourth"),
		static_cast<int32>(Column.LiftCatalog(2, ThreeChests)),
		static_cast<int32>(FSolidArrange::EConfirm::Refused));

	TArray<SolidStrongholdFurniture::FPlacedFurnishing> TenTables = Starters;
	SolidStrongholdFurniture::FPlacedFurnishing ExtraTable;
	ExtraTable.ItemIndex = Table;
	for (int32 Added = 0; Added < 10; ++Added)
	{
		TenTables.Add(ExtraTable);
	}
	TestEqual(
		TEXT("ten small tables stop an eleventh"),
		static_cast<int32>(Column.LiftCatalog(Table, TenTables)),
		static_cast<int32>(FSolidArrange::EConfirm::Refused));

	TestEqual(
		TEXT("a small table lifts onto the remembered hex"),
		static_cast<int32>(Column.LiftCatalog(Table, Starters)),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	TestTrue(TEXT("the small table is held"), Column.IsHolding());
	TestFalse(TEXT("lifting closes the column"), Column.IsBrowserOpen());
	TestEqual(TEXT("the new piece is the fourth"), Column.PieceCount(), 4);
	TestEqual(TEXT("the small table uses the remembered column"), Column.PieceAt(3).X, 1);
	TestEqual(TEXT("the small table uses the remembered row"), Column.PieceAt(3).Y, -2);
	TestTrue(TEXT("esc discards the small table"), Column.PutBack());
	TestEqual(TEXT("the discarded table is gone"), Column.PieceCount(), 3);
	TestFalse(TEXT("nothing is held after the discard"), Column.IsHolding());
	TestFalse(TEXT("a discarded table is not written"), Column.WritePlaced(TenTables));

	Column.SetHover(1, 0);
	Column.Confirm();
	TestEqual(
		TEXT("the small table lifts again"),
		static_cast<int32>(Column.LiftCatalog(Table, Starters)),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	Column.SetHover(1, 0);
	TestEqual(
		TEXT("the small table goes down on the empty hex"),
		static_cast<int32>(Column.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Placed));
	TArray<SolidStrongholdFurniture::FPlacedFurnishing> Written = Starters;
	TestTrue(TEXT("putting the new piece down writes it"), Column.WritePlaced(Written));
	TestEqual(TEXT("the list gains the small table"), Written.Num(), 4);
	TestEqual(TEXT("the added row is the small table"), Written[3].ItemIndex, Table);
	TestEqual(TEXT("the added table is on column 1"), Written[3].X, 1);
	TestEqual(TEXT("the added table is on row 0"), Written[3].Y, 0);

	TestEqual(
		TEXT("a closed column does not lift"),
		static_cast<int32>(Column.LiftCatalog(1, Starters)),
		static_cast<int32>(FSolidArrange::EConfirm::None));
	Column.SetHover(1, -2);
	Column.Confirm();
	TestEqual(
		TEXT("the second bed lifts"),
		static_cast<int32>(Column.LiftCatalog(1, Starters)),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	TestTrue(TEXT("the second bed is held"), Column.IsHolding());
	Column.PutBack();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidArrangeRememberTest,
	"SolidCore1.Stronghold.ArrangeRemember",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidArrangeRememberTest::RunTest(const FString& Parameters)
{
	TArray<SolidStrongholdFurniture::FPlacedFurnishing> List = SolidStrongholdFurniture::StartingFurnishings();
	const TArray<SolidStrongholdFurniture::FPlacedFurnishing> Started = List;
	TestEqual(TEXT("a new game list is the starter three"), List.Num(), 3);
	if (List.Num() != 3)
	{
		return false;
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidDungeonLevel::CollectMeagreHexes(Hexes);
	const SolidStrongholdGrid::FCell Entrance = SolidDungeonLevel::MeagreEntranceCell();
	const SolidStrongholdGrid::FCell Captain = SolidDungeonLevel::MeagreCaptainCell();

	FSolidArrange Arrange;
	FSolidArrange::FContext Context;
	Context.bInStronghold = true;
	Arrange.Toggle(Context);
	Arrange.Open(List, Hexes, Entrance, true, Captain, true, 0.f);

	Arrange.SetHover(2, 0);
	TestEqual(
		TEXT("confirm lifts the fireplace"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	Arrange.Turn(1);
	Arrange.SetHover(1, -2);
	TestTrue(TEXT("esc puts the fireplace back"), Arrange.PutBack());
	TestFalse(TEXT("a put-back is not written"), Arrange.WritePlaced(List));
	TestEqual(TEXT("the fireplace stays on (2, 0)"), List[0].X, Started[0].X);
	TestEqual(TEXT("the fireplace stays on row 0"), List[0].Y, Started[0].Y);
	TestTrue(TEXT("the fireplace facing stays"), List[0].RotationDeg == Started[0].RotationDeg);

	Arrange.SetHover(0, -1);
	TestEqual(
		TEXT("confirm lifts the bed"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	Arrange.SetHover(0, 0);
	Arrange.Turn(1);
	FSolidArrange::FDraw Drawn;
	TestTrue(TEXT("the held bed draws"), Arrange.DrawOf(1, Drawn));
	SolidStrongholdFurniture::FPlacedFurnishing Candidate = Arrange.PieceAt(1);
	Candidate.X = 0;
	Candidate.Y = 0;
	Candidate.RotationDeg = Drawn.Yaw;
	if (!Arrange.CanPlace(Candidate))
	{
		Arrange.Turn(-1);
		TestTrue(TEXT("the held bed draws on the first facing"), Arrange.DrawOf(1, Drawn));
	}
	TestEqual(
		TEXT("a legal confirm puts the bed down"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Placed));
	TestTrue(TEXT("the put-down is written"), Arrange.WritePlaced(List));
	TestEqual(TEXT("the bed anchor is remembered"), List[1].X, 0);
	TestEqual(TEXT("the bed row is remembered"), List[1].Y, 0);
	TestTrue(
		TEXT("the bed facing is remembered"),
		FMath::IsNearlyEqual(List[1].RotationDeg, Drawn.Yaw, 0.01f));
	TestEqual(TEXT("the fireplace column stays"), List[0].X, Started[0].X);
	TestEqual(TEXT("the fireplace row stays"), List[0].Y, Started[0].Y);
	TestEqual(TEXT("the chest column stays"), List[2].X, Started[2].X);
	TestEqual(TEXT("the chest row stays"), List[2].Y, Started[2].Y);
	TestTrue(TEXT("the chest facing stays"), List[2].RotationDeg == Started[2].RotationDeg);

	Arrange.SetHover(2, 1);
	TestEqual(
		TEXT("confirm lifts the chest"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Lifted));
	Arrange.SetHover(Entrance.Col, Entrance.Row);
	TestEqual(
		TEXT("the entrance does not take the chest"),
		static_cast<int32>(Arrange.Confirm()),
		static_cast<int32>(FSolidArrange::EConfirm::Refused));
	TestFalse(TEXT("a refused hex is not written"), Arrange.WritePlaced(List));
	TestEqual(TEXT("the chest column is unchanged"), List[2].X, Started[2].X);
	TestEqual(TEXT("the chest row is unchanged"), List[2].Y, Started[2].Y);

	Arrange.Leave();
	TestFalse(TEXT("leaving does not write the held chest"), Arrange.WritePlaced(List));
	TestEqual(TEXT("the chest is still on (2, 1)"), List[2].X, 2);
	TestEqual(TEXT("the chest is still on row 1"), List[2].Y, 1);

	return true;
}

#endif
