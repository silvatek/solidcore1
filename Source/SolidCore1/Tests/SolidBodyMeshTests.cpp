#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBodyMesh.h"
#include "Character/SolidSkeletonHover.h"
#include "SolidContentLookup.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "MeshDescription.h"
#include "ReferenceSkeleton.h"
#include "Rendering/SkeletalMeshRenderData.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	int32 RenderVertexCount(const USkeletalMesh* Mesh)
	{
		const FSkeletalMeshRenderData* RenderData = Mesh ? Mesh->GetResourceForRendering() : nullptr;
		if (!RenderData || RenderData->LODRenderData.Num() == 0)
		{
			return 0;
		}
		return static_cast<int32>(RenderData->LODRenderData[0].GetNumVertices());
	}

	int32 DescriptionVertexCount(USkeletalMesh* Mesh)
	{
		const FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		return Description ? Description->Vertices().Num() : 0;
	}

	float BindDelta(const USkeletalMesh* A, const USkeletalMesh* B)
	{
		if (!A || !B)
		{
			return 1.e6f;
		}
		const TArray<FTransform>& PoseA = A->GetRefSkeleton().GetRawRefBonePose();
		const TArray<FTransform>& PoseB = B->GetRefSkeleton().GetRawRefBonePose();
		if (PoseA.Num() != PoseB.Num())
		{
			return 1.e6f;
		}
		float Delta = 0.f;
		for (int32 Index = 0; Index < PoseA.Num(); ++Index)
		{
			Delta = FMath::Max(Delta, static_cast<float>((PoseA[Index].GetTranslation() - PoseB[Index].GetTranslation()).Size()));
		}
		return Delta;
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
	TestFalse(TEXT("calf is not a toe"), SolidBodyMesh::IsDigitBoneName(TEXT("calf_r")));
	TestFalse(TEXT("ankle is not a toe"), SolidBodyMesh::IsDigitBoneName(TEXT("foot_l")));
	TestTrue(TEXT("toe is a digit"), SolidBodyMesh::IsDigitBoneName(TEXT("ball_l")));
	TestTrue(TEXT("thumb is a digit"), SolidBodyMesh::IsDigitBoneName(TEXT("thumb_01_l")));
	TestTrue(TEXT("upper-arm twist is not a shoulder plate"), SolidBodyMesh::IsTwistBoneName(TEXT("upperarm_twist_01_l")));
	TestTrue(TEXT("calf twist is not a shin"), SolidBodyMesh::IsTwistBoneName(TEXT("calf_twist_02_r")));
	TestFalse(TEXT("calf is not a twist"), SolidBodyMesh::IsTwistBoneName(TEXT("calf_r")));
	TestTrue(TEXT("hip bottom reaches the thigh"), FMath::IsNearlyEqual(SolidBodyMesh::HipBottomRadius(10.f, 8.f, 9.f), 17.f));
	TestTrue(TEXT("wide pelvis stays wider than the thigh"), FMath::IsNearlyEqual(SolidBodyMesh::HipBottomRadius(20.f, 4.f, 6.f), 20.f));
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
	USkeletalMesh* Neutral = SolidBodyMesh::Build(Source, FSolidBodyParams(), GetTransientPackage());
	USkeletalMesh* Tall = SolidBodyMesh::Build(Source, Stretched, GetTransientPackage());
	USkeletalMesh* SamA = SolidBodyMesh::Build(Source, First, GetTransientPackage());
	USkeletalMesh* SamB = SolidBodyMesh::Build(Source, First, GetTransientPackage());
	USkeletalMesh* Alex = SolidBodyMesh::Build(Source, Second, GetTransientPackage());
	TestNotNull(TEXT("neutral body"), Neutral);
	TestNotNull(TEXT("stretched body"), Tall);
	TestNotNull(TEXT("sam body"), SamA);
	TestNotNull(TEXT("sam body again"), SamB);
	TestNotNull(TEXT("alex body"), Alex);
	if (!Neutral || !Tall || !SamA || !SamB || !Alex)
	{
		AddError(TEXT("A low-poly body could not be built on the Viking skeleton."));
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

	const TArray<FTransform>& NeutralBind = Neutral->GetRefSkeleton().GetRawRefBonePose();
	const TArray<FTransform>& TallBind = Tall->GetRefSkeleton().GetRawRefBonePose();
	bool bNeutralMatchesViking = NeutralBind.Num() == SourceTranslations.Num();
	for (int32 Index = 0; bNeutralMatchesViking && Index < SourceTranslations.Num(); ++Index)
	{
		bNeutralMatchesViking = FMath::IsNearlyEqual(
			NeutralBind[Index].GetTranslation().Size(),
			SourceTranslations[Index].Size(),
			0.05f);
	}
	TestTrue(TEXT("neutral sliders keep every viking bone length"), bNeutralMatchesViking);
	const float NeutralLength = NeutralBind.IsValidIndex(ScaledBone)
		? NeutralBind[ScaledBone].GetTranslation().Size()
		: 0.f;
	const float TallLength = TallBind.IsValidIndex(ScaledBone)
		? TallBind[ScaledBone].GetTranslation().Size()
		: 0.f;
	TestTrue(TEXT("neutral length is a real segment"), NeutralLength > 1.f);
	TestTrue(
		TEXT("drawn length lands on that bone"),
		TallLength > NeutralLength * ((SolidBody::ParamMax + 1.f) * 0.5f));

	const int32 SamVerts = RenderVertexCount(SamA);
	const int32 SourceVerts = RenderVertexCount(Source);
	TestTrue(TEXT("low-poly body has a render lod"), SamVerts > 0);
	TestTrue(TEXT("generated body stays within the polygon budget"), SamVerts < 16000);
	if (SourceVerts > 0)
	{
		TestTrue(TEXT("generated body is lighter than the viking sculpt"), SamVerts < SourceVerts);
	}
	TestEqual(TEXT("same seed rebuilds the same vertex count"), RenderVertexCount(SamB), SamVerts);
	TestTrue(TEXT("body keeps the viking bone count"),
		SamA->GetRefSkeleton().GetRawBoneNum() == Source->GetRefSkeleton().GetRawBoneNum());
	TestTrue(TEXT("body has an inverse bind"), SamA->GetRefBasesInvMatrix().Num() > 0);
	TestTrue(TEXT("torso and legs are separate slots"), SamA->GetMaterials().Num() >= 2);
	if (SamA->GetMaterials().Num() >= 2)
	{
		TestEqual(TEXT("torso slot"), SamA->GetMaterials()[0].MaterialSlotName, FName(TEXT("Torso")));
		TestEqual(TEXT("legs slot"), SamA->GetMaterials()[1].MaterialSlotName, FName(TEXT("Legs")));
	}

	const int32 Described = DescriptionVertexCount(SamA);
	if (Described > 0)
	{
		TestTrue(TEXT("described body stays within the polygon budget"), Described < 16000);
		TestEqual(TEXT("same seed rebuilds the same description"), DescriptionVertexCount(SamB), Described);
	}

	TestTrue(TEXT("same seed rebuilds the same bind"), BindDelta(SamA, SamB) < 0.05f);
	TestTrue(TEXT("another name is another bind"), BindDelta(SamA, Alex) > 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyFaceTest,
	"SolidCore1.Body.Face",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyFaceTest::RunTest(const FString& Parameters)
{
	const FString Neutral = SolidBodyMesh::FaceStringForCodepoint(0x1F611u);
	TestEqual(TEXT("U+1F611 is a surrogate pair"), Neutral.Len(), 2);
	if (Neutral.Len() >= 2)
	{
		TestEqual(TEXT("U+1F611 high surrogate"), static_cast<uint32>(Neutral[0]), 0xD83Du);
		TestEqual(TEXT("U+1F611 low surrogate"), static_cast<uint32>(Neutral[1]), 0xDE11u);
	}
	TestEqual(TEXT("a plane character stays one unit"), SolidBodyMesh::FaceStringForCodepoint(0x41u).Len(), 1);

	TestEqual(
		TEXT("face ignores case"),
		SolidBodyMesh::FaceCodepointForName(TEXT("Sam")),
		SolidBodyMesh::FaceCodepointForName(TEXT("sam")));

	const uint32 Known[] = {
		0x1F610u,
		0x1F611u,
		0x1F642u,
		0x1F641u,
		0x1F612u,
		0x1F644u,
	};
	auto IsKnownFace = [&Known](const uint32 Codepoint)
	{
		for (const uint32 Face : Known)
		{
			if (Face == Codepoint)
			{
				return true;
			}
		}
		return false;
	};
	TestTrue(TEXT("Sam picks a face glyph"), IsKnownFace(SolidBodyMesh::FaceCodepointForName(TEXT("Sam"))));
	TestTrue(TEXT("Alex picks a face glyph"), IsKnownFace(SolidBodyMesh::FaceCodepointForName(TEXT("Alex"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodySkinSlotTest,
	"SolidCore1.Body.SkinSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodySkinSlotTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* Source = SolidContentLookup::FindVikingMesh();
	TestNotNull(TEXT("SK_Viking"), Source);
	if (!Source)
	{
		return false;
	}

	const FSolidBodyParams Params = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	USkeletalMesh* Body = SolidBodyMesh::Build(Source, Params, GetTransientPackage());
	TestNotNull(TEXT("body"), Body);
	if (!Body)
	{
		return false;
	}

	const TArray<FSkeletalMaterial>& Materials = Body->GetMaterials();
	TestTrue(TEXT("head and hands have their own slot"), Materials.Num() >= 3);
	if (Materials.Num() >= 3)
	{
		TestEqual(TEXT("skin slot"), Materials[2].MaterialSlotName, FName(TEXT("Skin")));
	}
	if (Materials.Num() >= 1)
	{
		TestEqual(TEXT("shirt slot stays first"), Materials[0].MaterialSlotName, FName(TEXT("Torso")));
	}
	if (Materials.Num() >= 2)
	{
		TestEqual(TEXT("trouser slot stays second"), Materials[1].MaterialSlotName, FName(TEXT("Legs")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyConeJointTest,
	"SolidCore1.Body.ConeJoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyConeJointTest::RunTest(const FString& Parameters)
{
	const FSolidBodyParams Average;
	const SolidBodyMesh::FSolidConeProfile Profile = SolidBodyMesh::ConeProfile(Average);
	auto Same = [](const float A, const float B)
	{
		return FMath::IsNearlyEqual(A, B, 0.02f);
	};

	TestTrue(TEXT("split width is the outer span"), Same(SolidBodyMesh::SplitHalfWidth(8.f, 9.f), 17.f));
	TestTrue(TEXT("a negative offset does not shrink the child"), Same(SolidBodyMesh::SplitHalfWidth(-3.f, 4.f), 4.f));
	TestTrue(
		TEXT("chin ring matches the skull"),
		Same(
			SolidBodyMesh::HeadChinRadius(Average),
			SolidBody::HeadLengthCm * SolidBodyMesh::HeadRadiusOverHeight * SolidBodyMesh::HeadChinSlice));
	TestTrue(TEXT("jaw is that chin ring"), Same(Profile.Jaw, SolidBodyMesh::HeadChinRadius(Average)));
	TestTrue(TEXT("one spine is the chest"), Same(SolidBodyMesh::SpineJointRadius(Profile.Waist, Profile.Chest, 0, 0), Profile.Chest));
	TestTrue(TEXT("a chain starts at the waist"), Same(SolidBodyMesh::SpineJointRadius(Profile.Waist, Profile.Chest, 0, 2), Profile.Waist));
	TestTrue(TEXT("a chain ends at the chest"), Same(SolidBodyMesh::SpineJointRadius(Profile.Waist, Profile.Chest, 2, 2), Profile.Chest));

	FSolidBodyParams Bulkier;
	Bulkier.Bulk = 1.5f;
	const SolidBodyMesh::FSolidConeProfile BulkProfile = SolidBodyMesh::ConeProfile(Bulkier);
	TestTrue(TEXT("bulk thickens the thigh"), Same(BulkProfile.Thigh, SolidBodyMesh::JointThigh * 1.5f));
	TestTrue(TEXT("bulk leaves the waist"), Same(BulkProfile.Waist, Profile.Waist));

	FSolidBodyParams Thicker;
	Thicker.TorsoGirth = 1.25f;
	TestTrue(
		TEXT("girth thickens the chest"),
		Same(SolidBodyMesh::ConeProfile(Thicker).Chest, SolidBodyMesh::JointChest * 1.25f));

	TArray<FSolidDeformBone> Bones;
	TArray<FVector> Positions;
	auto Add = [&Bones, &Positions](const int32 Parent, const ESolidBodyBoneRole Role, const FVector& At)
	{
		FSolidDeformBone Bone;
		Bone.ParentIndex = Parent;
		Bone.Role = Role;
		Bones.Add(Bone);
		Positions.Add(At);
		return Bones.Num() - 1;
	};

	const int32 Pelvis = Add(INDEX_NONE, ESolidBodyBoneRole::Pelvis, FVector(0.f, 0.f, 0.f));
	const int32 Waist = Add(Pelvis, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 22.f));
	const int32 Mid = Add(Waist, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 44.f));
	const int32 Chest = Add(Mid, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 66.f));
	const int32 Neck = Add(Chest, ESolidBodyBoneRole::Neck, FVector(0.f, 0.f, 88.f));
	const int32 Head = Add(Neck, ESolidBodyBoneRole::Head, FVector(0.f, 0.f, 110.f));
	const int32 ClavicleL = Add(Chest, ESolidBodyBoneRole::Clavicle, FVector(-8.f, 0.f, 88.f));
	const int32 UpperL = Add(ClavicleL, ESolidBodyBoneRole::Arm, FVector(-16.f, 0.f, 88.f));
	const int32 ElbowL = Add(UpperL, ESolidBodyBoneRole::Arm, FVector(-16.f, 0.f, 60.f));
	const int32 WristL = Add(ElbowL, ESolidBodyBoneRole::Hand, FVector(-16.f, 0.f, 36.f));
	const int32 ClavicleR = Add(Chest, ESolidBodyBoneRole::Clavicle, FVector(8.f, 0.f, 88.f));
	const int32 UpperR = Add(ClavicleR, ESolidBodyBoneRole::Arm, FVector(16.f, 0.f, 88.f));
	const int32 ThighL = Add(Pelvis, ESolidBodyBoneRole::Leg, FVector(-10.f, 0.f, 0.f));
	const int32 KneeL = Add(ThighL, ESolidBodyBoneRole::Foot, FVector(-10.f, 0.f, -40.f));
	const int32 AnkleL = Add(KneeL, ESolidBodyBoneRole::Foot, FVector(-10.f, 0.f, -62.f));
	const int32 ThighR = Add(Pelvis, ESolidBodyBoneRole::Leg, FVector(10.f, 0.f, 0.f));
	Add(ThighR, ESolidBodyBoneRole::Foot, FVector(10.f, 0.f, -40.f));

	TArray<float> Radii;
	SolidBodyMesh::JointRadii(Bones, Positions, Average, Radii);

	float IntoWaist = 0.f;
	float WaistEnd = 0.f;
	float OutOfWaist = 0.f;
	float IntoMid = 0.f;
	float MidEnd = 0.f;
	float OutOfMid = 0.f;
	SolidBodyMesh::SegmentRadii(Radii, Pelvis, Waist, IntoWaist, WaistEnd);
	SolidBodyMesh::SegmentRadii(Radii, Waist, Mid, OutOfWaist, IntoMid);
	SolidBodyMesh::SegmentRadii(Radii, Mid, Chest, MidEnd, OutOfMid);
	TestTrue(TEXT("abdomen meets the waist"), Same(WaistEnd, OutOfWaist));
	TestTrue(TEXT("waist meets the next spine"), Same(IntoMid, MidEnd));
	TestTrue(TEXT("chest cone leaves the middle spine"), Same(OutOfMid, Radii[Chest]));

	float ThighStart = 0.f;
	float KneeFromThigh = 0.f;
	float KneeFromShin = 0.f;
	float AnkleFromShin = 0.f;
	SolidBodyMesh::SegmentRadii(Radii, ThighL, KneeL, ThighStart, KneeFromThigh);
	SolidBodyMesh::SegmentRadii(Radii, KneeL, AnkleL, KneeFromShin, AnkleFromShin);
	TestTrue(TEXT("thigh meets shin"), Same(KneeFromThigh, KneeFromShin));
	TestTrue(TEXT("shin meets the ankle"), Same(AnkleFromShin, Profile.Ankle));
	TestTrue(TEXT("both thighs share a radius"), Same(Radii[ThighL], Radii[ThighR]));
	TestTrue(
		TEXT("hips match the combined thighs"),
		Same(Radii[Pelvis], SolidBodyMesh::SplitHalfWidth(10.f, Radii[ThighL])));
	TestTrue(TEXT("the shirt leaves on that hip circle"), Same(IntoWaist, Radii[Pelvis]));

	float ChestStart = 0.f;
	float NeckFromChest = 0.f;
	float NeckStart = 0.f;
	float Chin = 0.f;
	SolidBodyMesh::SegmentRadii(Radii, Chest, Neck, ChestStart, NeckFromChest);
	SolidBodyMesh::SegmentRadii(Radii, Neck, Head, NeckStart, Chin);
	TestTrue(TEXT("chest meets the neck"), Same(NeckFromChest, NeckStart));
	TestTrue(TEXT("neck meets the chin"), Same(Chin, Profile.Jaw));
	TestTrue(TEXT("the neck stays a neck"), Same(Radii[Neck], Profile.Neck));
	TestTrue(
		TEXT("chest matches both arms"),
		Same(Radii[Chest], SolidBodyMesh::SplitHalfWidth(16.f, Radii[UpperL])));
	TestTrue(TEXT("both arms share a shoulder"), Same(Radii[UpperL], Radii[UpperR]));

	float ArmStart = 0.f;
	float ElbowEnd = 0.f;
	float ElbowStart = 0.f;
	float WristEnd = 0.f;
	SolidBodyMesh::SegmentRadii(Radii, UpperL, ElbowL, ArmStart, ElbowEnd);
	SolidBodyMesh::SegmentRadii(Radii, ElbowL, WristL, ElbowStart, WristEnd);
	TestTrue(TEXT("upper arm meets the forearm"), Same(ElbowEnd, ElbowStart));
	TestTrue(TEXT("the elbow is one radius"), Same(ElbowEnd, Profile.Elbow));
	TestTrue(TEXT("the shoulder cap ends on the upper arm"), Same(ArmStart, Profile.Shoulder));
	TestTrue(TEXT("forearm meets the hand"), Same(WristEnd, Profile.Wrist));

	TArray<FSolidDeformBone> LoneBones;
	TArray<FVector> LonePos;
	auto AddLone = [&LoneBones, &LonePos](const int32 Parent, const ESolidBodyBoneRole Role, const FVector& At)
	{
		FSolidDeformBone Bone;
		Bone.ParentIndex = Parent;
		Bone.Role = Role;
		LoneBones.Add(Bone);
		LonePos.Add(At);
		return LoneBones.Num() - 1;
	};
	const int32 LonePelvis = AddLone(INDEX_NONE, ESolidBodyBoneRole::Pelvis, FVector::ZeroVector);
	const int32 LoneSpine = AddLone(LonePelvis, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 20.f));
	const int32 LoneNeck = AddLone(LoneSpine, ESolidBodyBoneRole::Neck, FVector(0.f, 0.f, 40.f));
	const int32 LoneHead = AddLone(LoneNeck, ESolidBodyBoneRole::Head, FVector(0.f, 0.f, 62.f));
	TArray<float> LoneRadii;
	SolidBodyMesh::JointRadii(LoneBones, LonePos, Average, LoneRadii);
	TestTrue(TEXT("a lone spine is the chest"), Same(LoneRadii[LoneSpine], Profile.Chest));
	TestTrue(TEXT("a neck with no arms keeps the neck radius"), Same(LoneRadii[LoneNeck], Profile.Neck));
	TestTrue(TEXT("a lone head is still the chin"), Same(LoneRadii[LoneHead], Profile.Jaw));
	TestTrue(TEXT("hips with no thighs keep the waist"), Same(LoneRadii[LonePelvis], Profile.Waist));

	TArray<float> Empty;
	SolidBodyMesh::JointRadii(TArray<FSolidDeformBone>(), TArray<FVector>(), Average, Empty);
	TestEqual(TEXT("an empty chain has no joints"), Empty.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodySegmentKeyTest,
	"SolidCore1.Body.SegmentKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodySegmentKeyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("head"), SolidBodyMesh::SegmentKey(TEXT("head")), FString(TEXT("head")));
	TestEqual(TEXT("left eye"), SolidBodyMesh::SegmentKey(TEXT("eye_jnt_l")), FString(TEXT("head")));
	TestEqual(TEXT("right eye"), SolidBodyMesh::SegmentKey(TEXT("eye_r")), FString(TEXT("head")));
	TestEqual(TEXT("eyelid"), SolidBodyMesh::SegmentKey(TEXT("eyelid_jnt_l")), FString(TEXT("head")));
	TestEqual(TEXT("neck"), SolidBodyMesh::SegmentKey(TEXT("neck_01")), FString(TEXT("neck")));
	TestEqual(TEXT("left shoulder clavicle"), SolidBodyMesh::SegmentKey(TEXT("clavicle_l")), FString(TEXT("shoulder_l")));
	TestEqual(TEXT("left upper arm"), SolidBodyMesh::SegmentKey(TEXT("upperarm_l")), FString(TEXT("shoulder_l")));
	TestEqual(TEXT("left shoulder joint"), SolidBodyMesh::SegmentKey(TEXT("shoulder_jnt_l")), FString(TEXT("shoulder_l")));
	TestEqual(TEXT("left shoulder joint tip"), SolidBodyMesh::SegmentKey(TEXT("shoulder_jnt_1_r")), FString(TEXT("shoulder_r")));
	TestEqual(TEXT("upper arm twist stays out"), SolidBodyMesh::SegmentKey(TEXT("upperarm_twist_01_l")), FString());
	TestEqual(TEXT("chest slab"), SolidBodyMesh::SegmentKey(TEXT("spine_03")), FString(TEXT("spine_03")));
	TestEqual(TEXT("mid slab"), SolidBodyMesh::SegmentKey(TEXT("spine_02")), FString(TEXT("spine_02")));
	TestEqual(TEXT("hip slab"), SolidBodyMesh::SegmentKey(TEXT("spine_01")), FString(TEXT("spine_01")));
	TestEqual(TEXT("left thigh"), SolidBodyMesh::SegmentKey(TEXT("thigh_l")), FString(TEXT("thigh_l")));
	TestEqual(TEXT("left thigh twist"), SolidBodyMesh::SegmentKey(TEXT("thigh_twist_01_l")), FString(TEXT("thigh_l")));
	TestEqual(TEXT("right calf"), SolidBodyMesh::SegmentKey(TEXT("calf_r")), FString(TEXT("calf_r")));
	TestEqual(TEXT("right calf twist"), SolidBodyMesh::SegmentKey(TEXT("calf_twist_01_r")), FString(TEXT("calf_r")));
	TestEqual(TEXT("left foot"), SolidBodyMesh::SegmentKey(TEXT("foot_l")), FString(TEXT("foot_l")));
	TestEqual(TEXT("left ball"), SolidBodyMesh::SegmentKey(TEXT("ball_l")), FString(TEXT("foot_l")));
	TestEqual(TEXT("left extra toe"), SolidBodyMesh::SegmentKey(TEXT("extra_finger_l")), FString(TEXT("foot_l")));
	TestEqual(TEXT("left forearm"), SolidBodyMesh::SegmentKey(TEXT("lowerarm_l")), FString(TEXT("forearm_l")));
	TestEqual(TEXT("left hand"), SolidBodyMesh::SegmentKey(TEXT("hand_l")), FString(TEXT("hand_l")));
	TestEqual(TEXT("left index"), SolidBodyMesh::SegmentKey(TEXT("index_01_l")), FString(TEXT("hand_l")));
	TestEqual(TEXT("pelvis is not a piece"), SolidBodyMesh::SegmentKey(TEXT("pelvis")), FString());
	TestEqual(TEXT("axe stays off the body"), SolidBodyMesh::SegmentKey(TEXT("axe_jnt")), FString());
	TestEqual(TEXT("cloack stays off the body"), SolidBodyMesh::SegmentKey(TEXT("cloack_jnt_1_l")), FString());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyFitConeTest,
	"SolidCore1.Body.FitCone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyFitConeTest::RunTest(const FString& Parameters)
{
	auto Length = [](const SolidBodyMesh::FSolidFittedCone& Cone)
	{
		return static_cast<float>(FVector::Dist(Cone.Start, Cone.End));
	};
	auto Mid = [](const SolidBodyMesh::FSolidFittedCone& Cone)
	{
		return (Cone.Start + Cone.End) * 0.5;
	};
	auto Contains = [](const SolidBodyMesh::FSolidFittedCone& Cone, const FVector& Point)
	{
		const FVector Axis = Cone.End - Cone.Start;
		const double Span = Axis.Size();
		if (Span < 0.01 || !Cone.bValid)
		{
			return false;
		}
		const FVector Unit = Axis / Span;
		const FVector Rel = Point - Cone.Start;
		const double Along = FVector::DotProduct(Rel, Unit);
		const double Radial = (Rel - Unit * Along).Size();
		const double U = FMath::Clamp(Along / Span, 0.0, 1.0);
		const double Have = FMath::Lerp(
			static_cast<double>(Cone.RadiusStart),
			static_cast<double>(Cone.RadiusEnd),
			U);
		return Radial <= Have + 0.02 && Radial <= static_cast<double>(FMath::Max(Cone.RadiusStart, Cone.RadiusEnd)) + 0.02;
	};

	TestFalse(TEXT("no points"), SolidBodyMesh::FitCone(nullptr, 0).bValid);

	const FVector Shaft[] = { FVector(0.f, 0.f, 0.f), FVector(0.f, 0.f, 40.f) };
	const SolidBodyMesh::FSolidFittedCone Long = SolidBodyMesh::FitCone(Shaft, 2);
	TestTrue(TEXT("a shaft is a cone"), Long.bValid);
	TestTrue(TEXT("a shaft is only as long as its bones"), FMath::IsNearlyEqual(Length(Long), 40.f, 0.05f));
	TestTrue(TEXT("the shaft start is inside"), Contains(Long, Shaft[0]));
	TestTrue(TEXT("the shaft end is inside"), Contains(Long, Shaft[1]));

	const FVector Bulge[] = {
		FVector(0.f, 0.f, 0.f),
		FVector(0.f, 0.f, 40.f),
		FVector(5.f, 0.f, 20.f),
	};
	const SolidBodyMesh::FSolidFittedCone Wide = SolidBodyMesh::FitCone(Bulge, 3);
	TestTrue(TEXT("a sideways bone stays inside its cone"), Contains(Wide, Bulge[2]));
	TestTrue(TEXT("the bulge does not lengthen the shaft"), FMath::IsNearlyEqual(Length(Wide), 40.f, 0.05f));

	const FVector Left[] = {
		FVector(-12.f, 0.f, 0.f),
		FVector(-12.f, 1.f, 30.f),
		FVector(-16.f, -2.f, 14.f),
	};
	const FVector Right[] = {
		FVector(12.f, 0.f, 0.f),
		FVector(12.f, 1.f, 30.f),
		FVector(16.f, -2.f, 14.f),
	};
	const SolidBodyMesh::FSolidFittedCone LeftCone = SolidBodyMesh::FitCone(Left, 3);
	const SolidBodyMesh::FSolidFittedCone RightCone = SolidBodyMesh::FitCone(Right, 3);
	TestTrue(TEXT("mirrored legs are the same length"), FMath::IsNearlyEqual(Length(LeftCone), Length(RightCone), 0.05f));
	TestTrue(
		TEXT("mirrored legs share a radius"),
		FMath::IsNearlyEqual(
			FMath::Min(LeftCone.RadiusStart, LeftCone.RadiusEnd),
			FMath::Min(RightCone.RadiusStart, RightCone.RadiusEnd),
			0.05f)
		&& FMath::IsNearlyEqual(
			FMath::Max(LeftCone.RadiusStart, LeftCone.RadiusEnd),
			FMath::Max(RightCone.RadiusStart, RightCone.RadiusEnd),
			0.05f));
	const FVector LeftMid = Mid(LeftCone);
	const FVector RightMid = Mid(RightCone);
	TestTrue(TEXT("mirrored legs meet on X"), FMath::IsNearlyEqual(LeftMid.X, -RightMid.X, 0.05));
	TestTrue(TEXT("mirrored legs share Y"), FMath::IsNearlyEqual(LeftMid.Y, RightMid.Y, 0.05));
	TestTrue(TEXT("mirrored legs share Z"), FMath::IsNearlyEqual(LeftMid.Z, RightMid.Z, 0.05));

	const FVector Joint[] = { FVector(3.f, 4.f, 5.f) };
	const SolidBodyMesh::FSolidFittedCone Bead = SolidBodyMesh::FitCone(Joint, 1);
	TestTrue(TEXT("one bone is still a cone"), Bead.bValid);
	const float BeadLength = Length(Bead);
	TestTrue(TEXT("one bone is about a centimetre"), BeadLength >= 1.f && BeadLength <= 2.f);
	TestTrue(TEXT("the lone bone is inside"), Contains(Bead, Joint[0]));

	float RayT = 0.f;
	float SegU = 0.f;
	const float OnAxis = SolidSkeletonHover::RaySegmentDistance(
		FVector(0.f, -10.f, 0.f),
		FVector(0.f, 1.f, 0.f),
		FVector::ZeroVector,
		FVector(0.f, 0.f, 10.f),
		RayT,
		SegU);
	TestTrue(TEXT("a ray down the pointer hits the axis"), FMath::IsNearlyZero(OnAxis, 0.01f));

	const float Offset = SolidSkeletonHover::RaySegmentDistance(
		FVector(3.f, -10.f, 0.f),
		FVector(0.f, 1.f, 0.f),
		FVector::ZeroVector,
		FVector(0.f, 0.f, 10.f),
		RayT,
		SegU);
	TestTrue(TEXT("three centimetres off the axis misses a thin cone"), Offset > 1.f);
	TestTrue(TEXT("three centimetres off the axis hits a cone of that radius"), Offset <= 3.f + 0.01f);

	const float Away = SolidSkeletonHover::RaySegmentDistance(
		FVector(0.f, 5.f, 0.f),
		FVector(0.f, 1.f, 0.f),
		FVector::ZeroVector,
		FVector(0.f, 0.f, 10.f),
		RayT,
		SegU);
	TestTrue(TEXT("a ray heading away does not hit"), RayT <= 0.01f && Away > 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyLeftThighTest,
	"SolidCore1.Body.LeftThigh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyLeftThighTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the hip end is 20 cm across"), SolidBodyMesh::LeftThighTopDiameterCm, 20.f);
	TestEqual(TEXT("the knee end is 15 cm across"), SolidBodyMesh::LeftThighBottomDiameterCm, 15.f);
	TestEqual(TEXT("the thigh bone"), FString(SolidBodyMesh::LeftThighBone), FString(TEXT("calf_l")));

	const FVector ParentJoint(1.f, 2.f, 90.f);
	const FVector BoneEnd(1.f, 2.f, 68.f);
	const SolidBodyMesh::FSolidFittedCone Cone = SolidBodyMesh::LeftThighCone(ParentJoint, BoneEnd);
	TestTrue(TEXT("the thigh is a cone"), Cone.bValid);
	TestTrue(TEXT("the wide end is the hip"), Cone.Start.Equals(ParentJoint, 0.01));
	TestTrue(TEXT("the cone ends on calf_l"), Cone.End.Equals(BoneEnd, 0.01));
	TestTrue(
		TEXT("the cone is the length of the bone"),
		FMath::IsNearlyEqual(static_cast<float>(FVector::Dist(Cone.Start, Cone.End)), 22.f, 0.05f));
	TestTrue(TEXT("the hip is 10 cm in radius"), FMath::IsNearlyEqual(Cone.RadiusStart, 10.f, 0.01f));
	TestTrue(TEXT("the far end is 7.5 cm in radius"), FMath::IsNearlyEqual(Cone.RadiusEnd, 7.5f, 0.01f));

	const SolidBodyMesh::FSolidFittedCone Short = SolidBodyMesh::LeftThighCone(ParentJoint, ParentJoint);
	TestFalse(TEXT("a bone with no length is not drawn"), Short.bValid);

	TestEqual(TEXT("the right thigh bone"), FString(SolidBodyMesh::RightThighBone), FString(TEXT("calf_r")));
	const FVector LeftParent(-8.f, 1.f, 90.f);
	const FVector LeftEnd(-10.f, 2.f, 68.f);
	const FVector RightParent(8.f, 1.f, 90.f);
	const FVector RightEnd(10.f, 2.f, 68.f);
	const SolidBodyMesh::FSolidFittedCone LeftCone = SolidBodyMesh::LeftThighCone(LeftParent, LeftEnd);
	const SolidBodyMesh::FSolidFittedCone RightCone = SolidBodyMesh::LeftThighCone(RightParent, RightEnd);
	TestTrue(
		TEXT("mirrored thighs are the same length"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(LeftCone.Start, LeftCone.End)),
			static_cast<float>(FVector::Dist(RightCone.Start, RightCone.End)),
			0.05f));
	TestTrue(TEXT("mirrored thighs share the wide radius"), FMath::IsNearlyEqual(LeftCone.RadiusStart, RightCone.RadiusStart, 0.01f));
	TestTrue(TEXT("mirrored thighs share the narrow radius"), FMath::IsNearlyEqual(LeftCone.RadiusEnd, RightCone.RadiusEnd, 0.01f));
	TestTrue(TEXT("the right thigh starts across from the left"), RightCone.Start.Equals(FVector(-LeftCone.Start.X, LeftCone.Start.Y, LeftCone.Start.Z), 0.01));
	TestTrue(TEXT("the right thigh ends across from the left"), RightCone.End.Equals(FVector(-LeftCone.End.X, LeftCone.End.Y, LeftCone.End.Z), 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyCalfConeTest,
	"SolidCore1.Body.CalfCone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyCalfConeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the calf starts at the thigh's narrow end"), SolidBodyMesh::CalfTopDiameterCm, SolidBodyMesh::LeftThighBottomDiameterCm);
	TestEqual(TEXT("the calf top is 15 cm across"), SolidBodyMesh::CalfTopDiameterCm, 15.f);
	TestEqual(TEXT("the calf bottom is 12 cm across"), SolidBodyMesh::CalfBottomDiameterCm, 12.f);
	TestEqual(TEXT("left calf bone"), FString(SolidBodyMesh::LeftCalfBone), FString(TEXT("foot_l")));
	TestEqual(TEXT("right calf bone"), FString(SolidBodyMesh::RightCalfBone), FString(TEXT("foot_r")));

	const FVector Knee(0.f, 0.f, 60.f);
	const FVector Ankle(0.f, 0.f, 20.f);
	const SolidBodyMesh::FSolidFittedCone Straight = SolidBodyMesh::CalfCone(Knee, Ankle);
	TestTrue(TEXT("a straight calf is a cone"), Straight.bValid);
	TestTrue(TEXT("the wide end is the knee"), Straight.Start.Equals(Knee, 0.01));
	TestTrue(TEXT("the cone ends on the foot bone"), Straight.End.Equals(Ankle, 0.01));
	TestTrue(
		TEXT("the cone is the length of the foot bone"),
		FMath::IsNearlyEqual(static_cast<float>(FVector::Dist(Straight.Start, Straight.End)), 40.f, 0.05f));
	TestTrue(TEXT("the top radius is 7.5 cm"), FMath::IsNearlyEqual(Straight.RadiusStart, 7.5f, 0.01f));
	TestTrue(TEXT("the bottom radius is 6 cm"), FMath::IsNearlyEqual(Straight.RadiusEnd, 6.f, 0.01f));

	const FVector LeftKnee(-6.f, 1.f, 50.f);
	const FVector LeftAnkle(-7.f, 2.f, 10.f);
	const SolidBodyMesh::FSolidFittedCone Left = SolidBodyMesh::CalfCone(LeftKnee, LeftAnkle);
	const SolidBodyMesh::FSolidFittedCone Right = SolidBodyMesh::CalfCone(
		FVector(-LeftKnee.X, LeftKnee.Y, LeftKnee.Z),
		FVector(-LeftAnkle.X, LeftAnkle.Y, LeftAnkle.Z));
	TestTrue(
		TEXT("mirrored calves are the same length"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(Left.Start, Left.End)),
			static_cast<float>(FVector::Dist(Right.Start, Right.End)),
			0.05f));
	TestTrue(TEXT("the right calf starts across from the left"), Right.Start.Equals(FVector(-Left.Start.X, Left.Start.Y, Left.Start.Z), 0.01));
	TestTrue(TEXT("the right calf ends across from the left"), Right.End.Equals(FVector(-Left.End.X, Left.End.Y, Left.End.Z), 0.01));

	TestFalse(TEXT("a bone with no length is not drawn"), SolidBodyMesh::CalfCone(Knee, Knee).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyBallCylinderTest,
	"SolidCore1.Body.BallCylinder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyBallCylinderTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the foot is 10 cm across"), SolidBodyMesh::BallDiameterCm, 10.f);
	TestEqual(TEXT("left foot bone"), FString(SolidBodyMesh::LeftBallBone), FString(TEXT("ball_l")));
	TestEqual(TEXT("right foot bone"), FString(SolidBodyMesh::RightBallBone), FString(TEXT("ball_r")));

	const FVector Ankle(0.f, 0.f, 12.f);
	const FVector Toe(0.f, 14.f, 10.f);
	const SolidBodyMesh::FSolidFittedCone Cylinder = SolidBodyMesh::BallCylinder(Ankle, Toe);
	TestTrue(TEXT("the foot is a cylinder"), Cylinder.bValid);
	TestTrue(TEXT("the cylinder starts at the ankle"), Cylinder.Start.Equals(Ankle, 0.01));
	TestTrue(TEXT("the cylinder ends on the ball bone"), Cylinder.End.Equals(Toe, 0.01));
	TestTrue(
		TEXT("the cylinder is the length of the bone"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(Cylinder.Start, Cylinder.End)),
			static_cast<float>(FVector::Dist(Ankle, Toe)),
			0.05f));
	TestTrue(TEXT("both ends are 5 cm in radius"), FMath::IsNearlyEqual(Cylinder.RadiusStart, 5.f, 0.01f));
	TestTrue(TEXT("the far end matches the near end"), FMath::IsNearlyEqual(Cylinder.RadiusStart, Cylinder.RadiusEnd, 0.01f));

	const FVector LeftAnkle(-4.f, 1.f, 8.f);
	const FVector LeftToe(-5.f, 12.f, 6.f);
	const SolidBodyMesh::FSolidFittedCone Left = SolidBodyMesh::BallCylinder(LeftAnkle, LeftToe);
	const SolidBodyMesh::FSolidFittedCone Right = SolidBodyMesh::BallCylinder(
		FVector(-LeftAnkle.X, LeftAnkle.Y, LeftAnkle.Z),
		FVector(-LeftToe.X, LeftToe.Y, LeftToe.Z));
	TestTrue(
		TEXT("mirrored feet are the same length"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(Left.Start, Left.End)),
			static_cast<float>(FVector::Dist(Right.Start, Right.End)),
			0.05f));
	TestTrue(TEXT("the right foot starts across from the left"), Right.Start.Equals(FVector(-Left.Start.X, Left.Start.Y, Left.Start.Z), 0.01));
	TestTrue(TEXT("the right foot ends across from the left"), Right.End.Equals(FVector(-Left.End.X, Left.End.Y, Left.End.Z), 0.01));

	TestFalse(TEXT("a bone with no length is not drawn"), SolidBodyMesh::BallCylinder(Ankle, Ankle).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyExtraFingerConeTest,
	"SolidCore1.Body.ExtraFingerCone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyExtraFingerConeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the ankle end matches the foot cylinder"), SolidBodyMesh::ExtraFingerAnkleDiameterCm, SolidBodyMesh::BallDiameterCm);
	TestEqual(TEXT("the ankle end is 10 cm across"), SolidBodyMesh::ExtraFingerAnkleDiameterCm, 10.f);
	TestEqual(TEXT("the toe end is 5 cm across"), SolidBodyMesh::ExtraFingerToeDiameterCm, 5.f);
	TestEqual(TEXT("left toe bone"), FString(SolidBodyMesh::LeftExtraFingerBone), FString(TEXT("extra_finger_l")));
	TestEqual(TEXT("right toe bone"), FString(SolidBodyMesh::RightExtraFingerBone), FString(TEXT("extra_finger_r")));

	const FVector AnkleEnd(0.f, 14.f, 10.f);
	const FVector ToeEnd(0.f, 22.f, 9.f);
	const SolidBodyMesh::FSolidFittedCone Cone = SolidBodyMesh::ExtraFingerCone(AnkleEnd, ToeEnd);
	TestTrue(TEXT("the toe is a cone"), Cone.bValid);
	TestTrue(TEXT("the wide end is towards the ankle"), Cone.Start.Equals(AnkleEnd, 0.01));
	TestTrue(TEXT("the cone ends on the extra finger"), Cone.End.Equals(ToeEnd, 0.01));
	TestTrue(
		TEXT("the cone is the length of the bone"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(Cone.Start, Cone.End)),
			static_cast<float>(FVector::Dist(AnkleEnd, ToeEnd)),
			0.05f));
	TestTrue(TEXT("the ankle end is 5 cm in radius"), FMath::IsNearlyEqual(Cone.RadiusStart, 5.f, 0.01f));
	TestTrue(TEXT("the toe end is 2.5 cm in radius"), FMath::IsNearlyEqual(Cone.RadiusEnd, 2.5f, 0.01f));

	const FVector LeftAnkleEnd(-5.f, 12.f, 6.f);
	const FVector LeftToeEnd(-6.f, 20.f, 5.f);
	const SolidBodyMesh::FSolidFittedCone Left = SolidBodyMesh::ExtraFingerCone(LeftAnkleEnd, LeftToeEnd);
	const SolidBodyMesh::FSolidFittedCone Right = SolidBodyMesh::ExtraFingerCone(
		FVector(-LeftAnkleEnd.X, LeftAnkleEnd.Y, LeftAnkleEnd.Z),
		FVector(-LeftToeEnd.X, LeftToeEnd.Y, LeftToeEnd.Z));
	TestTrue(
		TEXT("mirrored toes are the same length"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(Left.Start, Left.End)),
			static_cast<float>(FVector::Dist(Right.Start, Right.End)),
			0.05f));
	TestTrue(TEXT("the right toe starts across from the left"), Right.Start.Equals(FVector(-Left.Start.X, Left.Start.Y, Left.Start.Z), 0.01));
	TestTrue(TEXT("the right toe ends across from the left"), Right.End.Equals(FVector(-Left.End.X, Left.End.Y, Left.End.Z), 0.01));

	TestFalse(TEXT("a bone with no length is not drawn"), SolidBodyMesh::ExtraFingerCone(AnkleEnd, AnkleEnd).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHipConeTest,
	"SolidCore1.Body.HipCone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHipConeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the hip bottom is 50 cm side to side"), SolidBodyMesh::HipSideDiameterCm, 50.f);
	TestEqual(TEXT("the hip top is 30 cm side to side"), SolidBodyMesh::HipTopSideDiameterCm, 30.f);
	TestEqual(TEXT("the hip is 20 cm front to back"), SolidBodyMesh::HipDepthDiameterCm, 20.f);
	TestEqual(TEXT("left hip bone"), FString(SolidBodyMesh::HipLeftBone), FString(TEXT("thigh_l")));
	TestEqual(TEXT("right hip bone"), FString(SolidBodyMesh::HipRightBone), FString(TEXT("thigh_r")));
	TestEqual(TEXT("spine bone"), FString(SolidBodyMesh::HipSpineBone), FString(TEXT("spine_01")));
	TestEqual(TEXT("the hip uses the shirt"), SolidBodyMesh::ShirtMaterialSlot, static_cast<uint8>(0));
	TestTrue(TEXT("the shirt is not the trousers"), SolidBodyMesh::ShirtMaterialSlot != SolidBodyMesh::TrouserMaterialSlot);

	const FVector SpineStart(1.f, 2.f, 95.f);
	const FVector SpineEnd(1.f, 4.f, 112.f);
	const SolidBodyMesh::FSolidFittedCone Cone = SolidBodyMesh::HipCone(SpineStart, SpineEnd);
	TestTrue(TEXT("the hip is a piece"), Cone.bValid);
	TestTrue(TEXT("the piece starts at the spine joint"), Cone.Start.Equals(SpineStart, 0.01));
	TestTrue(TEXT("the piece ends on spine_01"), Cone.End.Equals(SpineEnd, 0.01));
	TestTrue(
		TEXT("the piece is the length of spine_01"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(Cone.Start, Cone.End)),
			static_cast<float>(FVector::Dist(SpineStart, SpineEnd)),
			0.05f));
	TestTrue(TEXT("the bottom is 25 cm toward the sides"), FMath::IsNearlyEqual(Cone.RadiusStart, 25.f, 0.01f));
	TestTrue(TEXT("the top is 15 cm toward the sides"), FMath::IsNearlyEqual(Cone.RadiusEnd, 15.f, 0.01f));
	TestTrue(TEXT("the bottom is 20 cm front to back"), FMath::IsNearlyEqual(Cone.DepthScaleStart, 0.4f, 0.01f));
	TestTrue(TEXT("the top stays 20 cm front to back"), FMath::IsNearlyEqual(Cone.DepthScaleEnd, 20.f / 30.f, 0.01f));
	TestTrue(TEXT("the full width is the mesh forward axis"), Cone.WideAxis.Equals(FVector::ForwardVector, 0.01));

	float BestT = TNumericLimits<float>::Max();
	TestTrue(
		TEXT("a ray from the side meets the hip"),
		SolidBodyMesh::RayHitsCone(Cone, FVector(40.f, 3.f, 103.f), FVector(-1.f, 0.f, 0.f), BestT));
	BestT = TNumericLimits<float>::Max();
	TestFalse(
		TEXT("a ray past the side misses the hip"),
		SolidBodyMesh::RayHitsCone(Cone, FVector(40.f, 3.f, 103.f), FVector(1.f, 0.f, 0.f), BestT));
	BestT = TNumericLimits<float>::Max();
	TestTrue(
		TEXT("a ray from the front meets the squashed side"),
		SolidBodyMesh::RayHitsCone(Cone, FVector(1.f, 20.f, 103.f), FVector(0.f, -1.f, 0.f), BestT));
	BestT = TNumericLimits<float>::Max();
	TestFalse(
		TEXT("a ray that only clips the round outline misses the front"),
		SolidBodyMesh::RayHitsCone(Cone, FVector(1.f, 18.f, 103.f), FVector(1.f, 0.f, 0.f), BestT));

	TestFalse(TEXT("a bone with no length is not drawn"), SolidBodyMesh::HipCone(SpineStart, SpineStart).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyTorsoAndArmsTest,
	"SolidCore1.Body.TorsoAndArms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyTorsoAndArmsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("spine_02 rises"), SolidBodyMesh::IsRisingSpineBoneName(TEXT("spine_02")));
	TestTrue(TEXT("spine_03 rises"), SolidBodyMesh::IsRisingSpineBoneName(TEXT("spine_03")));
	TestTrue(TEXT("spine_04 rises"), SolidBodyMesh::IsRisingSpineBoneName(TEXT("spine_04")));
	TestFalse(TEXT("spine_01 is the hip"), SolidBodyMesh::IsRisingSpineBoneName(TEXT("spine_01")));
	TestFalse(TEXT("a spine twist stays bare"), SolidBodyMesh::IsRisingSpineBoneName(TEXT("spine_02_twist")));
	TestFalse(TEXT("the neck stays bare"), SolidBodyMesh::IsRisingSpineBoneName(TEXT("neck_01")));

	TestTrue(TEXT("left clavicle"), SolidBodyMesh::IsClavicleBoneName(TEXT("clavicle_l")));
	TestTrue(TEXT("right clavicle"), SolidBodyMesh::IsClavicleBoneName(TEXT("clavicle_r")));
	TestFalse(TEXT("a clavicle twist stays bare"), SolidBodyMesh::IsClavicleBoneName(TEXT("clavicle_twist_01_l")));
	TestFalse(TEXT("shoulder_jnt is not a clavicle"), SolidBodyMesh::IsClavicleBoneName(TEXT("shoulder_jnt_l")));
	TestFalse(TEXT("shoulder_jnt tip is not a clavicle"), SolidBodyMesh::IsClavicleBoneName(TEXT("shoulder_jnt_1_r")));

	TestTrue(TEXT("left upper arm"), SolidBodyMesh::IsUpperArmBoneName(TEXT("upperarm_l")));
	TestTrue(TEXT("right upper arm"), SolidBodyMesh::IsUpperArmBoneName(TEXT("upperarm_r")));
	TestFalse(TEXT("an upper-arm twist stays bare"), SolidBodyMesh::IsUpperArmBoneName(TEXT("upperarm_twist_01_l")));
	TestFalse(TEXT("shoulder_jnt is not an upper arm"), SolidBodyMesh::IsUpperArmBoneName(TEXT("shoulder_jnt_l")));

	TestTrue(TEXT("left forearm"), SolidBodyMesh::IsForearmBoneName(TEXT("lowerarm_l")));
	TestTrue(TEXT("right forearm"), SolidBodyMesh::IsForearmBoneName(TEXT("lowerarm_r")));
	TestFalse(TEXT("a forearm twist stays bare"), SolidBodyMesh::IsForearmBoneName(TEXT("lowerarm_twist_01_r")));
	TestFalse(TEXT("a hand is not a forearm"), SolidBodyMesh::IsForearmBoneName(TEXT("hand_l")));
	TestTrue(TEXT("left hand"), SolidBodyMesh::IsHandBoneName(TEXT("hand_l")));
	TestTrue(TEXT("right hand"), SolidBodyMesh::IsHandBoneName(TEXT("hand_r")));
	TestFalse(TEXT("a finger is not the hand"), SolidBodyMesh::IsHandBoneName(TEXT("index_01_l")));
	TestFalse(TEXT("an IK hand stays bare"), SolidBodyMesh::IsHandBoneName(TEXT("ik_hand_l")));
	TestFalse(TEXT("a forearm is not a hand"), SolidBodyMesh::IsHandBoneName(TEXT("lowerarm_l")));
	TestFalse(TEXT("the head stays bare"), SolidBodyMesh::IsUpperArmBoneName(TEXT("head")));
	TestFalse(TEXT("an eye stays bare"), SolidBodyMesh::IsClavicleBoneName(TEXT("eye_l")));

	TestEqual(TEXT("the chest is 40 cm side to side"), SolidBodyMesh::ChestSideDiameterCm, 40.f);
	TestEqual(TEXT("the torso depth matches the hip"), SolidBodyMesh::TorsoDepthDiameterCm, SolidBodyMesh::HipDepthDiameterCm);
	TestEqual(TEXT("skin is the third material"), SolidBodyMesh::SkinMaterialSlot, static_cast<uint8>(2));
	TestEqual(TEXT("the arms use the shirt"), SolidBodyMesh::ShirtMaterialSlot, static_cast<uint8>(0));
	TestTrue(
		TEXT("the shoulder meets the upper arm"),
		FMath::IsNearlyEqual(SolidBodyMesh::UpperArmShoulderDiameterCm, SolidBodyMesh::JointShoulder * 2.f, 0.01f));
	TestTrue(
		TEXT("the elbow meets the forearm"),
		FMath::IsNearlyEqual(SolidBodyMesh::UpperArmElbowDiameterCm, SolidBodyMesh::JointElbow * 2.f, 0.01f));
	TestTrue(
		TEXT("the wrist is the joint section doubled"),
		FMath::IsNearlyEqual(SolidBodyMesh::ForearmWristDiameterCm, SolidBodyMesh::JointWrist * 2.f, 0.01f));

	float BottomCm = 0.f;
	float TopCm = 0.f;
	SolidBodyMesh::TorsoSideDiameters(0, 2, BottomCm, TopCm);
	TestTrue(TEXT("the first of two starts at the waist"), FMath::IsNearlyEqual(BottomCm, 30.f, 0.01f));
	TestTrue(TEXT("the first of two is halfway to the chest"), FMath::IsNearlyEqual(TopCm, 35.f, 0.01f));
	SolidBodyMesh::TorsoSideDiameters(1, 2, BottomCm, TopCm);
	TestTrue(TEXT("the second of two starts where the first ended"), FMath::IsNearlyEqual(BottomCm, 35.f, 0.01f));
	TestTrue(TEXT("the second of two is the chest"), FMath::IsNearlyEqual(TopCm, 40.f, 0.01f));
	SolidBodyMesh::TorsoSideDiameters(0, 1, BottomCm, TopCm);
	TestTrue(TEXT("one spine bone starts at the waist"), FMath::IsNearlyEqual(BottomCm, 30.f, 0.01f));
	TestTrue(TEXT("one spine bone ends at the chest"), FMath::IsNearlyEqual(TopCm, 40.f, 0.01f));
	SolidBodyMesh::TorsoSideDiameters(0, 3, BottomCm, TopCm);
	TestTrue(TEXT("the first of three starts at the waist"), FMath::IsNearlyEqual(BottomCm, 30.f, 0.01f));
	TestTrue(TEXT("the first of three takes a third of the widen"), FMath::IsNearlyEqual(TopCm, 30.f + 10.f / 3.f, 0.01f));
	SolidBodyMesh::TorsoSideDiameters(2, 3, BottomCm, TopCm);
	TestTrue(TEXT("the last of three starts a third short of the chest"), FMath::IsNearlyEqual(BottomCm, 30.f + 20.f / 3.f, 0.01f));
	TestTrue(TEXT("the last of three is the chest"), FMath::IsNearlyEqual(TopCm, 40.f, 0.01f));
	SolidBodyMesh::TorsoSideDiameters(5, 2, BottomCm, TopCm);
	TestTrue(TEXT("a step past the end stays on the last bone"), FMath::IsNearlyEqual(BottomCm, 35.f, 0.01f));
	TestTrue(TEXT("a step past the end still reaches the chest"), FMath::IsNearlyEqual(TopCm, 40.f, 0.01f));
	SolidBodyMesh::TorsoSideDiameters(0, 0, BottomCm, TopCm);
	TestTrue(TEXT("no rising spine stays at the waist"), FMath::IsNearlyEqual(BottomCm, SolidBodyMesh::HipTopSideDiameterCm, 0.01f));
	TestTrue(TEXT("no rising spine has no taper"), FMath::IsNearlyEqual(TopCm, SolidBodyMesh::HipTopSideDiameterCm, 0.01f));

	const FVector Bottom(0.f, 0.f, 100.f);
	const FVector Top(0.f, 0.f, 122.f);
	const SolidBodyMesh::FSolidFittedCone Torso = SolidBodyMesh::TorsoCone(Bottom, Top, 30.f, 40.f);
	TestTrue(TEXT("the torso piece is drawn"), Torso.bValid);
	TestTrue(TEXT("the torso piece starts at the parent joint"), Torso.Start.Equals(Bottom, 0.01));
	TestTrue(TEXT("the torso piece ends on the spine bone"), Torso.End.Equals(Top, 0.01));
	TestTrue(TEXT("the waist end is 15 cm toward the sides"), FMath::IsNearlyEqual(Torso.RadiusStart, 15.f, 0.01f));
	TestTrue(TEXT("the chest end is 20 cm toward the sides"), FMath::IsNearlyEqual(Torso.RadiusEnd, 20.f, 0.01f));
	TestTrue(TEXT("the waist end is 20 cm front to back"), FMath::IsNearlyEqual(Torso.DepthScaleStart, 20.f / 30.f, 0.01f));
	TestTrue(TEXT("the chest end is 20 cm front to back"), FMath::IsNearlyEqual(Torso.DepthScaleEnd, 20.f / 40.f, 0.01f));
	TestTrue(TEXT("the full width is the mesh forward axis"), Torso.WideAxis.Equals(FVector::ForwardVector, 0.01));
	float BestT = TNumericLimits<float>::Max();
	TestTrue(
		TEXT("a ray from the side meets the torso"),
		SolidBodyMesh::RayHitsCone(Torso, FVector(30.f, 0.f, 111.f), FVector(-1.f, 0.f, 0.f), BestT));
	BestT = TNumericLimits<float>::Max();
	TestFalse(
		TEXT("a ray past the side misses the torso"),
		SolidBodyMesh::RayHitsCone(Torso, FVector(30.f, 0.f, 111.f), FVector(1.f, 0.f, 0.f), BestT));
	BestT = TNumericLimits<float>::Max();
	TestFalse(
		TEXT("a ray outside the front misses the torso"),
		SolidBodyMesh::RayHitsCone(Torso, FVector(0.f, 16.f, 111.f), FVector(1.f, 0.f, 0.f), BestT));
	TestFalse(TEXT("a spine bone with no length is not drawn"), SolidBodyMesh::TorsoCone(Bottom, Bottom, 30.f, 40.f).bValid);

	const FVector Root(-2.f, 1.f, 140.f);
	const FVector Tip(-18.f, 2.f, 148.f);
	const SolidBodyMesh::FSolidFittedCone Clavicle = SolidBodyMesh::ClavicleCone(Root, Tip);
	TestTrue(TEXT("the clavicle is a cone"), Clavicle.bValid);
	TestTrue(TEXT("the clavicle starts at the parent joint"), Clavicle.Start.Equals(Root, 0.01));
	TestTrue(TEXT("the clavicle ends on the bone"), Clavicle.End.Equals(Tip, 0.01));
	TestTrue(TEXT("the clavicle root is 7 cm in radius"), FMath::IsNearlyEqual(Clavicle.RadiusStart, 7.f, 0.01f));
	TestTrue(TEXT("the clavicle tip matches the upper arm"), FMath::IsNearlyEqual(Clavicle.RadiusEnd, 5.2f, 0.01f));
	TestTrue(TEXT("the clavicle is round"), FMath::IsNearlyEqual(Clavicle.DepthScaleStart, 1.f, 0.01f));
	TestTrue(TEXT("the clavicle tip is round"), FMath::IsNearlyEqual(Clavicle.DepthScaleEnd, 1.f, 0.01f));

	const FVector Shoulder(-18.f, 2.f, 148.f);
	const FVector Elbow(-20.f, 4.f, 120.f);
	const SolidBodyMesh::FSolidFittedCone Upper = SolidBodyMesh::UpperArmCone(Shoulder, Elbow);
	TestTrue(TEXT("the upper arm is a cone"), Upper.bValid);
	TestTrue(TEXT("the upper arm is 5.2 cm in radius at the parent"), FMath::IsNearlyEqual(Upper.RadiusStart, 5.2f, 0.01f));
	TestTrue(TEXT("the upper arm is 4.2 cm in radius at the bone"), FMath::IsNearlyEqual(Upper.RadiusEnd, 4.2f, 0.01f));
	TestTrue(TEXT("the upper arm is the length of the bone"), FMath::IsNearlyEqual(
		static_cast<float>(FVector::Dist(Upper.Start, Upper.End)),
		static_cast<float>(FVector::Dist(Shoulder, Elbow)),
		0.05f));

	const FVector Wrist(-22.f, 6.f, 96.f);
	const SolidBodyMesh::FSolidFittedCone Forearm = SolidBodyMesh::ForearmCone(Elbow, Wrist);
	TestTrue(TEXT("the forearm is a cone"), Forearm.bValid);
	TestTrue(TEXT("the forearm starts where the upper arm ends"), FMath::IsNearlyEqual(Forearm.RadiusStart, Upper.RadiusEnd, 0.01f));
	TestTrue(TEXT("the forearm is 3.4 cm in radius at the bone"), FMath::IsNearlyEqual(Forearm.RadiusEnd, 3.4f, 0.01f));
	TestFalse(TEXT("an arm bone with no length is not drawn"), SolidBodyMesh::ForearmCone(Elbow, Elbow).bValid);

	const FVector Knuckle(-24.f, 8.f, 72.f);
	const SolidBodyMesh::FSolidFittedCone Hand = SolidBodyMesh::HandCone(Wrist, Knuckle);
	TestTrue(TEXT("the hand is a cone"), Hand.bValid);
	TestTrue(TEXT("the hand starts at the wrist"), Hand.Start.Equals(Wrist, 0.01));
	TestTrue(TEXT("the hand ends on the bone"), Hand.End.Equals(Knuckle, 0.01));
	TestTrue(
		TEXT("the hand uses the lower-arm start diameter"),
		FMath::IsNearlyEqual(Hand.RadiusStart, SolidBodyMesh::LowerArmStartDiameterCm * 0.5f, 0.01f));
	TestTrue(
		TEXT("the hand uses the lower-arm end diameter"),
		FMath::IsNearlyEqual(Hand.RadiusEnd, SolidBodyMesh::LowerArmEndDiameterCm * 0.5f, 0.01f));
	TestTrue(TEXT("the hand is the length of the bone"), FMath::IsNearlyEqual(
		static_cast<float>(FVector::Dist(Hand.Start, Hand.End)),
		static_cast<float>(FVector::Dist(Wrist, Knuckle)),
		0.05f));
	TestFalse(TEXT("a hand bone with no length is not drawn"), SolidBodyMesh::HandCone(Wrist, Wrist).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyShoulderPieceTest,
	"SolidCore1.Body.ShoulderPiece",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyShoulderPieceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("neck bone"), FString(SolidBodyMesh::ShoulderNeckBone), FString(TEXT("neck_01")));
	TestEqual(TEXT("left clavicle"), FString(SolidBodyMesh::ShoulderLeftClavicleBone), FString(TEXT("clavicle_l")));
	TestEqual(TEXT("right clavicle"), FString(SolidBodyMesh::ShoulderRightClavicleBone), FString(TEXT("clavicle_r")));
	TestEqual(TEXT("left upper arm"), FString(SolidBodyMesh::ShoulderLeftUpperArmBone), FString(TEXT("upperarm_l")));
	TestEqual(TEXT("right upper arm"), FString(SolidBodyMesh::ShoulderRightUpperArmBone), FString(TEXT("upperarm_r")));
	TestEqual(TEXT("the chest bone"), FString(SolidBodyMesh::SpineChestBone), FString(TEXT("spine_03")));
	TestEqual(TEXT("the shoulder uses the shirt"), SolidBodyMesh::ShirtMaterialSlot, static_cast<uint8>(0));

	const FVector NeckStart(0.f, 0.f, 140.f);
	const FVector NeckEnd(0.f, 0.f, 159.4f);
	const FVector LeftClavicleStart[] = { NeckStart, NeckStart };
	const FVector LeftClavicleEnd[] = { FVector(7.26f, 3.71f, 151.39f), FVector(-7.26f, 3.71f, 151.39f) };
	const FVector UpperStart[] = { LeftClavicleEnd[0], LeftClavicleEnd[1] };
	const FVector UpperEnd[] = { FVector(19.92f, -2.60f, 144.81f), FVector(-19.92f, -2.60f, 144.81f) };
	const FVector SegmentStart[] = { LeftClavicleStart[0], LeftClavicleStart[1], UpperStart[0], UpperStart[1] };
	const FVector SegmentEnd[] = { LeftClavicleEnd[0], LeftClavicleEnd[1], UpperEnd[0], UpperEnd[1] };
	const SolidBodyMesh::FSolidShoulderPiece Piece = SolidBodyMesh::ShoulderPiece(
		NeckStart,
		NeckEnd,
		SegmentStart,
		SegmentEnd,
		4,
		SolidBodyMesh::ChestSideDiameterCm,
		FVector::ForwardVector);
	TestTrue(TEXT("the shoulders are a piece"), Piece.bValid);
	TestTrue(TEXT("the piece starts at the neck joint"), Piece.Lower.Start.Equals(NeckStart, 0.01));
	TestTrue(TEXT("the piece ends on neck_01"), Piece.Upper.End.Equals(NeckEnd, 0.01));
	TestTrue(
		TEXT("the piece is the length of neck_01"),
		FMath::IsNearlyEqual(
			static_cast<float>(FVector::Dist(Piece.Lower.Start, Piece.Upper.End)),
			static_cast<float>(FVector::Dist(NeckStart, NeckEnd)),
			0.05f));
	TestTrue(TEXT("the base is 20 cm toward the sides"), FMath::IsNearlyEqual(Piece.Lower.RadiusStart, 20.f, 0.01f));
	TestTrue(TEXT("the base is 20 cm front to back"), FMath::IsNearlyEqual(Piece.Lower.DepthScaleStart, 0.5f, 0.01f));
	TestTrue(
		TEXT("the top uses the neck-circle radius"),
		FMath::IsNearlyEqual(Piece.Upper.RadiusEnd, SolidBodyMesh::ShoulderTopRadiusCm, 0.01f));
	TestTrue(TEXT("the top circle is round"), FMath::IsNearlyEqual(Piece.Upper.DepthScaleEnd, 1.f, 0.01f));
	TestTrue(TEXT("the full width is the mesh forward axis"), Piece.Lower.WideAxis.Equals(FVector::ForwardVector, 0.01));
	TestTrue(TEXT("the middle is wider than the chest"), Piece.Lower.RadiusEnd > Piece.Lower.RadiusStart);
	TestTrue(
		TEXT("the middle clears the upper arm by the elbow radius"),
		Piece.Lower.RadiusEnd * FMath::Cos(PI / static_cast<float>(SolidBodyMesh::ShoulderSideCount))
			> 19.92f + SolidBodyMesh::ShoulderUpperArmClearanceCm);
	TestTrue(TEXT("the left upper arm sits inside"), SolidBodyMesh::ShoulderContains(Piece, UpperEnd[0]));
	TestTrue(TEXT("the right upper arm sits inside"), SolidBodyMesh::ShoulderContains(Piece, UpperEnd[1]));
	TestTrue(TEXT("the left clavicle sits inside"), SolidBodyMesh::ShoulderContains(Piece, LeftClavicleEnd[0]));
	TestTrue(TEXT("the right clavicle sits inside"), SolidBodyMesh::ShoulderContains(Piece, LeftClavicleEnd[1]));

	const FVector WideNeckStart(0.f, 0.f, 0.f);
	const FVector WideNeckEnd(0.f, 0.f, 20.f);
	const FVector WideStart[] = { WideNeckStart, FVector(0.f, 0.f, 8.f) };
	const FVector WideEnd[] = { FVector(8.f, 0.f, 6.f), FVector(35.f, 0.f, 10.f) };
	const SolidBodyMesh::FSolidShoulderPiece Wide = SolidBodyMesh::ShoulderPiece(
		WideNeckStart,
		WideNeckEnd,
		WideStart,
		WideEnd,
		2,
		40.f,
		FVector::ForwardVector);
	TestTrue(TEXT("a wide upper arm still makes a piece"), Wide.bValid);
	TestTrue(TEXT("the middle reaches past that upper arm"), Wide.Lower.RadiusEnd > 35.f);
	TestTrue(TEXT("that upper arm sits inside"), SolidBodyMesh::ShoulderContains(Wide, WideEnd[1]));
	const float InsideTop = SolidBodyMesh::ShoulderTopRadiusCm - 1.f;
	const float OutsideTop = SolidBodyMesh::ShoulderTopRadiusCm + 1.f;
	TestTrue(TEXT("a centimetre inside the neck circle is inside"), SolidBodyMesh::ShoulderContains(Wide, FVector(InsideTop, 0.f, 20.f)));
	TestFalse(TEXT("a centimetre outside the neck circle is outside"), SolidBodyMesh::ShoulderContains(Wide, FVector(OutsideTop, 0.f, 20.f)));
	TestTrue(TEXT("the base still reaches 9 cm forward"), SolidBodyMesh::ShoulderContains(Wide, FVector(0.f, 9.f, 0.f)));
	TestFalse(TEXT("the base stops before 12 cm forward"), SolidBodyMesh::ShoulderContains(Wide, FVector(0.f, 12.f, 0.f)));

	float HitT = TNumericLimits<float>::Max();
	TestTrue(
		TEXT("a ray from the side meets the shoulder"),
		SolidBodyMesh::RayHitsCone(Wide.Lower, FVector(50.f, 0.f, 10.f), FVector(-1.f, 0.f, 0.f), HitT)
			|| SolidBodyMesh::RayHitsCone(Wide.Upper, FVector(50.f, 0.f, 10.f), FVector(-1.f, 0.f, 0.f), HitT));

	TestFalse(
		TEXT("a neck with no length is not drawn"),
		SolidBodyMesh::ShoulderPiece(
			NeckStart,
			NeckStart,
			SegmentStart,
			SegmentEnd,
			4,
			40.f,
			FVector::ForwardVector).bValid);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
