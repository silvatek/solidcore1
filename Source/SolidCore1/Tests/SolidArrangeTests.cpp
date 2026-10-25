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
	const FVector Plate = FSolidArrangeSign::PlateSizeCm();
	bool bInside = true;
	bool bProud = true;
	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();
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
	}
	TestTrue(TEXT("the strokes stay inside the plate"), bInside);
	TestTrue(TEXT("the strokes stand proud of the plate, into the room"), bProud);
	TestTrue(TEXT("the two lines sit one above the other"), MinZ < 0.f && MaxZ > 0.f);

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
		TestTrue(TEXT("each floor's words are instanced"), TowerLetters->GetInstanceCount() > 80 * Tower->FloorCount());
	}

	return true;
}

#endif
