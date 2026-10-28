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
	TestEqual(TEXT("Humble draws one building"), Shell->GetBuildingMeshes().Num(), 1);
	if (!Gate || Shell->GetCurtainMeshes().Num() != 6 || Shell->GetBuildingMeshes().Num() != 1)
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

	const UStaticMeshComponent* Building = Shell->GetBuildingMeshes()[0];
	const float BuildingTop = Fortress->Buildings[0].HeightCm(Fortress->ParapetCm);
	TestTrue(
		TEXT("the building stands above the wall"),
		FMath::IsNearlyEqual(SolidFortressExteriorTestPrivate::TopZ(Building), BuildingTop, 0.5f)
			&& BuildingTop > CurtainTop + 1.f);
	TestTrue(
		TEXT("the building meets the inner south curtain"),
		FMath::IsNearlyEqual(SolidFortressExteriorTestPrivate::FaceMaxX(Building), HalfDepth - FSolidStronghold::WallCm, 0.5f));

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
