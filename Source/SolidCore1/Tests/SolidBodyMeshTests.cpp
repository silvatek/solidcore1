#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBodyMesh.h"
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
	TestTrue(
		TEXT("shoulder line matches both arms"),
		Same(Radii[Neck], SolidBodyMesh::SplitHalfWidth(16.f, Radii[UpperL])));
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

#endif // WITH_DEV_AUTOMATION_TESTS
