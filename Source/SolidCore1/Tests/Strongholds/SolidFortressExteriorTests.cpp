#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Fortress/Actors/SolidFortressExterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/SolidStronghold.h"
#include "Components/StaticMeshComponent.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidFortressExteriorTestPrivate
{
	float FaceMinX(const UStaticMeshComponent* Part)
	{
		return Part->GetRelativeLocation().X - Part->GetRelativeScale3D().X * 50.f;
	}

	float FaceMaxX(const UStaticMeshComponent* Part)
	{
		return Part->GetRelativeLocation().X + Part->GetRelativeScale3D().X * 50.f;
	}

	float TopZ(const UStaticMeshComponent* Part)
	{
		return Part->GetRelativeLocation().Z + Part->GetRelativeScale3D().Z * 50.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFortressExteriorTest,
	"SolidCore1.Stronghold.FortressExterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFortressExteriorTest::RunTest(const FString& Parameters)
{
	const FIntPoint HumbleCell(35, 15);
	TUniquePtr<FSolidStronghold> Keep = SolidStronghold::Make(
		ESolidStrongholdType::Fortress,
		ESolidStrongholdSize::Humble,
		HumbleCell,
		SolidStronghold::WorldXY(HumbleCell));
	TestNotNull(TEXT("a Humble fortress"), Keep.Get());
	if (!Keep)
	{
		return false;
	}

	const SolidStronghold::FSolidFortress* Fortress = static_cast<const SolidStronghold::FSolidFortress*>(Keep.Get());
	TestTrue(
		TEXT("Humble names the fortress exterior"),
		Keep->ExteriorClass() == ASolidFortressExterior::StaticClass());
	TestTrue(
		TEXT("no map leaves the gate on local -X at yaw 180"),
		FMath::IsNearlyEqual(Keep->ExteriorYawDeg(nullptr), 180.f, 0.01f));

	ASolidFortressExterior* Shell = NewObject<ASolidFortressExterior>();
	TestNotNull(TEXT("fortress exterior"), Shell);
	if (!Shell)
	{
		return false;
	}
	Keep->ConfigureExterior(Shell);
	TestEqual(TEXT("the exterior stands on the fortress column"), Shell->GetStrongholdCell().X, HumbleCell.X);
	TestEqual(TEXT("the exterior stands on the fortress row"), Shell->GetStrongholdCell().Y, HumbleCell.Y);

	UStaticMeshComponent* Gate = Shell->GetGateMesh();
	TestNotNull(TEXT("the gate"), Gate);
	TestEqual(TEXT("the opening is the gate"), Shell->GetPassageMesh(), Gate);
	TestEqual(TEXT("the outer wall is six stretches"), Shell->GetCurtainMeshes().Num(), 6);
	TestTrue(TEXT("Humble draws the building and its merlons"), Shell->GetBuildingMeshes().Num() > 1);
	if (!Gate || Shell->GetCurtainMeshes().Num() != 6 || Shell->GetBuildingMeshes().Num() <= 1)
	{
		return false;
	}

	const float HalfDepth = Fortress->OuterDepthCm() * 0.5f;
	TestTrue(
		TEXT("the gate is the opening"),
		FMath::IsNearlyEqual(Gate->GetRelativeScale3D().Y * 100.f, FSolidStronghold::OpeningCm, 0.5f)
			&& FMath::IsNearlyEqual(Gate->GetRelativeScale3D().Z * 100.f, FSolidStronghold::StoreyCm, 0.5f));
	TestTrue(TEXT("the gate is on local -X"), Gate->GetRelativeLocation().X < 0.f);
	TestTrue(TEXT("the gate is centered"), FMath::IsNearlyZero(Gate->GetRelativeLocation().Y, 0.5f));
	TestTrue(
		TEXT("the gate is flush with the north face"),
		FMath::IsNearlyEqual(SolidFortressExteriorTestPrivate::FaceMinX(Gate), -HalfDepth, 0.5f));
	TestTrue(
		TEXT("the gate blocks"),
		Gate->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);

	const float CurtainTop = Fortress->CurtainHeightCm();
	for (const TObjectPtr<UStaticMeshComponent>& Part : Shell->GetCurtainMeshes())
	{
		if (!TestNotNull(TEXT("a curtain stretch"), Part.Get()))
		{
			return false;
		}
		TestTrue(
			TEXT("the outer wall is the storey plus the parapet"),
			FMath::IsNearlyEqual(SolidFortressExteriorTestPrivate::TopZ(Part.Get()), CurtainTop, 0.5f));
	}

	const UStaticMeshComponent* Lintel = Shell->GetCurtainMeshes().Last();
	TestTrue(
		TEXT("the gate stops under the parapet"),
		FMath::IsNearlyEqual(
			Lintel->GetRelativeLocation().Z - Lintel->GetRelativeScale3D().Z * 50.f,
			FSolidStronghold::StoreyCm,
			0.5f));

	const float BuildingTop = Fortress->Buildings[0].HeightCm(Fortress->ParapetCm);
	const float Thickness = Fortress->ParapetCm * 0.4f;
	const UStaticMeshComponent* Building = nullptr;
	int32 Merlons = 0;
	int32 Lows = 0;
	float WidestLow = 0.f;
	float NarrowestHigh = TNumericLimits<float>::Max();
	for (const TObjectPtr<UStaticMeshComponent>& Part : Shell->GetBuildingMeshes())
	{
		if (!Part)
		{
			continue;
		}
		const float PartHeight = Part->GetRelativeScale3D().Z * 100.f;
		if (FMath::IsNearlyEqual(PartHeight, BuildingTop - Fortress->ParapetCm, 0.5f))
		{
			Building = Part.Get();
		}
		else if (FMath::IsNearlyEqual(PartHeight, Fortress->ParapetCm, 0.5f)
			&& FMath::IsNearlyEqual(SolidFortressExteriorTestPrivate::TopZ(Part.Get()), BuildingTop, 0.5f))
		{
			++Merlons;
		}
		else if (FMath::IsNearlyEqual(PartHeight, Fortress->ParapetCm * 0.5f, 0.5f))
		{
			++Lows;
		}
		else
		{
			continue;
		}

		if (!FMath::IsNearlyEqual(PartHeight, Fortress->ParapetCm, 0.5f)
			&& !FMath::IsNearlyEqual(PartHeight, Fortress->ParapetCm * 0.5f, 0.5f))
		{
			continue;
		}
		const float PlanX = Part->GetRelativeScale3D().Y * 100.f;
		const float PlanY = Part->GetRelativeScale3D().X * 100.f;
		const bool bThickX = FMath::IsNearlyEqual(PlanX, Thickness, 0.5f);
		const bool bThickY = FMath::IsNearlyEqual(PlanY, Thickness, 0.5f);
		TestTrue(TEXT("a merlon is forty percent of the parapet height thick"), bThickX || bThickY);
		if (bThickX && bThickY)
		{
			continue;
		}
		const float Along = bThickX ? PlanY : PlanX;
		if (FMath::IsNearlyEqual(PartHeight, Fortress->ParapetCm * 0.5f, 0.5f))
		{
			WidestLow = FMath::Max(WidestLow, Along);
		}
		else
		{
			NarrowestHigh = FMath::Min(NarrowestHigh, Along);
		}
	}
	TestNotNull(TEXT("the building body"), Building);
	if (!Building)
	{
		return false;
	}
	TestTrue(
		TEXT("the building stands above the wall"),
		FMath::IsNearlyEqual(SolidFortressExteriorTestPrivate::TopZ(Building), BuildingTop - Fortress->ParapetCm, 0.5f)
			&& BuildingTop > CurtainTop + 1.f);
	TestTrue(TEXT("the corners are high"), Merlons >= 4);
	TestTrue(TEXT("low sections sit between the corners"), Lows > 0);
	TestTrue(
		TEXT("a low section is narrower than a high section"),
		WidestLow > 0.f && WidestLow < NarrowestHigh);
	TestTrue(
		TEXT("the building meets the inner south curtain"),
		FMath::IsNearlyEqual(SolidFortressExteriorTestPrivate::FaceMaxX(Building), HalfDepth - FSolidStronghold::WallCm, 0.5f));

	const UStaticMeshComponent* Door = nullptr;
	int32 Doors = 0;
	for (const TObjectPtr<UStaticMeshComponent>& Part : Shell->GetBuildingMeshes())
	{
		if (!Part)
		{
			continue;
		}
		const float PlanX = Part->GetRelativeScale3D().Y * 100.f;
		const float PlanY = Part->GetRelativeScale3D().X * 100.f;
		const float SizeZ = Part->GetRelativeScale3D().Z * 100.f;
		if (FMath::IsNearlyEqual(SizeZ, FSolidStronghold::StoreyCm, 0.5f)
			&& FMath::IsNearlyEqual(PlanX, FSolidStronghold::OpeningCm, 0.5f)
			&& PlanY < FSolidStronghold::WallCm)
		{
			++Doors;
			Door = Part.Get();
		}
	}
	TestEqual(TEXT("Humble draws one doorway"), Doors, 1);
	TestNotNull(TEXT("the doorway"), Door);
	if (Door)
	{
		const SolidStronghold::FSolidFortressBuilding& KeepBuilding = Fortress->Buildings[0];
		const float DoorPlanX = KeepBuilding.InnerMinCm.X + KeepBuilding.DoorFromInnerCm();
		TestTrue(
			TEXT("the doorway is centered on the landing"),
			FMath::IsNearlyEqual(Door->GetRelativeLocation().Y, DoorPlanX - Fortress->OuterWidthCm() * 0.5f, 0.5f));
		TestTrue(
			TEXT("the doorway stands on the north face"),
			SolidFortressExteriorTestPrivate::FaceMinX(Door) < SolidFortressExteriorTestPrivate::FaceMinX(Building));
	}

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Map && Map->LoadDefault());
	SolidStrongholds::FGrassFace Face;
	const bool bGrass = Map && SolidStrongholds::FindGrassFace(Map, HumbleCell, Face);
	const float GrassYaw = bGrass ? Face.FacingYawDeg : 0.f;
	TestTrue(
		TEXT("the fortress exterior turns toward the site grass"),
		Map && FMath::IsNearlyEqual(Keep->ExteriorYawDeg(Map), FMath::UnwindDegrees(GrassYaw + 180.f), 0.01f));

	const FVector2D Stand = Fortress->OutsideGateXY(nullptr);
	const FVector2D FromCenter = Stand - Fortress->Location;
	TestTrue(
		TEXT("the stand is one hex outside the gate"),
		FMath::IsNearlyEqual(FromCenter.X, Fortress->OuterDepthCm() * 0.5f + SolidStrongholdGrid::FlatToFlatCm, 0.5f)
			&& FMath::IsNearlyZero(FromCenter.Y, 0.5f));
	return true;
}

#endif
