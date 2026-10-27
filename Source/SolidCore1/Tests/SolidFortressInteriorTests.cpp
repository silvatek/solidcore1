#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Fortress/Actors/SolidFortressInterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/SolidStronghold.h"
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
	TestNull(TEXT("the visit has no opening"), Room->GetPassageMesh());
	TestEqual(TEXT("the outer wall is four stretches"), Room->GetCurtainMeshes().Num(), 4);
	if (Room->GetCurtainMeshes().Num() != 4)
	{
		return false;
	}

	const float HalfWidth = Fortress->OuterWidthCm() * 0.5f;
	const float HalfDepth = Fortress->OuterDepthCm() * 0.5f;
	float MinX = TNumericLimits<float>::Max();
	float MaxX = -MinX;
	float MinY = MinX;
	float MaxY = -MinX;
	const UStaticMeshComponent* GateSide = nullptr;
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
		TestTrue(
			TEXT("each stretch is the curtain height"),
			FMath::IsNearlyEqual(Location.Z + Scale.Z * 50.f, Fortress->CurtainHeightCm(), 0.5f)
				&& FMath::IsNearlyZero(Location.Z - Scale.Z * 50.f, 0.5f));
		if (!GateSide || Location.X < GateSide->GetRelativeLocation().X)
		{
			GateSide = Part;
		}
	}

	TestTrue(
		TEXT("the ring matches the outer face"),
		FMath::IsNearlyEqual(MinX, -HalfDepth, 0.5f)
			&& FMath::IsNearlyEqual(MaxX, HalfDepth, 0.5f)
			&& FMath::IsNearlyEqual(MinY, -HalfWidth, 0.5f)
			&& FMath::IsNearlyEqual(MaxY, HalfWidth, 0.5f));
	TestTrue(
		TEXT("the gate side is a full wall"),
		GateSide && GateSide->GetRelativeLocation().X < 0.f
			&& FMath::IsNearlyEqual(GateSide->GetRelativeScale3D().Y * 100.f, Fortress->OuterWidthCm(), 0.5f));
	return true;
}

#endif
