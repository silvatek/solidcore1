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

	const FSolidBodyParams Sam = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	const FSolidBodyParams Alex = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Alex")));
	TestTrue(TEXT("sam draw is in range"), Sam.ExactEquals(Sam.Clamped()));
	TestTrue(TEXT("alex is a different body"), !Alex.ExactEquals(Sam));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyBoneRoleTest,
	"SolidCore1.Body.BoneRole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyBoneRoleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("upper-arm twist is not a shoulder plate"), SolidBodyMesh::IsTwistBoneName(TEXT("upperarm_twist_01_l")));
	TestTrue(TEXT("calf twist is not a shin"), SolidBodyMesh::IsTwistBoneName(TEXT("calf_twist_02_r")));
	TestFalse(TEXT("calf is not a twist"), SolidBodyMesh::IsTwistBoneName(TEXT("calf_r")));
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
	const TArray<FMeshBoneInfo>& Infos = Ref.GetRawRefBoneInfo();
	TArray<FSolidDeformBone> Named;
	Named.Reserve(Infos.Num());
	for (int32 Index = 0; Index < Infos.Num(); ++Index)
	{
		FSolidDeformBone Bone;
		Bone.ParentIndex = Infos[Index].ParentIndex;
		Bone.Name = Infos[Index].Name;
		Named.Add(Bone);
	}
	const SolidBodyMesh::FSolidBodyMesh Catalogue = SolidBodyMesh::FSolidBodyMesh::FromBones(Named);
	int32 Recognized = 0;
	int32 ScaledBone = INDEX_NONE;
	for (int32 Index = 0; Index < Infos.Num(); ++Index)
	{
		bool bWrapped = false;
		for (const SolidBodyMesh::FSolidBodyPart& Part : Catalogue.Parts)
		{
			if (Part.Bones.Contains(Infos[Index].Name))
			{
				bWrapped = true;
				break;
			}
		}
		if (!bWrapped)
		{
			continue;
		}
		++Recognized;
		if (ScaledBone == INDEX_NONE && Infos[Index].Name == FName(SolidBodyMesh::LeftThighBone))
		{
			ScaledBone = Index;
		}
	}
	TestTrue(TEXT("viking bone names map onto a body"), Recognized > 0);
	TestTrue(TEXT("the thigh span is a bone whose length can change"), ScaledBone != INDEX_NONE);
	if (Recognized == 0 || ScaledBone == INDEX_NONE)
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

	const TArray<FTransform>& SourceBind = Ref.GetRawRefBonePose();

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
	Stretched.LegLength = SolidBody::ParamMax;

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
	FSolidBodyRaySegmentTest,
	"SolidCore1.Body.RaySegment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyRaySegmentTest::RunTest(const FString& Parameters)
{
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

	TestEqual(TEXT("the chest is 40 cm side to side"), SolidBodyMesh::ChestSideDiameterCm, 40.f);
	TestEqual(TEXT("the torso depth matches the hip"), SolidBodyMesh::TorsoDepthDiameterCm, SolidBodyMesh::HipDepthDiameterCm);
	TestEqual(TEXT("skin is the third material"), SolidBodyMesh::SkinMaterialSlot, static_cast<uint8>(2));
	TestEqual(TEXT("the arms use the shirt"), SolidBodyMesh::ShirtMaterialSlot, static_cast<uint8>(0));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyPartsTest,
	"SolidCore1.Body.Parts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyPartsTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TArray<FName> Rising;
	Rising.Add(TEXT("spine_02"));
	Rising.Add(TEXT("spine_03"));
	FSolidBodyMesh Body = FSolidBodyMesh::Make(Rising);

	const TCHAR* Expected[] = {
		TEXT("Thigh (L)"),
		TEXT("Thigh (R)"),
		TEXT("Calf (L)"),
		TEXT("Calf (R)"),
		TEXT("Foot (L)"),
		TEXT("Foot (R)"),
		TEXT("Toe (L)"),
		TEXT("Toe (R)"),
		TEXT("Hip"),
		TEXT("Abdomen"),
		TEXT("Chest"),
		TEXT("Shoulders"),
		TEXT("Upper arm (L)"),
		TEXT("Upper arm (R)"),
		TEXT("Lower arm (L)"),
		TEXT("Lower arm (R)"),
	};
	const int32 ExpectedCount = static_cast<int32>(UE_ARRAY_COUNT(Expected));
	TestEqual(TEXT("part count"), Body.Parts.Num(), ExpectedCount);
	if (Body.Parts.Num() == ExpectedCount)
	{
		for (int32 Index = 0; Index < Body.Parts.Num(); ++Index)
		{
			TestEqual(Expected[Index], Body.Parts[Index].Name, FString(Expected[Index]));
		}
	}

	const FSolidBodyPart* Thigh = Body.Find(TEXT("Thigh (L)"));
	TestTrue(TEXT("the thigh is on the list"), Thigh != nullptr);
	if (Thigh)
	{
		TestEqual(TEXT("one thigh bone"), Thigh->Bones.Num(), 1);
		TestTrue(TEXT("the thigh wraps calf_l"), Thigh->Bones.Num() == 1 && Thigh->Bones[0] == FName(LeftThighBone));
		TestTrue(TEXT("the thigh is trousers"), Thigh->Colour == ESolidPartColour::Trousers);
		TestEqual(TEXT("trousers slot"), Thigh->MaterialSlot(), TrouserMaterialSlot);
		TestTrue(TEXT("the thigh is 20 cm at the hip"), FMath::IsNearlyEqual(Thigh->Start.X, LeftThighTopDiameterCm, 0.01f));
		TestTrue(TEXT("the thigh is round at the hip"), FMath::IsNearlyEqual(Thigh->Start.Y, Thigh->Start.X, 0.01f));
		TestTrue(TEXT("the thigh is 15 cm at the knee"), FMath::IsNearlyEqual(Thigh->End.X, LeftThighBottomDiameterCm, 0.01f));
		TestTrue(TEXT("the thigh has no mid section"), !Thigh->bHasMid);
	}

	const FSolidBodyPart* Hip = Body.Find(TEXT("Hip"));
	TestTrue(TEXT("the hip is on the list"), Hip != nullptr);
	if (Hip)
	{
		TestEqual(TEXT("three hip bones"), Hip->Bones.Num(), 3);
		TestTrue(TEXT("the hip span is spine_01"), Hip->Bones.Num() >= 1 && Hip->Bones[0] == FName(HipSpineBone));
		TestTrue(TEXT("the left thigh bone is inside the hip"), Hip->Bones.Num() == 3 && Hip->Bones[1] == FName(HipLeftBone));
		TestTrue(TEXT("the right thigh bone is inside the hip"), Hip->Bones.Num() == 3 && Hip->Bones[2] == FName(HipRightBone));
		TestTrue(TEXT("the hip is a shirt"), Hip->Colour == ESolidPartColour::Shirt);
		TestTrue(TEXT("the hip bottom is 50 cm across"), FMath::IsNearlyEqual(Hip->Start.X, HipSideDiameterCm, 0.01f));
		TestTrue(TEXT("the hip bottom is 20 cm deep"), FMath::IsNearlyEqual(Hip->Start.Y, HipDepthDiameterCm, 0.01f));
		TestTrue(TEXT("the hip top is 30 cm across"), FMath::IsNearlyEqual(Hip->End.X, HipTopSideDiameterCm, 0.01f));
		TestTrue(TEXT("the hip top stays 20 cm deep"), FMath::IsNearlyEqual(Hip->End.Y, HipDepthDiameterCm, 0.01f));
	}

	float AbdomenBottom = 0.f;
	float AbdomenTop = 0.f;
	TorsoSideDiameters(0, 2, AbdomenBottom, AbdomenTop);
	const FSolidBodyPart* Abdomen = Body.Find(TEXT("Abdomen"));
	TestTrue(TEXT("the abdomen is on the list"), Abdomen != nullptr);
	if (Abdomen)
	{
		TestTrue(TEXT("the abdomen wraps spine_02"), Abdomen->Bones.Num() == 1 && Abdomen->Bones[0] == FName(TEXT("spine_02")));
		TestTrue(TEXT("the abdomen starts at the waist"), FMath::IsNearlyEqual(Abdomen->Start.X, AbdomenBottom, 0.01f));
		TestTrue(TEXT("the abdomen ends halfway to the chest"), FMath::IsNearlyEqual(Abdomen->End.X, AbdomenTop, 0.01f));
		TestTrue(TEXT("the abdomen is 20 cm deep"), FMath::IsNearlyEqual(Abdomen->Start.Y, TorsoDepthDiameterCm, 0.01f));
		TestTrue(TEXT("the abdomen stays 20 cm deep"), FMath::IsNearlyEqual(Abdomen->End.Y, TorsoDepthDiameterCm, 0.01f));
	}

	float ChestBottom = 0.f;
	float ChestTop = 0.f;
	TorsoSideDiameters(1, 2, ChestBottom, ChestTop);
	const FSolidBodyPart* Chest = Body.Find(TEXT("Chest"));
	TestTrue(TEXT("the chest is on the list"), Chest != nullptr);
	if (Chest)
	{
		TestTrue(TEXT("the chest wraps spine_03"), Chest->Bones.Num() == 1 && Chest->Bones[0] == FName(SpineChestBone));
		TestTrue(TEXT("the chest starts where the abdomen ended"), FMath::IsNearlyEqual(Chest->Start.X, ChestBottom, 0.01f));
		TestTrue(TEXT("the chest ends at the shoulder breadth"), FMath::IsNearlyEqual(Chest->End.X, ChestSideDiameterCm, 0.01f));
		TestTrue(TEXT("the chest end matches the step"), FMath::IsNearlyEqual(Chest->End.X, ChestTop, 0.01f));
	}

	FSolidBodyPart* Shoulders = nullptr;
	for (FSolidBodyPart& Part : Body.Parts)
	{
		if (Part.Name == TEXT("Shoulders"))
		{
			Shoulders = &Part;
		}
	}
	TestTrue(TEXT("the shoulders are on the list"), Shoulders != nullptr);
	if (Shoulders && Chest)
	{
		TestTrue(TEXT("the shoulders have a mid section"), Shoulders->bHasMid);
		TestEqual(TEXT("five shoulder bones"), Shoulders->Bones.Num(), 5);
		TestTrue(TEXT("the shoulder span is neck_01"), Shoulders->Bones.Num() >= 1 && Shoulders->Bones[0] == FName(ShoulderNeckBone));
		TestTrue(TEXT("the left clavicle is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderLeftClavicleBone)));
		TestTrue(TEXT("the right clavicle is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderRightClavicleBone)));
		TestTrue(TEXT("the left upper-arm bone is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderLeftUpperArmBone)));
		TestTrue(TEXT("the right upper-arm bone is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderRightUpperArmBone)));
		TestTrue(TEXT("the shoulder base matches the chest"), FMath::IsNearlyEqual(Shoulders->Start.X, Chest->End.X, 0.01f));
		TestTrue(TEXT("the shoulder base is 20 cm deep"), FMath::IsNearlyEqual(Shoulders->Start.Y, TorsoDepthDiameterCm, 0.01f));
		TestTrue(TEXT("the shoulder top is a 20 cm circle"), FMath::IsNearlyEqual(Shoulders->End.X, ShoulderTopRadiusCm * 2.f, 0.01f));
		TestTrue(TEXT("the shoulder top is round"), FMath::IsNearlyEqual(Shoulders->End.Y, Shoulders->End.X, 0.01f));
		TestTrue(TEXT("the mid waits until the piece is drawn"), FMath::IsNearlyEqual(Shoulders->Mid.X, 0.f, 0.01f));
		TestTrue(TEXT("the shoulders are a shirt"), Shoulders->Colour == ESolidPartColour::Shirt);
		TestEqual(TEXT("shirt slot"), Shoulders->MaterialSlot(), ShirtMaterialSlot);

		const FVector NeckStart(0.f, 0.f, 140.f);
		const FVector NeckEnd(0.f, 0.f, 159.4f);
		const FVector SegmentStart[] = { NeckStart, NeckStart, FVector(7.26f, 3.71f, 151.39f), FVector(-7.26f, 3.71f, 151.39f) };
		const FVector SegmentEnd[] = {
			FVector(7.26f, 3.71f, 151.39f),
			FVector(-7.26f, 3.71f, 151.39f),
			FVector(19.92f, -2.60f, 144.81f),
			FVector(-19.92f, -2.60f, 144.81f),
		};
		const FSolidPartDraw Draw = Shoulders->Fit(
			NeckStart,
			NeckEnd,
			SegmentStart,
			SegmentEnd,
			4,
			FVector::ForwardVector);
		TestTrue(TEXT("drawing the shoulders uses both cones"), Draw.bShoulder && Draw.Shoulder.bValid);
		TestTrue(TEXT("the fitted mid is wider than the chest"), Shoulders->Mid.X > Shoulders->Start.X);
		TestTrue(TEXT("the fitted mid has depth"), Shoulders->Mid.Y > 0.f);
	}

	const FSolidBodyPart* Upper = Body.Find(TEXT("Upper arm (L)"));
	TestTrue(TEXT("the upper arm is on the list"), Upper != nullptr);
	if (Upper)
	{
		TestTrue(TEXT("the upper arm wraps lowerarm_l"), Upper->Bones.Num() == 1 && Upper->Bones[0] == FName(TEXT("lowerarm_l")));
		TestTrue(TEXT("the upper arm is a shirt"), Upper->Colour == ESolidPartColour::Shirt);
		TestTrue(TEXT("the upper arm is 8.4 cm at the shoulder"), FMath::IsNearlyEqual(Upper->Start.X, UpperArmElbowDiameterCm, 0.01f));
		TestTrue(TEXT("the upper arm is 6.8 cm at the elbow"), FMath::IsNearlyEqual(Upper->End.X, ForearmWristDiameterCm, 0.01f));
		TestTrue(TEXT("the upper arm is round"), FMath::IsNearlyEqual(Upper->Start.Y, Upper->Start.X, 0.01f));
	}

	const FSolidBodyPart* Lower = Body.Find(TEXT("lower arm (l)"));
	TestTrue(TEXT("the lower arm is on the list"), Lower != nullptr);
	if (Lower)
	{
		TestTrue(TEXT("the lower arm wraps hand_l"), Lower->Bones.Num() == 1 && Lower->Bones[0] == FName(TEXT("hand_l")));
		TestTrue(TEXT("the lower arm is a shirt"), Lower->Colour == ESolidPartColour::Shirt);
		TestTrue(TEXT("the lower arm starts at the elbow diameter"), FMath::IsNearlyEqual(Lower->Start.X, LowerArmStartDiameterCm, 0.01f));
		TestTrue(TEXT("the lower arm ends at the wrist diameter"), FMath::IsNearlyEqual(Lower->End.X, LowerArmEndDiameterCm, 0.01f));
		TestTrue(TEXT("the lower arm is round"), FMath::IsNearlyEqual(Lower->Start.Y, Lower->Start.X, 0.01f) && FMath::IsNearlyEqual(Lower->End.Y, Lower->End.X, 0.01f));
		TestTrue(TEXT("the lower arm is wider at the wrist"), Lower->End.X > Lower->Start.X);
		TestTrue(
			TEXT("the elbow steps from the upper arm down to the lower arm"),
			Upper && Upper->End.X > Lower->Start.X);
		TestTrue(TEXT("the lower arm has no mid section"), !Lower->bHasMid);
	}
	const FSolidBodyPart* LowerRight = Body.Find(TEXT("Lower arm (R)"));
	TestTrue(TEXT("the right lower arm matches"), Lower && LowerRight && LowerRight->Bones.Num() == 1
		&& LowerRight->Bones[0] == FName(TEXT("hand_r"))
		&& FMath::IsNearlyEqual(LowerRight->Start.X, Lower->Start.X, 0.01f)
		&& FMath::IsNearlyEqual(LowerRight->End.X, Lower->End.X, 0.01f));

	for (const FSolidBodyPart& Part : Body.Parts)
	{
		TestFalse(TEXT("clavicle_l is not its own part"), Part.Bones.Num() == 1 && Part.Bones[0] == FName(TEXT("clavicle_l")));
		TestFalse(TEXT("upperarm_l is not its own part"), Part.Bones.Num() == 1 && Part.Bones[0] == FName(TEXT("upperarm_l")));
		for (const FName& Bone : Part.Bones)
		{
			const FString BoneName = Bone.ToString();
			TestFalse(TEXT("twists stay off the body"), BoneName.Contains(TEXT("twist")));
			TestFalse(TEXT("shoulder_jnt stays bare"), BoneName.Equals(TEXT("shoulder_jnt"), ESearchCase::IgnoreCase));
			TestFalse(TEXT("the head stays bare"), BoneName.Equals(TEXT("head"), ESearchCase::IgnoreCase));
			TestFalse(TEXT("fingers stay bare"), BoneName.StartsWith(TEXT("index_")) || BoneName.StartsWith(TEXT("thumb_")));
		}
	}
	TestTrue(TEXT("there is no head part"), Body.Find(TEXT("Head")) == nullptr);

	FSolidBodyMesh Scaled = FSolidBodyMesh::Make({});
	TArray<FSolidDeformBone> ScaledBones;
	FSolidDeformBone Pelvis;
	Pelvis.Name = TEXT("pelvis");
	ScaledBones.Add(Pelvis);
	FSolidDeformBone ThighJoint;
	ThighJoint.Name = FName(HipLeftBone);
	ThighJoint.ParentIndex = 0;
	ThighJoint.LocalTranslation = FVector(10.f, 1.f, -2.f);
	ScaledBones.Add(ThighJoint);
	FSolidDeformBone CalfJoint;
	CalfJoint.Name = FName(LeftThighBone);
	CalfJoint.ParentIndex = 1;
	CalfJoint.LocalTranslation = FVector(0.f, 0.f, -40.f);
	ScaledBones.Add(CalfJoint);
	FSolidDeformBone HeadJoint;
	HeadJoint.Name = TEXT("head");
	HeadJoint.ParentIndex = 0;
	HeadJoint.LocalTranslation = FVector(0.f, 0.f, 20.f);
	ScaledBones.Add(HeadJoint);
	FSolidBodyParams WideLegs;
	WideLegs.LegLength = 1.5f;
	WideLegs.Bulk = 1.25f;
	WideLegs.HipWidth = 1.4f;
	Scaled.Apply(ScaledBones, WideLegs);
	const FSolidBodyPart* ScaledThigh = Scaled.Find(TEXT("Thigh (L)"));
	const FSolidBodyPart* ScaledHip = Scaled.Find(TEXT("Hip"));
	TestTrue(
		TEXT("bulk widens the thigh"),
		ScaledThigh && FMath::IsNearlyEqual(ScaledThigh->Start.X, LeftThighTopDiameterCm * WideLegs.Bulk, 0.01f));
	TestTrue(
		TEXT("hip width widens the hip"),
		ScaledHip && FMath::IsNearlyEqual(ScaledHip->Start.X, HipSideDiameterCm * WideLegs.HipWidth, 0.01f));
	TestTrue(
		TEXT("leg length sizes the thigh bone"),
		FMath::IsNearlyEqual(ScaledBones[2].LocalTranslation.Size(), 40.f * WideLegs.LegLength, 0.01f));
	TestTrue(
		TEXT("the hip spreads the thigh joint sideways"),
		FMath::IsNearlyEqual(ScaledBones[1].LocalTranslation.X, 10.f * WideLegs.HipWidth, 0.01f)
		&& FMath::IsNearlyEqual(ScaledBones[1].LocalTranslation.Z, -2.f, 0.01f));
	TestTrue(TEXT("the head stays the viking length"), ScaledBones[3].LocalTranslation.Equals(FVector(0.f, 0.f, 20.f), 0.01f));

	const FSolidBodyMesh Bare = FSolidBodyMesh::Make({});
	TestTrue(TEXT("no rising spine leaves out the abdomen"), Bare.Find(TEXT("Abdomen")) == nullptr);
	TestTrue(TEXT("no rising spine leaves out the chest"), Bare.Find(TEXT("Chest")) == nullptr);
	const FSolidBodyPart* BareShoulders = Bare.Find(TEXT("Shoulders"));
	TestTrue(
		TEXT("without a chest the shoulder base stays the chest width"),
		BareShoulders && FMath::IsNearlyEqual(BareShoulders->Start.X, ChestSideDiameterCm, 0.01f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
