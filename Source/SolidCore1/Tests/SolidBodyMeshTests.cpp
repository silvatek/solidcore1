#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBodyMesh.h"
#include "SolidContentLookup.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "MeshDescription.h"
#include "ReferenceSkeleton.h"
#include "SkeletalMeshAttributes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FVector AverageVertex(USkeletalMesh* Mesh)
	{
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description || Description->Vertices().Num() == 0)
		{
			return FVector::ZeroVector;
		}
		FSkeletalMeshAttributes Attributes(*Description);
		const TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		FVector Sum = FVector::ZeroVector;
		int32 Count = 0;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			Sum += FVector(Positions.Get(VertexID));
			++Count;
		}
		return Count > 0 ? Sum / static_cast<double>(Count) : FVector::ZeroVector;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodySeedNameTest,
	"SolidCore1.Body.SeedName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodySeedNameTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("name seed ignores case"), SolidBodyMesh::SeedForName(TEXT("Sam")), SolidBodyMesh::SeedForName(TEXT("sam")));
	TestTrue(TEXT("different names differ"), SolidBodyMesh::SeedForName(TEXT("Sam")) != SolidBodyMesh::SeedForName(TEXT("Alex")));
	TestEqual(TEXT("captain seed is stable"), SolidBodyMesh::CaptainSeed, 0x0C0A77A1u);

	const FSolidBodyParams Captain = FSolidBodyParams::FromSeed(SolidBodyMesh::CaptainSeed);
	const FSolidBodyParams Sam = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	TestTrue(TEXT("captain draw is in range"), Captain.ExactEquals(Captain.Clamped()));
	TestTrue(TEXT("sam is a different body"), !Captain.ExactEquals(Sam));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyBoneRoleTest,
	"SolidCore1.Body.BoneRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyBoneRoleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("pelvis"), SolidBodyMesh::RoleForBoneName(TEXT("Bip001_Pelvis")) == ESolidBodyBoneRole::Pelvis);
	TestTrue(TEXT("spine"), SolidBodyMesh::RoleForBoneName(TEXT("spine_02")) == ESolidBodyBoneRole::Spine);
	TestTrue(TEXT("neck"), SolidBodyMesh::RoleForBoneName(TEXT("neck_01")) == ESolidBodyBoneRole::Neck);
	TestTrue(TEXT("head"), SolidBodyMesh::RoleForBoneName(TEXT("head")) == ESolidBodyBoneRole::Head);
	TestTrue(TEXT("clavicle"), SolidBodyMesh::RoleForBoneName(TEXT("LeftShoulder")) == ESolidBodyBoneRole::Clavicle);
	TestTrue(TEXT("arm"), SolidBodyMesh::RoleForBoneName(TEXT("upperarm_l")) == ESolidBodyBoneRole::Arm);
	TestTrue(TEXT("hand"), SolidBodyMesh::RoleForBoneName(TEXT("mixamorig_LeftHand")) == ESolidBodyBoneRole::Hand);
	TestTrue(TEXT("thigh"), SolidBodyMesh::RoleForBoneName(TEXT("thigh_r")) == ESolidBodyBoneRole::Leg);
	TestTrue(TEXT("calf"), SolidBodyMesh::RoleForBoneName(TEXT("calf_r")) == ESolidBodyBoneRole::Foot);
	TestTrue(TEXT("ik helper stays put"), SolidBodyMesh::RoleForBoneName(TEXT("ik_foot_root")) == ESolidBodyBoneRole::Root);
	TestTrue(TEXT("unknown stays put"), SolidBodyMesh::RoleForBoneName(TEXT("root")) == ESolidBodyBoneRole::Root);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyVikingBuildTest,
	"SolidCore1.Body.VikingBuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyVikingBuildTest::RunTest(const FString& Parameters)
{
	TestNull(TEXT("null source"), SolidBodyMesh::Build(nullptr, FSolidBodyParams(), GetTransientPackage()));

	USkeletalMesh* Source = SolidContentLookup::FindVikingMesh();
	TestNotNull(TEXT("SK_Viking"), Source);
	if (!Source)
	{
		return false;
	}

	const FReferenceSkeleton& Ref = Source->GetRefSkeleton();
	TestTrue(TEXT("viking has bones"), Ref.GetRawBoneNum() > 0);
	int32 Recognized = 0;
	for (int32 Index = 0; Index < Ref.GetRawBoneNum(); ++Index)
	{
		if (SolidBodyMesh::RoleForBoneName(Ref.GetBoneName(Index).ToString()) != ESolidBodyBoneRole::Root)
		{
			++Recognized;
		}
	}
	TestTrue(TEXT("viking bone names map onto a body"), Recognized > 0);
	if (Recognized == 0)
	{
		const int32 Shown = FMath::Min(Ref.GetRawBoneNum(), 16);
		for (int32 Index = 0; Index < Shown; ++Index)
		{
			AddError(FString::Printf(TEXT("bone %d %s"), Index, *Ref.GetBoneName(Index).ToString()));
		}
		return false;
	}

	USkeleton* SourceSkeleton = Source->GetSkeleton();
	TestNotNull(TEXT("viking skeleton"), SourceSkeleton);
	if (!SourceSkeleton)
	{
		return false;
	}

	int32 ScaledBone = INDEX_NONE;
	ESolidBodyBoneRole ScaledRole = ESolidBodyBoneRole::Root;
	const TArray<FTransform>& SourceBind = Ref.GetRawRefBonePose();
	for (int32 Index = 0; Index < Ref.GetRawBoneNum() && Index < SourceBind.Num(); ++Index)
	{
		const ESolidBodyBoneRole Role = SolidBodyMesh::RoleForBoneName(Ref.GetBoneName(Index).ToString());
		const bool bScaled =
			Role == ESolidBodyBoneRole::Spine
			|| Role == ESolidBodyBoneRole::Neck
			|| Role == ESolidBodyBoneRole::Clavicle
			|| Role == ESolidBodyBoneRole::Arm
			|| Role == ESolidBodyBoneRole::Hand
			|| Role == ESolidBodyBoneRole::Leg
			|| Role == ESolidBodyBoneRole::Foot;
		if (!bScaled || SourceBind[Index].GetTranslation().Size() < 1.f)
		{
			continue;
		}
		ScaledBone = Index;
		ScaledRole = Role;
		break;
	}
	TestTrue(TEXT("viking has a bone whose length can change"), ScaledBone != INDEX_NONE);
	if (ScaledBone == INDEX_NONE)
	{
		return false;
	}

	TArray<FVector> SourceTranslations;
	TArray<uint8> SourceModes;
	SourceTranslations.Reserve(SourceBind.Num());
	SourceModes.Reserve(Ref.GetRawBoneNum());
	for (const FTransform& Pose : SourceBind)
	{
		SourceTranslations.Add(Pose.GetTranslation());
	}
	for (int32 Index = 0; Index < Ref.GetRawBoneNum(); ++Index)
	{
		SourceModes.Add(static_cast<uint8>(
			SourceSkeleton->GetBoneTranslationRetargetingMode(Index, false)));
	}

	FSolidBodyParams Stretched;
	switch (ScaledRole)
	{
	case ESolidBodyBoneRole::Neck:
		Stretched.NeckLength = SolidBody::ParamMax;
		break;
	case ESolidBodyBoneRole::Clavicle:
		Stretched.ShoulderWidth = SolidBody::ParamMax;
		break;
	case ESolidBodyBoneRole::Arm:
	case ESolidBodyBoneRole::Hand:
		Stretched.ArmLength = SolidBody::ParamMax;
		break;
	case ESolidBodyBoneRole::Leg:
	case ESolidBodyBoneRole::Foot:
		Stretched.LegLength = SolidBody::ParamMax;
		break;
	default:
		Stretched.Height = SolidBody::ParamMax;
		break;
	}

	const FSolidBodyParams First = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	const FSolidBodyParams Second = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Alex")));
	USkeletalMesh* Tall = SolidBodyMesh::Build(Source, Stretched, GetTransientPackage());
	USkeletalMesh* SamA = SolidBodyMesh::Build(Source, First, GetTransientPackage());
	USkeletalMesh* SamB = SolidBodyMesh::Build(Source, First, GetTransientPackage());
	USkeletalMesh* Alex = SolidBodyMesh::Build(Source, Second, GetTransientPackage());
	TestNotNull(TEXT("stretched body"), Tall);
	TestNotNull(TEXT("sam body"), SamA);
	TestNotNull(TEXT("sam body again"), SamB);
	TestNotNull(TEXT("alex body"), Alex);
	if (!Tall || !SamA || !SamB || !Alex)
	{
		AddError(TEXT("SK_Viking could not be copied into a generated body."));
		return false;
	}

	USkeleton* BodySkeleton = SamA->GetSkeleton();
	TestNotNull(TEXT("body skeleton"), BodySkeleton);
	TestTrue(TEXT("body skeleton is not the viking asset"), BodySkeleton && BodySkeleton != SourceSkeleton);
	TestEqual(TEXT("viking skeleton object stays"), Source->GetSkeleton(), SourceSkeleton);

	const TArray<FTransform>& SourceBindAfter = Source->GetRefSkeleton().GetRawRefBonePose();
	bool bSourcePoseStayed = SourceBindAfter.Num() == SourceTranslations.Num();
	for (int32 Index = 0; bSourcePoseStayed && Index < SourceTranslations.Num(); ++Index)
	{
		bSourcePoseStayed = SourceBindAfter[Index].GetTranslation().Equals(SourceTranslations[Index], 0.01f);
	}
	TestTrue(TEXT("viking bind pose stays"), bSourcePoseStayed);

	bool bSourceModesStayed = true;
	for (int32 Index = 0; bSourceModesStayed && Index < SourceModes.Num(); ++Index)
	{
		const uint8 Mode = static_cast<uint8>(
			SourceSkeleton->GetBoneTranslationRetargetingMode(Index, false));
		bSourceModesStayed = Mode == SourceModes[Index];
	}
	TestTrue(TEXT("viking retarget modes stay"), bSourceModesStayed);

	if (BodySkeleton)
	{
		const uint8 BodyMode = static_cast<uint8>(
			BodySkeleton->GetBoneTranslationRetargetingMode(ScaledBone, false));
		TestEqual(
			TEXT("scaled bones keep anim rotation and take the new length"),
			BodyMode,
			static_cast<uint8>(EBoneTranslationRetargetingMode::AnimationScaled));
	}

	const TArray<FTransform>& TallBind = Tall->GetRefSkeleton().GetRawRefBonePose();
	const float SourceLength = SourceTranslations[ScaledBone].Size();
	const float TallLength = TallBind.IsValidIndex(ScaledBone)
		? TallBind[ScaledBone].GetTranslation().Size()
		: 0.f;
	TestTrue(
		TEXT("drawn length lands on that bone"),
		TallLength > SourceLength * ((SolidBody::ParamMax + 1.f) * 0.5f));

	const FVector AverageA = AverageVertex(SamA);
	const FVector AverageB = AverageVertex(SamB);
	const FVector AverageAlex = AverageVertex(Alex);
	TestTrue(TEXT("same seed rebuilds the same shape"), AverageA.Equals(AverageB, 0.05f));
	TestFalse(TEXT("another name is another shape"), AverageA.Equals(AverageAlex, 0.5f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
