#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTowerShell.h"
#include "SolidTowerStronghold.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidTowerShellTestPrivate
{
	struct FEnds
	{
		float BottomZ = 0.f;
		float TopZ = 0.f;
		float BottomRadius = 0.f;
		float TopRadius = 0.f;
	};

	bool Measure(const UStaticMeshComponent* Part, FEnds& Out)
	{
		if (!Part)
		{
			return false;
		}
		UStaticMesh* Mesh = Part->GetStaticMesh();
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return false;
		}

		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		float MinZ = TNumericLimits<float>::Max();
		float MaxZ = -MinZ;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const float Z = Positions[VertexID].Z;
			MinZ = FMath::Min(MinZ, Z);
			MaxZ = FMath::Max(MaxZ, Z);
		}

		const FVector Location = Part->GetRelativeLocation();
		const FVector Scale = Part->GetRelativeScale3D();
		Out.BottomZ = Location.Z + MinZ * Scale.Z;
		Out.TopZ = Location.Z + MaxZ * Scale.Z;
		Out.BottomRadius = 0.f;
		Out.TopRadius = 0.f;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			const float Radius = FMath::Sqrt(
				FMath::Square(Position.X * Scale.X) + FMath::Square(Position.Y * Scale.Y));
			if (FMath::IsNearlyEqual(Position.Z, MinZ, 0.05f))
			{
				Out.BottomRadius = FMath::Max(Out.BottomRadius, Radius);
			}
			if (FMath::IsNearlyEqual(Position.Z, MaxZ, 0.05f))
			{
				Out.TopRadius = FMath::Max(Out.TopRadius, Radius);
			}
		}
		return true;
	}

	float CubeBottomZ(const UStaticMeshComponent* Part)
	{
		return Part->GetRelativeLocation().Z - Part->GetRelativeScale3D().Z * 50.f;
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

		FEnds GroundEnds;
		FEnds LastEnds;
		FEnds CapEnds;
		Test.TestTrue(*FString::Printf(TEXT("%s ground mesh"), Label), Measure(Ground, GroundEnds));
		Test.TestTrue(*FString::Printf(TEXT("%s top mesh"), Label), Measure(Last, LastEnds));
		Test.TestTrue(*FString::Printf(TEXT("%s cap mesh"), Label), Measure(Cap, CapEnds));

		Test.TestTrue(
			*FString::Printf(TEXT("%s ground course sits on the actor origin"), Label),
			FMath::IsNearlyEqual(GroundEnds.BottomZ, 0.f, 0.05f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s ground foot is the ground diameter"), Label),
			FMath::IsNearlyEqual(GroundEnds.BottomRadius, Plan.GroundExteriorRadiusCm, 0.05f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s ground course narrows toward its head"), Label),
			GroundEnds.TopRadius < GroundEnds.BottomRadius - 0.5f);
		Test.TestTrue(
			*FString::Printf(TEXT("%s ground head is the plan radius"), Label),
			FMath::IsNearlyEqual(
				GroundEnds.TopRadius,
				SolidTowerStronghold::ExteriorRadiusCm(Plan, GroundEnds.TopZ),
				0.05f));

		FEnds Previous = GroundEnds;
		for (int32 Floor = 1; Floor < Plan.FloorCount; ++Floor)
		{
			UStaticMeshComponent* Course = Shell->GetCourse(Floor);
			FEnds Ends;
			if (!Test.TestTrue(*FString::Printf(TEXT("%s course %d mesh"), Label, Floor), Course && Measure(Course, Ends)))
			{
				return;
			}
			Test.TestTrue(
				*FString::Printf(TEXT("%s course %d meets the course below"), Label, Floor),
				FMath::IsNearlyEqual(Ends.BottomZ, Previous.TopZ, 0.05f)
					&& FMath::IsNearlyEqual(Ends.BottomRadius, Previous.TopRadius, 0.05f));
			Test.TestTrue(
				*FString::Printf(TEXT("%s course %d head is the plan radius"), Label, Floor),
				FMath::IsNearlyEqual(
					Ends.TopRadius,
					SolidTowerStronghold::ExteriorRadiusCm(Plan, Ends.TopZ),
					0.05f));
			Previous = Ends;
		}

		Test.TestTrue(
			*FString::Printf(TEXT("%s cap meets the top course"), Label),
			FMath::IsNearlyEqual(CapEnds.BottomZ, LastEnds.TopZ, 0.05f)
				&& FMath::IsNearlyEqual(CapEnds.BottomRadius, LastEnds.TopRadius, 0.05f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s cap top is the tower height"), Label),
			FMath::IsNearlyEqual(CapEnds.TopZ, Plan.HeightCm(), 0.05f));
		Test.TestTrue(
			*FString::Printf(TEXT("%s cap is the top diameter"), Label),
			FMath::IsNearlyEqual(CapEnds.TopRadius, Plan.TopExteriorRadiusCm, 0.05f));

		UStaticMesh* DoorMesh = Door->GetStaticMesh();
		Test.TestNotNull(*FString::Printf(TEXT("%s door cube"), Label), DoorMesh);
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
			FMath::IsNearlyEqual(CubeBottomZ(Door), 0.f, 0.5f));
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
