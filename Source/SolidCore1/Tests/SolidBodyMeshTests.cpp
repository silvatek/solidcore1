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
	const SolidBodyMesh::FSolidBodyMesh Catalogue = SolidBodyMesh::FSolidBodyMesh::Make(Named);
	int32 Recognized = 0;
	int32 ScaledBone = INDEX_NONE;
	for (int32 Index = 0; Index < Infos.Num(); ++Index)
	{
		bool bWrapped = false;
		for (const TUniquePtr<SolidBodyMesh::FSolidBodyPart>& Part : Catalogue.Parts)
		{
			if (Part && Part->Bones.Contains(Infos[Index].Name))
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
	FSolidBodyPartNameTest,
	"SolidCore1.Body.PartName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyPartNameTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	const FSolidPartSection Round = FSolidPartSection::Round(1.f);
	auto Named = [&](const ESolidPartType Type, const ESolidPartSide Side)
	{
		const TUniquePtr<FSolidBodyPart> Part = MakeBodyPart(
			Type,
			Side,
			ESolidPartColour::Shirt,
			Round,
			Round,
			{});
		return Part ? Part->Name() : FString();
	};
	TestEqual(TEXT("a left thigh"), Named(ESolidPartType::Thigh, ESolidPartSide::Left), FString(TEXT("Thigh (L)")));
	TestEqual(TEXT("a right upper arm"), Named(ESolidPartType::UpperArm, ESolidPartSide::Right), FString(TEXT("Upper arm (R)")));
	TestEqual(TEXT("a core hip has no side"), Named(ESolidPartType::Hip, ESolidPartSide::Core), FString(TEXT("Hip")));
	TestEqual(TEXT("the shoulders stay plural"), Named(ESolidPartType::Shoulders, ESolidPartSide::Core), FString(TEXT("Shoulders")));

	const TUniquePtr<FSolidBodyPart> Part = MakeBodyPart(
		ESolidPartType::LowerArm,
		ESolidPartSide::Left,
		ESolidPartColour::Shirt,
		Round,
		Round,
		{});
	const FSolidBodyPart* Lower = Part.Get();
	TestNotNull(TEXT("the lower arm builds"), Lower);
	if (Lower)
	{
		TestEqual(TEXT("the part names itself"), Lower->Name(), FString(TEXT("Lower arm (L)")));
		TestTrue(TEXT("the part stores its type"), Lower->Type == ESolidPartType::LowerArm);
		TestTrue(TEXT("the part stores its side"), Lower->Side == ESolidPartSide::Left);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyPartAliasTest,
	"SolidCore1.Body.PartAlias",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyPartAliasTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TestTrue(TEXT("a thigh is a tube"), PartShape(ESolidPartType::Thigh) == ESolidPartShape::Tube);
	TestTrue(TEXT("the shoulders are the shoulder mesh"), PartShape(ESolidPartType::Shoulders) == ESolidPartShape::Shoulder);
	TestTrue(TEXT("a hand is an ellipsoid"), PartShape(ESolidPartType::Hand) == ESolidPartShape::Hand);
	TestTrue(TEXT("the head is a sphere"), PartShape(ESolidPartType::Head) == ESolidPartShape::Head);

	const FSolidPartSection Round = RoundSection(LeftThighBottomDiameterCm);
	const FSolidPartSection ExpectedRound = FSolidPartSection::Round(LeftThighBottomDiameterCm);
	TestTrue(TEXT("a round section matches"), FMath::IsNearlyEqual(Round.X, ExpectedRound.X) && FMath::IsNearlyEqual(Round.Y, ExpectedRound.Y));
	const FSolidPartSection Oval = OvalSection(HipSideDiameterCm, HipDepthDiameterCm);
	const FSolidPartSection ExpectedOval = FSolidPartSection::Oval(HipSideDiameterCm, HipDepthDiameterCm);
	TestTrue(TEXT("an oval section matches"), FMath::IsNearlyEqual(Oval.X, ExpectedOval.X) && FMath::IsNearlyEqual(Oval.Y, ExpectedOval.Y));

	const FSolidPartSection Round20 = RoundSection(LeftThighTopDiameterCm);
	const TUniquePtr<FSolidBodyPart> Declared = MakeBodyPart(
		ESolidPartType::Thigh,
		ESolidPartSide::Left,
		ESolidPartColour::Trousers,
		Round20,
		RoundSection(LeftThighBottomDiameterCm),
		{});
	const FSolidBodyPart* DeclaredPart = Declared.Get();
	TestNotNull(TEXT("the short declaration builds a thigh"), DeclaredPart);
	if (DeclaredPart)
	{
		TestEqual(TEXT("the short declaration names the thigh"), DeclaredPart->Name(), FString(TEXT("Thigh (L)")));
		TestTrue(TEXT("the short declaration is trousers"), DeclaredPart->Colour == ESolidPartColour::Trousers);
		TestTrue(TEXT("the thigh wraps calf_l"), DeclaredPart->Bones.Num() == 1 && DeclaredPart->Bones[0] == FName(LeftThighBone));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyPartsTest,
	"SolidCore1.Body.Parts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyPartsTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	FSolidBodyMesh Body = FSolidBodyMesh::Make({});

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
	int32 Cursor = 0;
	for (const TCHAR* Name : Expected)
	{
		int32 Found = INDEX_NONE;
		for (int32 Index = Cursor; Index < Body.Parts.Num(); ++Index)
		{
			if (Body.Parts[Index] && Body.Parts[Index]->Name().Equals(Name, ESearchCase::IgnoreCase))
			{
				Found = Index;
				break;
			}
		}
		TestTrue(Name, Found != INDEX_NONE);
		if (Found == INDEX_NONE)
		{
			break;
		}
		Cursor = Found + 1;
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
		TestTrue(TEXT("the thigh is a tube"), Thigh->Shape == ESolidPartShape::Tube);
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
	for (const TUniquePtr<FSolidBodyPart>& Part : Body.Parts)
	{
		if (Part && Part->Name() == TEXT("Shoulders"))
		{
			Shoulders = Part.Get();
		}
	}
	TestTrue(TEXT("the shoulders are on the list"), Shoulders != nullptr);
	if (Shoulders && Chest)
	{
		TestTrue(TEXT("the shoulders use the shoulder mesh"), Shoulders->Shape == ESolidPartShape::Shoulder);
		TestEqual(TEXT("five shoulder bones"), Shoulders->Bones.Num(), 5);
		TestTrue(TEXT("the shoulder span is neck_01"), Shoulders->Bones.Num() >= 1 && Shoulders->Bones[0] == FName(ShoulderNeckBone));
		TestTrue(TEXT("the left clavicle is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderLeftClavicleBone)));
		TestTrue(TEXT("the right clavicle is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderRightClavicleBone)));
		TestTrue(TEXT("the left upper-arm bone is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderLeftUpperArmBone)));
		TestTrue(TEXT("the right upper-arm bone is inside the shoulders"), Shoulders->Bones.Contains(FName(ShoulderRightUpperArmBone)));
		TestTrue(TEXT("the shoulder base matches the chest"), FMath::IsNearlyEqual(Shoulders->Start.X, Chest->End.X, 0.01f));
		TestTrue(TEXT("the shoulder base is 20 cm deep"), FMath::IsNearlyEqual(Shoulders->Start.Y, TorsoDepthDiameterCm, 0.01f));
		TestTrue(TEXT("the shoulder top is the neck circle"), FMath::IsNearlyEqual(Shoulders->End.X, ShoulderTopRadiusCm * 2.f, 0.01f));
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
		TestTrue(TEXT("the lower arm is a tube"), Lower->Shape == ESolidPartShape::Tube);
	}
	const FSolidBodyPart* LowerRight = Body.Find(TEXT("Lower arm (R)"));
	TestTrue(TEXT("the right lower arm matches"), Lower && LowerRight && LowerRight->Bones.Num() == 1
		&& LowerRight->Bones[0] == FName(TEXT("hand_r"))
		&& FMath::IsNearlyEqual(LowerRight->Start.X, Lower->Start.X, 0.01f)
		&& FMath::IsNearlyEqual(LowerRight->End.X, Lower->End.X, 0.01f));

	for (const TUniquePtr<FSolidBodyPart>& Part : Body.Parts)
	{
		if (!Part)
		{
			continue;
		}
		TestFalse(TEXT("clavicle_l is not its own part"), Part->Bones.Num() == 1 && Part->Bones[0] == FName(TEXT("clavicle_l")));
		TestFalse(TEXT("upperarm_l is not its own part"), Part->Bones.Num() == 1 && Part->Bones[0] == FName(TEXT("upperarm_l")));
		for (const FName& Bone : Part->Bones)
		{
			const FString BoneName = Bone.ToString();
			TestFalse(TEXT("twists stay off the body"), BoneName.Contains(TEXT("twist")));
			TestFalse(TEXT("shoulder_jnt stays bare"), BoneName.Equals(TEXT("shoulder_jnt"), ESearchCase::IgnoreCase));
			const bool bFinger = BoneName.StartsWith(TEXT("index_")) || BoneName.StartsWith(TEXT("thumb_"));
			if (bFinger)
			{
				TestTrue(TEXT("a finger bone belongs to a hand"), Part->Name().Contains(TEXT("Hand")));
			}
		}
	}

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
	FSolidBodyMesh Scaled = FSolidBodyMesh::Make(ScaledBones);
	FSolidBodyParams WideLegs;
	WideLegs.LegLength = 1.5f;
	WideLegs.Bulk = 1.25f;
	WideLegs.HipWidth = 1.4f;
	Scaled.ApplyCharacterAttributes(WideLegs);
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
		FMath::IsNearlyEqual(Scaled.CharacterBones[2].LocalTranslation.Size(), 40.f * WideLegs.LegLength, 0.01f));
	TestTrue(
		TEXT("the hip spreads the thigh joint sideways"),
		FMath::IsNearlyEqual(Scaled.CharacterBones[1].LocalTranslation.X, 10.f * WideLegs.HipWidth, 0.01f)
		&& FMath::IsNearlyEqual(Scaled.CharacterBones[1].LocalTranslation.Z, -2.f, 0.01f));
	TestTrue(TEXT("the head stays the viking length"), Scaled.CharacterBones[3].LocalTranslation.Equals(FVector(0.f, 0.f, 20.f), 0.01f));

	const FSolidBodyMesh Bare = FSolidBodyMesh::Make({});
	const FSolidBodyPart* BareShoulders = Bare.Find(TEXT("Shoulders"));
	TestTrue(
		TEXT("without a chest the shoulder base stays the chest width"),
		BareShoulders && FMath::IsNearlyEqual(BareShoulders->Start.X, ChestSideDiameterCm, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyTorsoSectionsTest,
	"SolidCore1.Body.TorsoSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyTorsoSectionsTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	const FSolidBodyMesh Body = FSolidBodyMesh::Make({});
	const FSolidBodyPart* Abdomen = Body.Find(TEXT("Abdomen"));
	const FSolidBodyPart* Chest = Body.Find(TEXT("Chest"));
	TestTrue(TEXT("three rising bones still have an abdomen"), Abdomen != nullptr);
	TestTrue(TEXT("three rising bones still have a chest"), Chest != nullptr);
	if (Abdomen)
	{
		TestEqual(TEXT("the abdomen is the first rising bone"), Abdomen->Bones.Num(), 1);
		TestTrue(TEXT("the abdomen wraps spine_02"), Abdomen->Bones.Num() == 1 && Abdomen->Bones[0] == FName(TEXT("spine_02")));
	}
	if (Chest)
	{
		TestTrue(TEXT("the chest length is spine_03"), Chest->Bones.Num() >= 1 && Chest->Bones[0] == FName(SpineChestBone));
	}
	for (const TUniquePtr<FSolidBodyPart>& Part : Body.Parts)
	{
		if (!Part)
		{
			continue;
		}
		TestFalse(TEXT("a rising bone is not its own spine slab"), Part->Name().StartsWith(TEXT("Spine")));
	}

	float AbdomenBottom = 0.f;
	float AbdomenTop = 0.f;
	float ChestBottom = 0.f;
	float ChestTop = 0.f;
	TorsoSideDiameters(0, 2, AbdomenBottom, AbdomenTop);
	TorsoSideDiameters(1, 2, ChestBottom, ChestTop);
	TestTrue(
		TEXT("the abdomen still takes the first half"),
		Abdomen && FMath::IsNearlyEqual(Abdomen->Start.X, AbdomenBottom, 0.01f)
		&& FMath::IsNearlyEqual(Abdomen->End.X, AbdomenTop, 0.01f));
	TestTrue(
		TEXT("the chest still takes the second half"),
		Chest && FMath::IsNearlyEqual(Chest->Start.X, ChestBottom, 0.01f)
		&& FMath::IsNearlyEqual(Chest->End.X, ChestTop, 0.01f));
	const FSolidBodyPart* Shoulders = Body.Find(TEXT("Shoulders"));
	TestTrue(
		TEXT("the shoulder base follows the chest"),
		Shoulders && Chest && FMath::IsNearlyEqual(Shoulders->Start.X, Chest->End.X, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyPartSizesBonesTest,
	"SolidCore1.Body.PartSizesBones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyPartSizesBonesTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	const FQuat ParentRotation(FVector::ForwardVector, HALF_PI);
	const FVector SpreadLocal = ParentRotation.UnrotateVector(FVector(8.f, 4.f, 1.f));
	const FVector Untouched(1.f, 2.f, 3.f);

	TArray<FSolidDeformBone> Skeleton;
	FSolidDeformBone Pelvis;
	Pelvis.Name = TEXT("pelvis");
	Pelvis.LocalRotation = ParentRotation;
	Skeleton.Add(Pelvis);

	FSolidDeformBone Span;
	Span.Name = TEXT("spine_01");
	Span.ParentIndex = 0;
	Span.LocalTranslation = FVector(0.f, 0.f, -30.f);
	Skeleton.Add(Span);

	FSolidDeformBone Thigh;
	Thigh.Name = TEXT("thigh_l");
	Thigh.ParentIndex = 0;
	Thigh.LocalTranslation = SpreadLocal;
	Skeleton.Add(Thigh);

	FSolidDeformBone Head;
	Head.Name = TEXT("head");
	Head.ParentIndex = 0;
	Head.LocalTranslation = Untouched;
	Skeleton.Add(Head);

	const TUniquePtr<FSolidBodyPart> HipPart = MakeBodyPart(
		ESolidPartType::Hip,
		ESolidPartSide::Core,
		ESolidPartColour::Shirt,
		FSolidPartSection::Oval(20.f, 16.f),
		FSolidPartSection::Oval(18.f, 16.f),
		Skeleton);
	FSolidBodyPart* Hip = HipPart.Get();
	TestNotNull(TEXT("the hip builds"), Hip);
	if (!Hip)
	{
		return false;
	}

	FSolidBodyParams Wide;
	Wide.Height = 1.5f;
	const float HipScale = SolidBody::ParamMax;
	Wide.HipWidth = HipScale;
	Wide.TorsoGirth = 1.25f;
	Hip->ApplyCharacterAttributes(Wide);

	TestTrue(TEXT("hip width widens the part"), FMath::IsNearlyEqual(Hip->Start.X, 20.f * HipScale, 0.01f));
	TestTrue(TEXT("torso girth sets the hip depth"), FMath::IsNearlyEqual(Hip->Start.Y, 20.f, 0.01f));
	TestTrue(
		TEXT("height lengthens the span bone"),
		Hip->BoneLocals[1].LocalTranslation.Equals(FVector(0.f, 0.f, -45.f), 0.01f));
	const FVector ExpectedSpread = ParentRotation.UnrotateVector(FVector(8.f * HipScale, 4.f * HipScale, 1.f));
	TestTrue(
		TEXT("the part spreads its other bone on the world side axes"),
		Hip->BoneLocals[2].LocalTranslation.Equals(ExpectedSpread, 0.01f));
	TestTrue(TEXT("a bone outside the part stays put"), Hip->BoneLocals[3].LocalTranslation.Equals(Untouched, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHeadTest,
	"SolidCore1.Body.Head",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHeadTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TestTrue(TEXT("an eye joint is an eye"), IsEyeBoneName(TEXT("eye_jnt_l")));
	TestTrue(TEXT("an eyelid is an eye"), IsEyeBoneName(TEXT("eyelid_r")));
	TestFalse(TEXT("the head bone is not an eye"), IsEyeBoneName(TEXT("head")));
	TestFalse(TEXT("a thumb is not an eye"), IsEyeBoneName(TEXT("thumb_l")));
	TestTrue(TEXT("eye_jnt is an eye joint"), IsEyeJointBoneName(TEXT("Eye_Jnt_R")));
	TestFalse(TEXT("an eyelid is not an eye joint"), IsEyeJointBoneName(TEXT("eyelid_l")));

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	AddBone(TEXT("neck_01"), INDEX_NONE, FVector(0.f, 0.f, 90.f));
	AddBone(HeadBone, 0, FVector(0.f, 0.f, 10.f));
	AddBone(TEXT("eye_jnt_l"), 1, FVector(4.f, 6.f, 12.f));
	AddBone(TEXT("eye_jnt_r"), 1, FVector(-4.f, 6.f, 12.f));
	AddBone(TEXT("eyelid_l"), 1, FVector(3.f, 6.f, 18.f));
	AddBone(TEXT("thumb_l"), 1, FVector(1.f, 1.f, 1.f));

	const FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	const FSolidBodyPart* Head = Body.Find(TEXT("Head"));
	TestTrue(TEXT("the head is on the list"), Head != nullptr);
	if (Head)
	{
		TestTrue(TEXT("the head is skin"), Head->Colour == ESolidPartColour::Skin);
		TestEqual(TEXT("skin slot"), Head->MaterialSlot(), SkinMaterialSlot);
		TestEqual(TEXT("head, two eye joints, and an eyelid"), Head->Bones.Num(), 4);
		TestTrue(TEXT("the span is the head bone"), Head->Bones.Num() >= 1 && Head->Bones[0] == FName(HeadBone));
		TestTrue(TEXT("the left eye joint is inside the head"), Head->Bones.Contains(FName(TEXT("eye_jnt_l"))));
		TestTrue(TEXT("the right eye joint is inside the head"), Head->Bones.Contains(FName(TEXT("eye_jnt_r"))));
		TestTrue(TEXT("the eyelid is inside the head"), Head->Bones.Contains(FName(TEXT("eyelid_l"))));
		TestFalse(TEXT("the thumb stays out of the head"), Head->Bones.Contains(FName(TEXT("thumb_l"))));
		TestTrue(
			TEXT("PartBones reads the head"),
			Head->Bones == Head->PartBones());
	}

	TArray<FSolidBodyVert> Verts;
	TArray<FSolidBodyTri> Tris;
	FSolidBodyMesh Drawn = Body;
	Drawn.Draw(Verts, Tris);
	TestTrue(TEXT("the head draws triangles"), Tris.Num() > 0);

	int32 SkinVerts = 0;
	bool bSkinnedToHead = true;
	for (const FSolidBodyVert& Vert : Verts)
	{
		if (Vert.Slot != SkinMaterialSlot)
		{
			continue;
		}
		++SkinVerts;
		bSkinnedToHead = bSkinnedToHead && Vert.Bone == 1;
	}
	TestTrue(TEXT("the sphere has skin vertices"), SkinVerts > 0);
	TestTrue(TEXT("the sphere is skinned to the head"), bSkinnedToHead);

	FSolidBodyParams BigHead;
	const float HeadScale = SolidBody::ParamMax;
	BigHead.HeadSize = HeadScale;
	FSolidBodyMesh Scaled = Body;
	Scaled.ApplyCharacterAttributes(BigHead);
	TestTrue(TEXT("head size lengthens the head bone"), Scaled.CharacterBones[1].LocalTranslation.Equals(FVector(0.f, 0.f, 10.f * HeadScale), 0.01f));
	TestTrue(
		TEXT("head size spreads an eye"),
		FMath::IsNearlyEqual(Scaled.CharacterBones[2].LocalTranslation.X, 4.f * HeadScale, 0.01f)
		&& FMath::IsNearlyEqual(Scaled.CharacterBones[2].LocalTranslation.Y, 6.f * HeadScale, 0.01f)
		&& FMath::IsNearlyEqual(Scaled.CharacterBones[2].LocalTranslation.Z, 12.f, 0.01f));
	TestTrue(TEXT("a thumb stays the viking offset"), Scaled.CharacterBones[5].LocalTranslation.Equals(FVector(1.f, 1.f, 1.f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHeadShapeTest,
	"SolidCore1.Body.HeadShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHeadShapeTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TestEqual(TEXT("the head is three bone lengths tall"), HeadHeightInBoneLengths, 3.f);

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	const FVector HeadTranslation(0.f, 0.f, 10.f);
	const FVector EyeTranslation(4.f, 6.f, 12.f);
	AddBone(TEXT("neck_01"), INDEX_NONE, FVector(0.f, 0.f, 90.f));
	AddBone(HeadBone, 0, HeadTranslation);
	AddBone(TEXT("eye_jnt_l"), 1, EyeTranslation);
	AddBone(TEXT("eyelid_l"), 1, FVector(3.f, 6.f, 18.f));

	const float HeadLength = static_cast<float>(HeadTranslation.Size());
	const float EyeLength = static_cast<float>(EyeTranslation.Size());
	const float SphereHeight = HeadLength * HeadHeightInBoneLengths;

	TArray<FSolidBodyVert> Verts;
	TArray<FSolidBodyTri> Tris;
	FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	Body.Draw(Verts, Tris);

	float MinZ = 1.e9f;
	float MaxZ = -1.e9f;
	float MaxRadial = 0.f;
	int32 SkinVerts = 0;
	for (const FSolidBodyVert& Vert : Verts)
	{
		if (Vert.Slot != SkinMaterialSlot)
		{
			continue;
		}
		++SkinVerts;
		MinZ = FMath::Min(MinZ, static_cast<float>(Vert.Position.Z));
		MaxZ = FMath::Max(MaxZ, static_cast<float>(Vert.Position.Z));
		const float Radial = static_cast<float>(FVector(Vert.Position.X, Vert.Position.Y, 0.f).Size());
		MaxRadial = FMath::Max(MaxRadial, Radial);
	}
	TestTrue(TEXT("the head draws a sphere"), SkinVerts > 0);
	TestTrue(TEXT("the height is three head-bone lengths"), FMath::IsNearlyEqual(MaxZ - MinZ, SphereHeight, 0.01f));
	TestTrue(TEXT("the horizontal radius is the eye joint length"), FMath::IsNearlyEqual(MaxRadial, EyeLength, 0.05f));
	TestTrue(TEXT("an eyelid does not set the radius"), !FMath::IsNearlyEqual(MaxRadial, static_cast<float>(FVector(3.f, 6.f, 18.f).Size()), 0.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyDetailTest,
	"SolidCore1.Body.Detail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyDetailTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TestEqual(TEXT("every part uses five times the first ring"), BodyPartDetail, 5);
	TestEqual(TEXT("tubes, shoulders, and the head share that ring"), BodyPartSides, 10 * BodyPartDetail);
	TestEqual(TEXT("the head uses that ring"), HeadSideCount, BodyPartSides);
	TestEqual(TEXT("the shoulders use that ring"), ShoulderSideCount, BodyPartSides);

	FSolidBodyPart Limb(
		ESolidPartType::Thigh,
		ESolidPartSide::Left,
		ESolidPartColour::Trousers,
		FSolidPartSection::Round(LeftThighTopDiameterCm),
		FSolidPartSection::Round(LeftThighBottomDiameterCm),
		{});
	TArray<FSolidBodyVert> LimbVerts;
	TArray<FSolidBodyTri> LimbTris;
	Limb.Draw(
		LimbVerts,
		LimbTris,
		0,
		1,
		FVector(0.f, 0.f, 40.f),
		FVector(0.f, 0.f, 0.f),
		nullptr,
		nullptr,
		0,
		FVector::ForwardVector);
	TestEqual(TEXT("a limb has two rings at the shared side count"), LimbVerts.Num(), 2 * BodyPartSides);

	const FSolidBodyMesh Catalogue = FSolidBodyMesh::Make({});
	const FSolidBodyPart* Found = Catalogue.Find(TEXT("Shoulders"));
	TestTrue(TEXT("the shoulders are on the list"), Found != nullptr);
	if (Found)
	{
		const TUniquePtr<FSolidBodyPart> Shoulders = Found->Clone();
		FSolidBodyPart* ShoulderMesh = Shoulders.Get();
		TestNotNull(TEXT("a copy keeps the shoulders"), ShoulderMesh);
		if (ShoulderMesh)
		{
			const FVector SegmentStart[] = {
				FVector(0.f, 0.f, 0.f),
				FVector(0.f, 0.f, 0.f),
				FVector(7.f, 3.f, 10.f),
				FVector(-7.f, 3.f, 10.f),
			};
			const FVector SegmentEnd[] = {
				FVector(7.f, 3.f, 10.f),
				FVector(-7.f, 3.f, 10.f),
				FVector(20.f, -2.f, 8.f),
				FVector(-20.f, -2.f, 8.f),
			};
			TArray<FSolidBodyVert> ShoulderVerts;
			TArray<FSolidBodyTri> ShoulderTris;
			ShoulderMesh->Draw(
				ShoulderVerts,
				ShoulderTris,
				0,
				1,
				FVector(0.f, 0.f, 0.f),
				FVector(0.f, 0.f, 20.f),
				SegmentStart,
				SegmentEnd,
				4,
				FVector::ForwardVector);
			TestEqual(TEXT("the shoulders have three rings at the shared side count"), ShoulderVerts.Num(), 3 * ShoulderSideCount);
		}
	}

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	AddBone(TEXT("neck_01"), INDEX_NONE, FVector(0.f, 0.f, 90.f));
	AddBone(HeadBone, 0, FVector(0.f, 0.f, 10.f));
	AddBone(TEXT("eye_jnt_l"), 1, FVector(4.f, 6.f, 12.f));

	TArray<FVector> Positions;
	SolidBodyDeform::ComponentPositions(Bones, Positions);
	TArray<FSolidBodyVert> HeadVerts;
	TArray<FSolidBodyTri> HeadTris;
	FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	Body.Draw(HeadVerts, HeadTris);

	float MinZ = 1.e9f;
	int32 SkinVerts = 0;
	for (const FSolidBodyVert& Vert : HeadVerts)
	{
		if (Vert.Slot != SkinMaterialSlot)
		{
			continue;
		}
		++SkinVerts;
		MinZ = FMath::Min(MinZ, static_cast<float>(Vert.Position.Z));
	}
	const int32 HeadRingVerts = 2 + (HeadRingCount - 2) * HeadSideCount;
	TestEqual(TEXT("the head has five times the ring vertices, plus the poles"), SkinVerts, HeadRingVerts);
	TestTrue(TEXT("the sphere starts at the base of the head bone"), FMath::IsNearlyEqual(MinZ, 90.f, 0.01f));
	TestTrue(TEXT("the head joint is above that base"), Positions.IsValidIndex(1) && Positions[1].Z > MinZ + 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHandsTest,
	"SolidCore1.Body.Hands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHandsTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TestEqual(TEXT("a hand is 15 cm across the fingers"), HandWidthDiameterCm, 15.f);
	TestEqual(TEXT("a hand is 3 cm thick"), HandThicknessDiameterCm, 3.f);

	TestTrue(TEXT("an index bone is a finger"), IsFingerBoneName(TEXT("index_01_l")));
	TestTrue(TEXT("a thumb bone is a finger"), IsFingerBoneName(TEXT("Thumb_L")));
	TestTrue(TEXT("a middle bone is a finger"), IsFingerBoneName(TEXT("middle_02_r")));
	TestTrue(TEXT("a ring bone is a finger"), IsFingerBoneName(TEXT("ring_01_l")));
	TestTrue(TEXT("a pinky bone is a finger"), IsFingerBoneName(TEXT("pinky_03_l")));
	TestTrue(TEXT("a little bone is a finger"), IsFingerBoneName(TEXT("little_01_r")));
	TestFalse(TEXT("a twist is not a finger"), IsFingerBoneName(TEXT("index_01_twist_l")));
	TestFalse(TEXT("a toe is not a finger"), IsFingerBoneName(LeftToeBone));
	TestFalse(TEXT("the hand bone is not a finger"), IsFingerBoneName(TEXT("hand_l")));
	TestFalse(TEXT("a beard joint is not a finger"), IsFingerBoneName(TEXT("beard_middle_jnt_2")));

	const FSolidBodyMesh Catalogue = FSolidBodyMesh::Make({});
	const FSolidBodyPart* ListedLeft = Catalogue.Find(TEXT("Hand (L)"));
	const FSolidBodyPart* ListedRight = Catalogue.Find(TEXT("Hand (R)"));
	TestTrue(TEXT("the left hand is on the list"), ListedLeft != nullptr);
	TestTrue(TEXT("the right hand is on the list"), ListedRight != nullptr);
	if (ListedLeft && ListedRight)
	{
		TestTrue(TEXT("the left hand is skin"), ListedLeft->Colour == ESolidPartColour::Skin);
		TestTrue(TEXT("the right hand is skin"), ListedRight->Colour == ESolidPartColour::Skin);
		TestEqual(TEXT("left hand skin slot"), ListedLeft->MaterialSlot(), SkinMaterialSlot);
		TestEqual(TEXT("right hand skin slot"), ListedRight->MaterialSlot(), SkinMaterialSlot);
	}

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	AddBone(TEXT("root"), INDEX_NONE, FVector::ZeroVector);
	AddBone(TEXT("lowerarm_l"), 0, FVector::ZeroVector);
	AddBone(LeftHandBone, 1, FVector(0.f, 0.f, 10.f));
	AddBone(TEXT("index_01_l"), 2, FVector(8.f, 0.f, 4.f));
	AddBone(TEXT("pinky_01_l"), 2, FVector(-6.f, 0.f, 5.f));
	AddBone(TEXT("middle_02_l"), 3, FVector(0.f, 0.f, 3.f));
	AddBone(TEXT("little_01_l"), 2, FVector(2.f, 0.f, 4.f));
	AddBone(TEXT("index_01"), 2, FVector(1.f, 0.f, 2.f));
	AddBone(TEXT("thumb_01_r"), 0, FVector(1.f, 2.f, 3.f));
	AddBone(LeftToeBone, 0, FVector(0.f, 0.f, -4.f));
	AddBone(TEXT("index_01_twist_l"), 2, FVector(1.f, 0.f, 1.f));
	AddBone(TEXT("lowerarm_r"), 0, FVector::ZeroVector);
	AddBone(RightHandBone, 11, FVector(12.f, 0.f, 0.f));
	AddBone(TEXT("index_01_r"), 12, FVector(4.f, 7.f, 0.f));
	AddBone(TEXT("ring_01_r"), 12, FVector(3.f, -5.f, 0.f));

	const FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	const FSolidBodyPart* Left = Body.Find(TEXT("Hand (L)"));
	const FSolidBodyPart* Right = Body.Find(TEXT("Hand (R)"));
	TestTrue(TEXT("the skeleton keeps the left hand"), Left != nullptr);
	TestTrue(TEXT("the skeleton keeps the right hand"), Right != nullptr);
	if (Left && Right)
	{
		TestTrue(
			TEXT("PartBones reads the left hand"),
			Left->Bones == Left->PartBones());
		TestTrue(
			TEXT("PartBones reads the right hand"),
			Right->Bones == Right->PartBones());
		TestTrue(TEXT("the left index is inside the left hand"), Left->Bones.Contains(FName(TEXT("index_01_l"))));
		TestTrue(TEXT("the left pinky is inside the left hand"), Left->Bones.Contains(FName(TEXT("pinky_01_l"))));
		TestTrue(TEXT("the left middle is inside the left hand"), Left->Bones.Contains(FName(TEXT("middle_02_l"))));
		TestTrue(TEXT("the left little is inside the left hand"), Left->Bones.Contains(FName(TEXT("little_01_l"))));
		TestTrue(TEXT("an unsided finger on hand_l joins the left hand"), Left->Bones.Contains(FName(TEXT("index_01"))));
		TestFalse(TEXT("the right thumb stays out of the left hand"), Left->Bones.Contains(FName(TEXT("thumb_01_r"))));
		TestFalse(TEXT("the toe stays out of the left hand"), Left->Bones.Contains(FName(LeftToeBone)));
		TestFalse(TEXT("a finger twist stays out of the left hand"), Left->Bones.Contains(FName(TEXT("index_01_twist_l"))));
		TestTrue(TEXT("the right index is inside the right hand"), Right->Bones.Contains(FName(TEXT("index_01_r"))));
		TestTrue(TEXT("the right ring is inside the right hand"), Right->Bones.Contains(FName(TEXT("ring_01_r"))));
		TestTrue(TEXT("a right-named thumb joins the right hand"), Right->Bones.Contains(FName(TEXT("thumb_01_r"))));
		TestFalse(TEXT("the left index stays out of the right hand"), Right->Bones.Contains(FName(TEXT("index_01_l"))));
		TestFalse(TEXT("the unsided left finger stays out of the right hand"), Right->Bones.Contains(FName(TEXT("index_01"))));
		const TUniquePtr<FSolidBodyPart> ThighPart = MakeBodyPart(
			ESolidPartType::Thigh,
			ESolidPartSide::Left,
			ESolidPartColour::Trousers,
			FSolidPartSection::Round(LeftThighTopDiameterCm),
			FSolidPartSection::Round(LeftThighBottomDiameterCm),
			Bones);
		const TUniquePtr<FSolidBodyPart> BareThighPart = MakeBodyPart(
			ESolidPartType::Thigh,
			ESolidPartSide::Left,
			ESolidPartColour::Trousers,
			FSolidPartSection::Round(LeftThighTopDiameterCm),
			FSolidPartSection::Round(LeftThighBottomDiameterCm),
			{});
		const FSolidBodyPart* Thigh = ThighPart.Get();
		const FSolidBodyPart* BareThigh = BareThighPart.Get();
		TestTrue(
			TEXT("a thigh keeps its viking bone"),
			Thigh && BareThigh
			&& Thigh->PartBones() == BareThigh->PartBones()
			&& Thigh->Shape == PartShape(ESolidPartType::Thigh));
	}

	const FSolidBodyPart* Head = Body.Find(TEXT("Head"));
	TestFalse(TEXT("fingers stay out of the head"), Head && Head->Bones.Contains(FName(TEXT("index_01_l"))));

	FSolidBodyParams LongArms;
	LongArms.ArmLength = 1.5f;
	LongArms.Height = 2.f;
	LongArms.Bulk = 1.8f;
	LongArms.TorsoGirth = 1.7f;
	FSolidBodyMesh Scaled = Body;
	Scaled.ApplyCharacterAttributes(LongArms);
	TestTrue(
		TEXT("arm length sizes the hand bone once"),
		Scaled.CharacterBones[2].LocalTranslation.Equals(FVector(0.f, 0.f, 15.f), 0.01f));
	TestTrue(
		TEXT("the hand leaves a finger at the viking offset"),
		Scaled.CharacterBones[3].LocalTranslation.Equals(FVector(8.f, 0.f, 4.f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHandShapeTest,
	"SolidCore1.Body.HandShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHandShapeTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	const FVector LeftFinger(0.f, 0.f, 12.f);
	const FVector RightFinger(8.f, 0.f, 0.f);
	AddBone(TEXT("root"), INDEX_NONE, FVector::ZeroVector);
	AddBone(TEXT("lowerarm_l"), 0, FVector::ZeroVector);
	AddBone(LeftHandBone, 1, FVector(0.f, 0.f, 10.f));
	AddBone(TEXT("index_01_l"), 2, LeftFinger);
	AddBone(TEXT("pinky_01_l"), 2, FVector(6.f, 0.f, 3.f));
	AddBone(TEXT("middle_02_l"), 3, FVector(0.f, 0.f, 2.f));
	AddBone(TEXT("lowerarm_r"), 0, FVector::ZeroVector);
	AddBone(RightHandBone, 6, FVector(12.f, 0.f, 0.f));
	AddBone(TEXT("index_01_r"), 7, RightFinger);
	AddBone(TEXT("ring_01_r"), 7, FVector(2.f, 5.f, 0.f));

	const FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	const FSolidBodyPart* Left = Body.Find(TEXT("Hand (L)"));
	const FSolidBodyPart* Right = Body.Find(TEXT("Hand (R)"));
	const FSolidBodyPart* Lower = Body.Find(TEXT("Lower arm (L)"));
	TestTrue(TEXT("the left hand is on the list"), Left != nullptr);
	TestTrue(TEXT("the right hand is on the list"), Right != nullptr);
	TestTrue(TEXT("the lower arm is on the list"), Lower != nullptr);
	if (Left && Right && Lower)
	{
		TestTrue(TEXT("the lower arm ends on the hand bone"), Lower->Bones.Num() == 1 && Lower->Bones[0] == FName(LeftHandBone));
		TestFalse(TEXT("the hand bone stays on the lower arm"), Left->Bones.Contains(FName(LeftHandBone)));
		TestFalse(TEXT("the right hand bone stays on the lower arm"), Right->Bones.Contains(FName(RightHandBone)));
		TestTrue(TEXT("the left hand starts from a finger on the hand bone"), Left->Bones.Num() >= 1 && Left->Bones[0] == FName(TEXT("index_01_l")));
		TestTrue(TEXT("the right hand starts from a finger on the hand bone"), Right->Bones.Num() >= 1 && Right->Bones[0] == FName(TEXT("index_01_r")));
		TestTrue(TEXT("the left pinky is inside the hand"), Left->Bones.Contains(FName(TEXT("pinky_01_l"))));
		TestTrue(TEXT("a further finger joint is inside the hand"), Left->Bones.Contains(FName(TEXT("middle_02_l"))));
		TestTrue(TEXT("the right ring is inside the hand"), Right->Bones.Contains(FName(TEXT("ring_01_r"))));
	}

	TArray<FVector> Positions;
	SolidBodyDeform::ComponentPositions(Bones, Positions);
	TArray<FSolidBodyVert> Verts;
	TArray<FSolidBodyTri> Tris;
	FSolidBodyMesh Drawn = Body;
	Drawn.Draw(Verts, Tris);

	const int32 HandRingVerts = 2 + (HeadRingCount - 2) * HeadSideCount;
	auto AxisValue = [](const FVector& Delta, const int32 Axis) -> float
	{
		return Axis == 0 ? static_cast<float>(Delta.X) : (Axis == 1 ? static_cast<float>(Delta.Y) : static_cast<float>(Delta.Z));
	};
	auto MeasureHand = [&](const int32 HandBone, const int32 LongAxis, const int32 WideAxis, const int32 ThinAxis, float& OutMin, float& OutMax, float& OutWide, float& OutThin, int32& OutCount)
	{
		OutMin = 1.e9f;
		OutMax = -1.e9f;
		OutWide = 0.f;
		OutThin = 0.f;
		OutCount = 0;
		const FVector Origin = Positions[HandBone];
		for (const FSolidBodyVert& Vert : Verts)
		{
			if (Vert.Bone != HandBone || Vert.Slot != SkinMaterialSlot)
			{
				continue;
			}
			++OutCount;
			const FVector Delta = Vert.Position - Origin;
			const float Along = AxisValue(Delta, LongAxis);
			OutMin = FMath::Min(OutMin, Along);
			OutMax = FMath::Max(OutMax, Along);
			OutWide = FMath::Max(OutWide, FMath::Abs(AxisValue(Delta, WideAxis)));
			OutThin = FMath::Max(OutThin, FMath::Abs(AxisValue(Delta, ThinAxis)));
		}
	};

	float LeftMin = 0.f;
	float LeftMax = 0.f;
	float LeftWide = 0.f;
	float LeftThin = 0.f;
	int32 LeftCount = 0;
	MeasureHand(2, 2, 0, 1, LeftMin, LeftMax, LeftWide, LeftThin, LeftCount);
	TestEqual(TEXT("the left hand is one ellipsoid"), LeftCount, HandRingVerts);
	TestTrue(TEXT("the left hand starts at the bottom of the hand bone"), FMath::IsNearlyEqual(LeftMin, 0.f, 0.01f));
	TestTrue(
		TEXT("the left hand ends at the top of the finger"),
		FMath::IsNearlyEqual(LeftMax - LeftMin, static_cast<float>(LeftFinger.Size()), 0.01f));
	TestTrue(TEXT("the left hand is 15 cm across"), FMath::IsNearlyEqual(LeftWide, HandWidthDiameterCm * 0.5f, 0.05f));
	TestTrue(TEXT("the left hand is 3 cm thick"), FMath::IsNearlyEqual(LeftThin, HandThicknessDiameterCm * 0.5f, 0.05f));

	float RightMin = 0.f;
	float RightMax = 0.f;
	float RightWide = 0.f;
	float RightThin = 0.f;
	int32 RightCount = 0;
	MeasureHand(7, 0, 1, 2, RightMin, RightMax, RightWide, RightThin, RightCount);
	TestEqual(TEXT("the right hand is one ellipsoid"), RightCount, HandRingVerts);
	TestTrue(TEXT("the right hand starts at the bottom of the hand bone"), FMath::IsNearlyEqual(RightMin, 0.f, 0.01f));
	TestTrue(
		TEXT("the right hand ends at the top of the finger"),
		FMath::IsNearlyEqual(RightMax - RightMin, static_cast<float>(RightFinger.Size()), 0.01f));
	TestTrue(TEXT("the right hand is 15 cm across"), FMath::IsNearlyEqual(RightWide, HandWidthDiameterCm * 0.5f, 0.05f));
	TestTrue(TEXT("the right hand is 3 cm thick"), FMath::IsNearlyEqual(RightThin, HandThicknessDiameterCm * 0.5f, 0.05f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHandFingersTest,
	"SolidCore1.Body.HandFingers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHandFingersTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bones.Add(Bone);
	};
	const TCHAR* LeftNames[] = {
		TEXT("thumb_01_l"), TEXT("thumb_02_l"), TEXT("thumb_03_l"),
		TEXT("index_01_l"), TEXT("index_02_l"), TEXT("index_03_l"),
		TEXT("middle_01_l"), TEXT("middle_02_l"), TEXT("middle_03_l"),
		TEXT("ring_01_l"), TEXT("ring_02_l"), TEXT("ring_03_l"),
		TEXT("pinky_01_l"), TEXT("pinky_02_l"), TEXT("pinky_03_l"),
	};
	const TCHAR* RightNames[] = {
		TEXT("thumb_01_r"), TEXT("thumb_02_r"), TEXT("thumb_03_r"),
		TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r"),
		TEXT("middle_01_r"), TEXT("middle_02_r"), TEXT("middle_03_r"),
		TEXT("ring_01_r"), TEXT("ring_02_r"), TEXT("ring_03_r"),
		TEXT("pinky_01_r"), TEXT("pinky_02_r"), TEXT("pinky_03_r"),
	};
	AddBone(LeftHandBone, INDEX_NONE);
	for (const TCHAR* BoneName : LeftNames)
	{
		AddBone(BoneName, 0);
	}
	const int32 RightHand = Bones.Num();
	AddBone(RightHandBone, INDEX_NONE);
	for (const TCHAR* BoneName : RightNames)
	{
		AddBone(BoneName, RightHand);
	}

	const FSolidBodyMesh Catalogue = FSolidBodyMesh::Make(Bones);
	const FSolidBodyPart* Left = Catalogue.Find(TEXT("Hand (L)"));
	const FSolidBodyPart* Right = Catalogue.Find(TEXT("Hand (R)"));
	TestTrue(TEXT("the left hand is on the list"), Left != nullptr);
	TestTrue(TEXT("the right hand is on the list"), Right != nullptr);
	if (!Left || !Right)
	{
		return false;
	}

	TestEqual(TEXT("the left hand lists every finger joint"), Left->Bones.Num(), static_cast<int32>(UE_ARRAY_COUNT(LeftNames)));
	TestEqual(TEXT("the right hand lists every finger joint"), Right->Bones.Num(), static_cast<int32>(UE_ARRAY_COUNT(RightNames)));
	TestFalse(TEXT("the left hand leaves the hand bone on the lower arm"), Left->Bones.Contains(FName(LeftHandBone)));
	TestFalse(TEXT("the right hand leaves the hand bone on the lower arm"), Right->Bones.Contains(FName(RightHandBone)));
	for (const TCHAR* BoneName : LeftNames)
	{
		TestTrue(BoneName, Left->Bones.Contains(FName(BoneName)));
	}
	for (const TCHAR* BoneName : RightNames)
	{
		TestTrue(BoneName, Right->Bones.Contains(FName(BoneName)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyBaseTest,
	"SolidCore1.Body.Base",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyBaseTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	const TArray<FSolidDeformBone> NoBones;
	TestTrue(TEXT("no parts leave the base at the mesh origin"), FMath::IsNearlyEqual(BodyBaseZ(NoBones), 0.f, 0.01f));

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	AddBone(TEXT("pelvis"), INDEX_NONE, FVector::ZeroVector);
	AddBone(LeftThighBone, 0, FVector(0.f, 0.f, -40.f));
	AddBone(LeftCalfBone, 1, FVector(0.f, 0.f, -30.f));
	AddBone(LeftBallBone, 2, FVector(16.f, 0.f, 0.f));
	AddBone(LeftToeBone, 3, FVector(0.f, 0.f, -8.f));

	const float NeutralBase = BodyBaseZ(Bones);
	TestTrue(TEXT("the downward toe sets the base"), FMath::IsNearlyEqual(NeutralBase, -78.f, 0.01f));
	TestTrue(TEXT("the base is below the foot bone"), NeutralBase < -70.f);

	const float HalfHeight = 96.f;
	TestTrue(
		TEXT("the offset puts that base on the capsule bottom"),
		FMath::IsNearlyEqual(BaseMeshOffset(HalfHeight, NeutralBase) + NeutralBase, -HalfHeight, 0.01f));

	FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	FSolidBodyParams LongLegs;
	LongLegs.Height = 1.f;
	LongLegs.LegLength = SolidBody::ParamMax;
	Body.ApplyCharacterAttributes(LongLegs);
	const float LongBase = BodyBaseZ(Body);
	TestTrue(
		TEXT("longer legs lower the base"),
		FMath::IsNearlyEqual(LongBase, -78.f * SolidBody::ParamMax, 0.01f));
	TestTrue(TEXT("the longer base is lower"), LongBase < NeutralBase);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHandstandBaseTest,
	"SolidCore1.Body.HandstandBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHandstandBaseTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	AddBone(TEXT("pelvis"), INDEX_NONE, FVector::ZeroVector);
	AddBone(LeftThighBone, 0, FVector(0.f, 0.f, -40.f));
	AddBone(LeftCalfBone, 1, FVector(0.f, 0.f, -30.f));
	AddBone(LeftBallBone, 2, FVector(16.f, 0.f, 0.f));
	AddBone(LeftToeBone, 3, FVector(0.f, 0.f, -8.f));
	AddBone(LeftHandBone, 0, FVector(0.f, 0.f, -40.f));
	const int32 HandIndex = Bones.Num() - 1;
	AddBone(TEXT("index_01_l"), HandIndex, FVector(0.f, 0.f, -50.f));
	const int32 FingerIndex = Bones.Num() - 1;

	TArray<FVector> Positions;
	SolidBodyDeform::ComponentPositions(Bones, Positions);
	int32 ToeIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		if (Bones[Index].Name.ToString().Equals(LeftToeBone, ESearchCase::IgnoreCase))
		{
			ToeIndex = Index;
			break;
		}
	}
	TestTrue(TEXT("the toe bone is in the pose"), Bones.IsValidIndex(ToeIndex));
	if (!Bones.IsValidIndex(ToeIndex))
	{
		return false;
	}
	const float ToeZ = static_cast<float>(Positions[ToeIndex].Z);
	const float HandZ = static_cast<float>(Positions[FingerIndex].Z);
	const float BaseZ = BodyBaseZ(Bones);
	const FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	const float WornBase = BodyBaseZ(Body);

	TestTrue(TEXT("the hand hangs below the toe"), HandZ < ToeZ);
	TestTrue(TEXT("a handstand plants the hand"), FMath::IsNearlyEqual(BaseZ, HandZ, 0.01f));
	TestTrue(TEXT("the worn body plants the same hand"), FMath::IsNearlyEqual(WornBase, HandZ, 0.01f));
	TestTrue(TEXT("the feet lift off that base"), ToeZ > BaseZ);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyPartClassTest,
	"SolidCore1.Body.PartClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyPartClassTest::RunTest(const FString& Parameters)
{
	using namespace SolidBodyMesh;

	TArray<FSolidDeformBone> Bones;
	auto AddBone = [&Bones](const TCHAR* Name, const int32 Parent, const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.Name = Name;
		Bone.ParentIndex = Parent;
		Bone.LocalTranslation = Translation;
		Bones.Add(Bone);
	};
	AddBone(TEXT("pelvis"), INDEX_NONE, FVector::ZeroVector);
	AddBone(LeftThighBone, 0, FVector(0.f, 0.f, -40.f));
	AddBone(LeftCalfBone, 0, FVector(0.f, 0.f, -30.f));
	AddBone(LeftBallBone, 0, FVector(0.f, 0.f, -10.f));
	AddBone(LeftToeBone, 0, FVector(0.f, 0.f, -8.f));
	AddBone(SpineAbdomenBone, 0, FVector(0.f, 0.f, 12.f));
	AddBone(TEXT("lowerarm_l"), 0, FVector(0.f, 0.f, -22.f));
	AddBone(LeftHandBone, 0, FVector(0.f, 0.f, -18.f));

	const FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	const FSolidBodyPart* Calf = Body.Find(TEXT("Calf (L)"));
	const FSolidBodyPart* Foot = Body.Find(TEXT("Foot (L)"));
	const FSolidBodyPart* Toe = Body.Find(TEXT("Toe (L)"));
	const FSolidBodyPart* Abdomen = Body.Find(TEXT("Abdomen"));
	const FSolidBodyPart* Lower = Body.Find(TEXT("Lower arm (L)"));
	TestTrue(TEXT("the calf wraps foot_l"), Calf && Calf->Bones.Num() == 1 && Calf->Bones[0] == FName(LeftCalfBone));
	TestTrue(TEXT("the foot wraps ball_l"), Foot && Foot->Bones.Num() == 1 && Foot->Bones[0] == FName(LeftBallBone));
	TestTrue(TEXT("the toe wraps the toe bone"), Toe && Toe->Bones.Num() == 1 && Toe->Bones[0] == FName(LeftToeBone));
	TestTrue(
		TEXT("those limbs stay tubes"),
		Calf && Foot && Toe && Lower
		&& Calf->Shape == ESolidPartShape::Tube
		&& Foot->Shape == ESolidPartShape::Tube
		&& Toe->Shape == ESolidPartShape::Tube
		&& Lower->Shape == ESolidPartShape::Tube);
	if (!Abdomen || !Lower)
	{
		return false;
	}
	const float AbdomenSide = Abdomen->Start.X;
	const float LowerSide = Lower->Start.X;

	FSolidBodyMesh Copied = Body;
	FSolidBodyParams Params;
	Params.LegLength = 1.5f;
	Params.Height = 1.2f;
	Params.Bulk = 1.25f;
	Params.TorsoGirth = 1.4f;
	Params.ArmLength = 1.3f;
	Copied.ApplyCharacterAttributes(Params);

	const FSolidBodyPart* CopiedThigh = Copied.Find(TEXT("Thigh (L)"));
	const FSolidBodyPart* CopiedAbdomen = Copied.Find(TEXT("Abdomen"));
	const FSolidBodyPart* CopiedLower = Copied.Find(TEXT("Lower arm (L)"));
	TestTrue(
		TEXT("a copy keeps the thigh on leg bulk"),
		CopiedThigh && FMath::IsNearlyEqual(CopiedThigh->Start.X, LeftThighTopDiameterCm * Params.Bulk, 0.01f));
	TestTrue(
		TEXT("a copy sizes the thigh bone by leg length and height"),
		FMath::IsNearlyEqual(Copied.CharacterBones[1].LocalTranslation.Size(), 40.f * Params.LegLength * Params.Height, 0.01f));
	TestTrue(
		TEXT("a copy sizes the calf bone by leg length and height"),
		FMath::IsNearlyEqual(Copied.CharacterBones[2].LocalTranslation.Size(), 30.f * Params.LegLength * Params.Height, 0.01f));
	TestTrue(
		TEXT("a copy keeps the abdomen on torso girth"),
		CopiedAbdomen && FMath::IsNearlyEqual(CopiedAbdomen->Start.X, AbdomenSide * Params.TorsoGirth, 0.01f));
	TestTrue(
		TEXT("a copy sizes the abdomen bone by height"),
		FMath::IsNearlyEqual(Copied.CharacterBones[5].LocalTranslation.Size(), 12.f * Params.Height, 0.01f));
	TestTrue(
		TEXT("a copy keeps the lower arm on bulk"),
		CopiedLower && FMath::IsNearlyEqual(CopiedLower->Start.X, LowerSide * Params.Bulk, 0.01f));
	TestTrue(
		TEXT("a copy sizes the lower arm bone by arm length"),
		FMath::IsNearlyEqual(Copied.CharacterBones[7].LocalTranslation.Size(), 18.f * Params.ArmLength, 0.01f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
