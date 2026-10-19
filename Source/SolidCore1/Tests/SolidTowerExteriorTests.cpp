#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Tower/SolidTowerExterior.h"
#include "SolidStrongholds.h"
#include "Strongholds/Tower/SolidTowerStronghold.h"
#include "SolidWorldMap.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidTowerExteriorTestPrivate
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
		ASolidTowerExterior* Shell = NewObject<ASolidTowerExterior>();
		Test.TestNotNull(*FString::Printf(TEXT("%s shell"), Label), Shell);
		if (!Shell)
		{
			return;
		}

		Shell->SetPlan(Plan);
		Shell->BuildVisuals();

		UStaticMeshComponent* Cap = Shell->GetCapMesh();
		UStaticMeshComponent* Door = Shell->GetDoorMesh();
		Test.TestNotNull(*FString::Printf(TEXT("%s cap"), Label), Cap);
		Test.TestNotNull(*FString::Printf(TEXT("%s door"), Label), Door);
		if (!Cap || !Door)
		{
			return;
		}

		FEnds CapEnds;
		Test.TestTrue(*FString::Printf(TEXT("%s cap mesh"), Label), Measure(Cap, CapEnds));
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
			*FString::Printf(TEXT("%s door blocks"), Label),
			Door->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerExteriorTest,
	"SolidCore1.Stronghold.TowerExterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerExteriorTest::RunTest(const FString& Parameters)
{
	SolidTowerExteriorTestPrivate::TestShell(*this, SolidTowerStronghold::Imperious, TEXT("Imperious"));
	SolidTowerExteriorTestPrivate::TestShell(*this, SolidTowerStronghold::Average, TEXT("Average"));

	TestTrue(TEXT("north grass puts the door north"),
		FMath::IsNearlyEqual(SolidTowerStronghold::ExteriorYawDeg(90.f), -90.f, 0.01f));
	TestTrue(TEXT("east grass puts the door east"),
		FMath::IsNearlyEqual(SolidTowerStronghold::ExteriorYawDeg(0.f), 180.f, 0.01f));

	const FIntPoint ImperiousCell(35, 15);
	TUniquePtr<FSolidStronghold> Spire = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Imperious,
		ImperiousCell,
		SolidStronghold::WorldXY(ImperiousCell));
	TestNotNull(TEXT("an Imperious tower"), Spire.Get());
	if (!Spire)
	{
		return false;
	}
	TestTrue(
		TEXT("the tower names the tower exterior"),
		Spire->ExteriorClass() == ASolidTowerExterior::StaticClass());
	TestTrue(
		TEXT("no map leaves the tower door on local -X at yaw 180"),
		FMath::IsNearlyEqual(Spire->ExteriorYawDeg(nullptr), SolidTowerStronghold::ExteriorYawDeg(0.f), 0.01f));

	ASolidTowerExterior* Shell = NewObject<ASolidTowerExterior>();
	TestNotNull(TEXT("tower exterior"), Shell);
	if (!Shell)
	{
		return false;
	}
	Spire->ConfigureExterior(Shell);
	TestEqual(TEXT("the exterior stands on the tower column"), Shell->GetStrongholdCell().X, ImperiousCell.X);
	TestEqual(TEXT("the exterior stands on the tower row"), Shell->GetStrongholdCell().Y, ImperiousCell.Y);
	TestEqual(
		TEXT("the exterior uses the Imperious plan"),
		Shell->GetPlan().FloorCount,
		SolidTowerStronghold::Imperious.FloorCount);
	UStaticMeshComponent* Door = Shell->GetDoorMesh();
	TestNotNull(TEXT("the tower stronghold builds the door"), Door);

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Map && Map->LoadDefault());
	SolidStrongholds::FGrassFace Face;
	const bool bGrass = Map && SolidStrongholds::FindGrassFace(Map, ImperiousCell, Face);
	const float GrassYaw = bGrass ? Face.FacingYawDeg : 0.f;
	TestTrue(
		TEXT("the tower exterior turns toward the site grass"),
		Map && FMath::IsNearlyEqual(
			Spire->ExteriorYawDeg(Map),
			SolidTowerStronghold::ExteriorYawDeg(GrassYaw),
			0.01f));
	if (Map)
	{
		Spire->RememberExit(Map);
		TestEqual(
			TEXT("the tower exit faces that grass"),
			static_cast<uint8>(Spire->Orientation),
			static_cast<uint8>(SolidStronghold::OrientationFromYaw(GrassYaw)));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
