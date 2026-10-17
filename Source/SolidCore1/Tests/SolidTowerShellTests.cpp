#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTowerShell.h"
#include "SolidTowerStronghold.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidTowerShellTestPrivate
{
	float BottomZ(const UStaticMeshComponent* Part)
	{
		return Part->GetRelativeLocation().Z - Part->GetRelativeScale3D().Z * 50.f;
	}

	float TopZ(const UStaticMeshComponent* Part)
	{
		return Part->GetRelativeLocation().Z + Part->GetRelativeScale3D().Z * 50.f;
	}

	void TestShell(
		FAutomationTestBase& Test,
		const SolidTowerStronghold::FPlan& Plan,
		const TCHAR* Label)
	{
		ASolidTowerShell* Shell = NewObject<ASolidTowerShell>();
		Test.TestNotNull(*FString::Printf(TEXT("%s shell"), Label), Shell);
		if (!Shell)
		{
			return;
		}

		Shell->SetPlan(Plan);
		Shell->BuildVisuals();

		Test.TestEqual(*FString::Printf(TEXT("%s course count"), Label), Shell->GetCourseCount(), Plan.FloorCount);

		UStaticMeshComponent* Ground = Shell->GetCourse(0);
		UStaticMeshComponent* Last = Shell->GetCourse(Plan.FloorCount - 1);
		UStaticMeshComponent* Cap = Shell->GetCapMesh();
		UStaticMeshComponent* Door = Shell->GetDoorMesh();
		Test.TestNotNull(*FString::Printf(TEXT("%s ground course"), Label), Ground);
		Test.TestNotNull(*FString::Printf(TEXT("%s top course"), Label), Last);
		Test.TestNotNull(*FString::Printf(TEXT("%s cap"), Label), Cap);
		Test.TestNotNull(*FString::Printf(TEXT("%s door"), Label), Door);
		if (!Ground || !Last || !Cap || !Door)
		{
			return;
		}

		UStaticMesh* GroundMesh = Ground->GetStaticMesh();
		UStaticMesh* CapMesh = Cap->GetStaticMesh();
		UStaticMesh* DoorMesh = Door->GetStaticMesh();
		Test.TestNotNull(*FString::Printf(TEXT("%s course cylinder"), Label), GroundMesh);
		Test.TestNotNull(*FString::Printf(TEXT("%s cap cylinder"), Label), CapMesh);
		Test.TestNotNull(*FString::Printf(TEXT("%s door cube"), Label), DoorMesh);

		const float GroundScale = Plan.GroundExteriorRadiusCm / 50.f;
		Test.TestTrue(
			*FString::Printf(TEXT("%s ground course is the ground diameter"), Label),
			FMath::IsNearlyEqual(Ground->GetRelativeScale3D().X, GroundScale, 0.01f)
				&& FMath::IsNearlyEqual(Ground->GetRelativeScale3D().Y, GroundScale, 0.01f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s ground course sits on the actor origin"), Label),
			FMath::IsNearlyEqual(BottomZ(Ground), 0.f, 0.5f));

		const float LastBottom = SolidTowerStronghold::FloorZCm(Plan, Plan.FloorCount - 1);
		const float LastRadius = SolidTowerStronghold::ExteriorRadiusCm(Plan, LastBottom);
		Test.TestTrue(
			*FString::Printf(TEXT("%s top course is narrower than the ground"), Label),
			Last->GetRelativeScale3D().X < Ground->GetRelativeScale3D().X - 0.1f);
		Test.TestTrue(
			*FString::Printf(TEXT("%s top course uses the radius at its foot"), Label),
			FMath::IsNearlyEqual(Last->GetRelativeScale3D().X, LastRadius / 50.f, 0.01f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s top course stops under the cap"), Label),
			FMath::IsNearlyEqual(TopZ(Last), Plan.HeightCm() - Plan.SlabCm, 0.5f));

		const float CapScale = Plan.TopExteriorRadiusCm / 50.f;
		Test.TestTrue(
			*FString::Printf(TEXT("%s cap is the top diameter"), Label),
			FMath::IsNearlyEqual(Cap->GetRelativeScale3D().X, CapScale, 0.01f)
				&& FMath::IsNearlyEqual(Cap->GetRelativeScale3D().Y, CapScale, 0.01f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s cap top is the tower height"), Label),
			FMath::IsNearlyEqual(TopZ(Cap), Plan.HeightCm(), 0.5f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s cap meets the top course"), Label),
			FMath::IsNearlyEqual(BottomZ(Cap), TopZ(Last), 0.5f));

		const FVector DoorCenter = SolidTowerStronghold::DoorSlabCenterCm(Plan);
		Test.TestTrue(
			*FString::Printf(TEXT("%s door is on local -X"), Label),
			Door->GetRelativeLocation().X < 0.f
				&& FMath::IsNearlyEqual(Door->GetRelativeLocation().X, DoorCenter.X, 0.5f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s door is the plan opening"), Label),
			FMath::IsNearlyEqual(Door->GetRelativeScale3D().Y * 100.f, Plan.DoorWidthCm, 0.5f)
				&& FMath::IsNearlyEqual(Door->GetRelativeScale3D().Z * 100.f, Plan.DoorHeightCm, 0.5f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s door meets the ground"), Label),
			FMath::IsNearlyEqual(BottomZ(Door), 0.f, 0.5f));
		const float OuterFace = Door->GetRelativeLocation().X - Door->GetRelativeScale3D().X * 50.f;
		Test.TestTrue(
			*FString::Printf(TEXT("%s door stands proud of the ground course"), Label),
			FMath::IsNearlyEqual(OuterFace, -(Plan.GroundExteriorRadiusCm + SolidTowerStronghold::DoorProudCm), 0.5f));

		Test.TestTrue(
			*FString::Printf(TEXT("%s stone blocks"), Label),
			Ground->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
		Test.TestTrue(
			*FString::Printf(TEXT("%s door blocks"), Label),
			Door->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerShellTest,
	"SolidCore1.Stronghold.TowerShell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerShellTest::RunTest(const FString& Parameters)
{
	SolidTowerShellTestPrivate::TestShell(*this, SolidTowerStronghold::Imperious, TEXT("Imperious"));
	SolidTowerShellTestPrivate::TestShell(*this, SolidTowerStronghold::Average, TEXT("Average"));

	TestTrue(TEXT("north grass puts the door north"),
		FMath::IsNearlyEqual(SolidTowerStronghold::ShellYawDeg(90.f), -90.f, 0.01f));
	TestTrue(TEXT("east grass puts the door east"),
		FMath::IsNearlyEqual(SolidTowerStronghold::ShellYawDeg(0.f), 180.f, 0.01f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
