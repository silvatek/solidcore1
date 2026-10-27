#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Fortress/Actors/SolidFortressInterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/SolidStronghold.h"
#include "SolidGameState.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFortressInteriorTest,
	"SolidCore1.Stronghold.FortressInterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFortressInteriorTest::RunTest(const FString& Parameters)
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
		TEXT("Humble names the fortress interior"),
		Keep->InteriorClass() == ASolidFortressInterior::StaticClass());

	ASolidFortressInterior* Room = NewObject<ASolidFortressInterior>();
	TestNotNull(TEXT("fortress interior"), Room);
	if (!Room)
	{
		return false;
	}
	Keep->ConfigureInterior(Room);
	TestEqual(TEXT("the visit stands on the fortress column"), Room->GetStrongholdCell().X, HumbleCell.X);
	TestEqual(TEXT("the visit stands on the fortress row"), Room->GetStrongholdCell().Y, HumbleCell.Y);
	TestEqual(TEXT("the opening is the gate"), Room->GetPassageMesh(), Room->GetGateMesh());
	TestNotNull(TEXT("the gate"), Room->GetGateMesh());
	TestEqual(TEXT("the outer wall is six stretches"), Room->GetCurtainMeshes().Num(), 6);
	if (!Room->GetGateMesh() || Room->GetCurtainMeshes().Num() != 6)
	{
		return false;
	}

	const float HalfWidth = Fortress->OuterWidthCm() * 0.5f;
	const float HalfDepth = Fortress->OuterDepthCm() * 0.5f;
	float MinX = TNumericLimits<float>::Max();
	float MaxX = -MinX;
	float MinY = MinX;
	float MaxY = -MinX;
	float TopZ = 0.f;
	for (const TObjectPtr<UStaticMeshComponent>& Part : Room->GetCurtainMeshes())
	{
		if (!Part)
		{
			continue;
		}
		const FVector Location = Part->GetRelativeLocation();
		const FVector Scale = Part->GetRelativeScale3D();
		MinX = FMath::Min(MinX, Location.X - Scale.X * 50.f);
		MaxX = FMath::Max(MaxX, Location.X + Scale.X * 50.f);
		MinY = FMath::Min(MinY, Location.Y - Scale.Y * 50.f);
		MaxY = FMath::Max(MaxY, Location.Y + Scale.Y * 50.f);
		TopZ = FMath::Max(TopZ, Location.Z + Scale.Z * 50.f);
	}

	TestTrue(
		TEXT("the ring matches the outer face"),
		FMath::IsNearlyEqual(MinX, -HalfDepth, 0.5f)
			&& FMath::IsNearlyEqual(MaxX, HalfDepth, 0.5f)
			&& FMath::IsNearlyEqual(MinY, -HalfWidth, 0.5f)
			&& FMath::IsNearlyEqual(MaxY, HalfWidth, 0.5f)
			&& FMath::IsNearlyEqual(TopZ, Fortress->CurtainHeightCm(), 0.5f));

	const UStaticMeshComponent* Gate = Room->GetGateMesh();
	TestTrue(
		TEXT("the gate is the storey opening on local -X"),
		Gate->GetRelativeLocation().X < 0.f
			&& FMath::IsNearlyZero(Gate->GetRelativeLocation().Y, 0.5f)
			&& FMath::IsNearlyEqual(Gate->GetRelativeScale3D().Y * 100.f, FSolidStronghold::OpeningCm, 0.5f)
			&& FMath::IsNearlyEqual(Gate->GetRelativeScale3D().Z * 100.f, FSolidStronghold::StoreyCm, 0.5f)
			&& FMath::IsNearlyZero(Gate->GetRelativeLocation().Z - Gate->GetRelativeScale3D().Z * 50.f, 0.5f));

	const FVector Spawn = Room->GetCaptainSpawnWorld();
	TestTrue(
		TEXT("the captain starts one hex inside the gate"),
		FMath::IsNearlyEqual(
			Spawn.X,
			FSolidStronghold::WallCm + SolidStrongholdGrid::FlatToFlatCm - HalfDepth,
			0.5f)
			&& FMath::IsNearlyZero(Spawn.Y, 0.5f)
			&& FMath::IsNearlyZero(Spawn.Z, 0.5f));
	TestTrue(TEXT("the arrival looks into the yard"), FMath::IsNearlyZero(Room->GetCaptainView().Yaw, 0.1f));

	USolidGameState& State = *NewObject<USolidGameState>();
	TestFalse(TEXT("a shut fortress refuses the gate"), Keep->AcceptsEnter(State, Keep.Get(), HumbleCell));
	State.bStrongholdsEnabled = true;
	TestTrue(TEXT("the open fortress accepts its gate"), Keep->AcceptsEnter(State, Keep.Get(), HumbleCell));
	TestFalse(
		TEXT("another cell is not the gate"),
		Keep->AcceptsEnter(State, Keep.Get(), FIntPoint(1, 1)));

	Keep->RememberExit(nullptr);
	const SolidStronghold::FExitStand Leave = SolidStronghold::ExitStand(*Keep);
	TestTrue(
		TEXT("leaving stands one hex outside the gate"),
		Leave.XY.Equals(Fortress->OutsideGateXY(nullptr), 0.5f));
	TestTrue(TEXT("leaving faces away along the grass"), FMath::IsNearlyEqual(Leave.YawDeg, 0.f, 0.1f));
	return true;
}

#endif
