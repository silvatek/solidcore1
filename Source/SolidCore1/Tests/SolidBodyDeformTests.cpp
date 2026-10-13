#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBodyDeform.h"
#include "SolidBodyParams.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr double BodyTol = 0.001;

	FSolidDeformBone MakeBone(
		const int32 Parent,
		const ESolidBodyBoneRole Role,
		const FVector& Translation)
	{
		FSolidDeformBone Bone;
		Bone.ParentIndex = Parent;
		Bone.Role = Role;
		Bone.LocalTranslation = Translation;
		return Bone;
	}

	FSolidDeformVertex MakeVertex(const FVector& Position, const int32 BoneIndex)
	{
		FSolidDeformVertex Vertex;
		Vertex.Position = Position;
		Vertex.BoneIndex[0] = BoneIndex;
		Vertex.Weight[0] = 1.f;
		return Vertex;
	}

	bool Near(const FVector& A, const FVector& B)
	{
		return A.Equals(B, BodyTol);
	}

	bool ChannelInRange(const float Value)
	{
		return Value >= SolidBody::ParamMin && Value <= SolidBody::ParamMax;
	}

	bool CloseCm(const double Actual, const double Expected)
	{
		return FMath::Abs(Actual - Expected) <= 0.02;
	}

	float Station(const float GroundZ, const float HeadsDown)
	{
		const float CrownZ = GroundZ + SolidBody::ProportionSole * SolidBody::HeadLengthCm;
		return CrownZ - HeadsDown * SolidBody::HeadLengthCm;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodySeedTest,
	"SolidCore1.Body.Seed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodySeedTest::RunTest(const FString& Parameters)
{
	const FSolidBodyParams First = FSolidBodyParams::FromSeed(42);
	const FSolidBodyParams Second = FSolidBodyParams::FromSeed(42);
	TestTrue(TEXT("same seed matches byte for byte"), First.ExactEquals(Second));
	TestTrue(TEXT("different seeds differ"), !FSolidBodyParams::FromSeed(1).ExactEquals(FSolidBodyParams::FromSeed(2)));

	TestTrue(TEXT("height in range"), ChannelInRange(First.Height));
	TestTrue(TEXT("shoulders in range"), ChannelInRange(First.ShoulderWidth));
	TestTrue(TEXT("hips in range"), ChannelInRange(First.HipWidth));
	TestTrue(TEXT("torso in range"), ChannelInRange(First.TorsoGirth));
	TestTrue(TEXT("arms in range"), ChannelInRange(First.ArmLength));
	TestTrue(TEXT("legs in range"), ChannelInRange(First.LegLength));
	TestTrue(TEXT("head in range"), ChannelInRange(First.HeadSize));
	TestTrue(TEXT("neck in range"), ChannelInRange(First.NeckLength));
	TestTrue(TEXT("bulk in range"), ChannelInRange(First.Bulk));

	const FSolidBodyParams Identity;
	TestTrue(TEXT("default height"), FMath::IsNearlyEqual(Identity.Height, 1.f));
	TestTrue(TEXT("default is in range"), Identity.ExactEquals(Identity.Clamped()));

	FSolidBodyParams Wild;
	Wild.Height = 0.1f;
	Wild.Bulk = 4.f;
	const FSolidBodyParams Clamped = Wild.Clamped();
	TestTrue(TEXT("short height clamps up"), FMath::IsNearlyEqual(Clamped.Height, SolidBody::ParamMin));
	TestTrue(TEXT("bulk clamps down"), FMath::IsNearlyEqual(Clamped.Bulk, SolidBody::ParamMax));
	TestTrue(TEXT("untouched channel stays"), FMath::IsNearlyEqual(Clamped.ArmLength, 1.f));

	FSolidDeformMesh Mesh;
	Mesh.Bones.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	Mesh.Bones.Add(MakeBone(0, ESolidBodyBoneRole::Leg, FVector(0.f, 0.f, -40.f)));
	Mesh.Vertices.Add(MakeVertex(FVector(8.f, 0.f, -48.f), 1));
	const FSolidDeformMesh Once = SolidBodyDeform::Apply(Mesh, First);
	const FSolidDeformMesh Twice = SolidBodyDeform::Apply(Mesh, Second);
	TestTrue(TEXT("same seed rebuilds the same leg"), Once.Bones[1].LocalTranslation.Equals(Twice.Bones[1].LocalTranslation, 0.f));
	TestTrue(TEXT("same seed rebuilds the same vertex"), Once.Vertices[0].Position.Equals(Twice.Vertices[0].Position, 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyIdentityTest,
	"SolidCore1.Body.Identity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyIdentityTest::RunTest(const FString& Parameters)
{
	FSolidDeformMesh Mesh;
	Mesh.Bones.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	Mesh.Bones.Add(MakeBone(0, ESolidBodyBoneRole::Leg, FVector(0.f, 0.f, -40.f)));
	Mesh.Vertices.Add(MakeVertex(FVector(8.f, 0.f, -40.f), 1));

	const FSolidDeformMesh Out = SolidBodyDeform::Apply(Mesh, FSolidBodyParams());
	TestTrue(TEXT("root stays"), Near(Out.Bones[0].LocalTranslation, Mesh.Bones[0].LocalTranslation));
	TestTrue(TEXT("leg stays"), Near(Out.Bones[1].LocalTranslation, Mesh.Bones[1].LocalTranslation));
	TestTrue(TEXT("vertex stays"), Near(Out.Vertices[0].Position, Mesh.Vertices[0].Position));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyLegLengthTest,
	"SolidCore1.Body.LegLength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyLegLengthTest::RunTest(const FString& Parameters)
{
	FSolidDeformMesh Mesh;
	Mesh.Bones.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	Mesh.Bones.Add(MakeBone(0, ESolidBodyBoneRole::Leg, FVector(0.f, 0.f, -40.f)));
	Mesh.Vertices.Add(MakeVertex(FVector(0.f, 0.f, -40.f), 1));
	Mesh.Vertices.Add(MakeVertex(FVector(8.f, 0.f, -40.f), 1));
	Mesh.Vertices.Add(MakeVertex(FVector(3.f, 0.f, 0.f), 0));

	FSolidBodyParams Params;
	Params.LegLength = 1.125f;
	const FSolidDeformMesh Out = SolidBodyDeform::Apply(Mesh, Params);

	TestTrue(TEXT("root bone stays"), Near(Out.Bones[0].LocalTranslation, FVector::ZeroVector));
	TestTrue(TEXT("knee bone lengthens"), Near(Out.Bones[1].LocalTranslation, FVector(0.f, 0.f, -45.f)));

	const FVector BoneDelta = Out.Bones[1].LocalTranslation - Mesh.Bones[1].LocalTranslation;
	const FVector KneeDelta = Out.Vertices[0].Position - Mesh.Vertices[0].Position;
	TestTrue(TEXT("knee vertex moves with the child bone"), Near(KneeDelta, BoneDelta));

	TArray<FVector> Joints;
	SolidBodyDeform::ComponentPositions(Out.Bones, Joints);
	TestTrue(TEXT("knee stays on the child joint"), Near(Out.Vertices[0].Position, Joints[1]));
	TestTrue(TEXT("sideways offset is not girth"), Near(Out.Vertices[1].Position, FVector(8.f, 0.f, -45.f)));
	TestTrue(TEXT("root vertex stays"), Near(Out.Vertices[2].Position, Mesh.Vertices[2].Position));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyGirthTest,
	"SolidCore1.Body.Girth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyGirthTest::RunTest(const FString& Parameters)
{
	FSolidDeformMesh Mesh;
	Mesh.Bones.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	Mesh.Bones.Add(MakeBone(0, ESolidBodyBoneRole::Leg, FVector(0.f, 0.f, -40.f)));
	Mesh.Vertices.Add(MakeVertex(FVector(8.f, 0.f, -40.f), 1));
	Mesh.Vertices.Add(MakeVertex(FVector(0.f, 0.f, -50.f), 1));
	Mesh.Vertices.Add(MakeVertex(FVector(8.f, 0.f, -50.f), 1));

	FSolidBodyParams Params;
	Params.Bulk = 1.125f;
	const FSolidDeformMesh Out = SolidBodyDeform::Apply(Mesh, Params);

	TestTrue(TEXT("root bone stays"), Near(Out.Bones[0].LocalTranslation, Mesh.Bones[0].LocalTranslation));
	TestTrue(TEXT("leg bone stays"), Near(Out.Bones[1].LocalTranslation, Mesh.Bones[1].LocalTranslation));
	TestTrue(TEXT("sideways flesh thickens"), Near(Out.Vertices[0].Position, FVector(9.f, 0.f, -40.f)));
	TestTrue(TEXT("along the bone stays"), Near(Out.Vertices[1].Position, FVector(0.f, 0.f, -50.f)));
	TestTrue(TEXT("along stays while sideways thickens"), Near(Out.Vertices[2].Position, FVector(9.f, 0.f, -50.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyChannelsTest,
	"SolidCore1.Body.Channels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyChannelsTest::RunTest(const FString& Parameters)
{
	FSolidDeformMesh Mesh;
	Mesh.Bones.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	Mesh.Bones.Add(MakeBone(0, ESolidBodyBoneRole::Pelvis, FVector(0.f, 0.f, 2.f)));
	Mesh.Bones.Add(MakeBone(1, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 10.f)));
	Mesh.Bones.Add(MakeBone(2, ESolidBodyBoneRole::Neck, FVector(0.f, 0.f, 2.f)));
	Mesh.Bones.Add(MakeBone(3, ESolidBodyBoneRole::Head, FVector(0.f, 0.f, 3.f)));
	Mesh.Bones.Add(MakeBone(2, ESolidBodyBoneRole::Clavicle, FVector(8.f, 0.f, 0.f)));
	Mesh.Bones.Add(MakeBone(5, ESolidBodyBoneRole::Arm, FVector(6.f, 0.f, 0.f)));
	Mesh.Bones.Add(MakeBone(1, ESolidBodyBoneRole::Leg, FVector(10.f, 0.f, -20.f)));
	Mesh.Vertices.Add(MakeVertex(FVector(0.f, 2.f, 17.f), 4));
	Mesh.Vertices.Add(MakeVertex(FVector(4.f, 0.f, 12.f), 2));

	FSolidBodyParams Params;
	Params.Height = 1.125f;
	Params.ShoulderWidth = 1.125f;
	Params.HipWidth = 1.125f;
	Params.TorsoGirth = 1.125f;
	Params.ArmLength = 1.125f;
	Params.NeckLength = 1.125f;
	Params.HeadSize = 1.125f;
	const FSolidDeformMesh Out = SolidBodyDeform::Apply(Mesh, Params);

	TestTrue(TEXT("pelvis offset stays"), Near(Out.Bones[1].LocalTranslation, FVector(0.f, 0.f, 2.f)));
	TestTrue(TEXT("spine grows with height"), Near(Out.Bones[2].LocalTranslation, FVector(0.f, 0.f, 11.25f)));
	TestTrue(TEXT("neck lengthens"), Near(Out.Bones[3].LocalTranslation, FVector(0.f, 0.f, 2.25f)));
	TestTrue(TEXT("head joint offset stays"), Near(Out.Bones[4].LocalTranslation, FVector(0.f, 0.f, 3.f)));
	TestTrue(TEXT("shoulders widen"), Near(Out.Bones[5].LocalTranslation, FVector(9.f, 0.f, 0.f)));
	TestTrue(TEXT("arm lengthens"), Near(Out.Bones[6].LocalTranslation, FVector(6.75f, 0.f, 0.f)));
	TestTrue(
		TEXT("thigh takes hip width and height"),
		Near(Out.Bones[7].LocalTranslation, FVector(12.65625f, 0.f, -22.5f)));
	TestTrue(TEXT("head flesh scales from the joint"), Near(Out.Vertices[0].Position, FVector(0.f, 2.25f, 18.5f)));
	TestTrue(TEXT("torso flesh thickens with the spine"), Near(Out.Vertices[1].Position, FVector(4.5f, 0.f, 13.25f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyRotatedLegTest,
	"SolidCore1.Body.RotatedLeg",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyRotatedLegTest::RunTest(const FString& Parameters)
{
	FSolidDeformMesh Mesh;
	FSolidDeformBone Root = MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector(0.f, 0.f, 100.f));
	Root.LocalRotation = FRotator(0.f, 0.f, 90.f).Quaternion();
	Mesh.Bones.Add(Root);
	Mesh.Bones.Add(MakeBone(0, ESolidBodyBoneRole::Leg, FVector(0.f, 0.f, -40.f)));

	TArray<FVector> OldJoints;
	SolidBodyDeform::ComponentPositions(Mesh.Bones, OldJoints);
	Mesh.Vertices.Add(MakeVertex(OldJoints[1], 1));

	FSolidBodyParams Params;
	Params.LegLength = 1.125f;
	const FSolidDeformMesh Out = SolidBodyDeform::Apply(Mesh, Params);

	const FVector NewLocal = Out.Bones[1].LocalTranslation;
	TestTrue(TEXT("rotated leg keeps its length in local space"), FMath::IsNearlyEqual(NewLocal.Size(), 45.0, 0.01));
	TestTrue(
		TEXT("local translation stays on the bone axis"),
		FVector::CrossProduct(FVector(0.f, 0.f, -1.f), NewLocal.GetSafeNormal()).IsNearlyZero(0.01f));
	TestTrue(TEXT("leg still points the same way"), FVector::DotProduct(FVector(0.f, 0.f, -1.f), NewLocal) > 0.0);

	TArray<FVector> NewJoints;
	SolidBodyDeform::ComponentPositions(Out.Bones, NewJoints);
	TestTrue(TEXT("knee stays on the rotated joint"), Near(Out.Vertices[0].Position, NewJoints[1]));
	TestTrue(
		TEXT("knee delta matches the joint delta"),
		Near(Out.Vertices[0].Position - OldJoints[1], NewJoints[1] - OldJoints[1]));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyDeformClampTest,
	"SolidCore1.Body.DeformClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyDeformClampTest::RunTest(const FString& Parameters)
{
	FSolidDeformMesh Mesh;
	Mesh.Bones.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	Mesh.Bones.Add(MakeBone(0, ESolidBodyBoneRole::Leg, FVector(0.f, 0.f, -40.f)));
	Mesh.Vertices.Add(MakeVertex(FVector(8.f, 0.f, -40.f), 1));

	FSolidBodyParams Params;
	Params.Bulk = 4.f;
	const FSolidDeformMesh Out = SolidBodyDeform::Apply(Mesh, Params);
	const float ExpectedX = 8.f * SolidBody::ParamMax;
	TestTrue(TEXT("bulk past the range clamps"), FMath::IsNearlyEqual(Out.Vertices[0].Position.X, ExpectedX, BodyTol));
	TestTrue(TEXT("clamped girth keeps the joint height"), FMath::IsNearlyEqual(Out.Vertices[0].Position.Z, -40.0, BodyTol));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodyHeadLengthTest,
	"SolidCore1.Body.HeadLengths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodyHeadLengthTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("mid-thigh is halfway from the hip to the knee"),
		FMath::IsNearlyEqual(
			SolidBody::ProportionMidThigh,
			(SolidBody::ProportionPubic + SolidBody::ProportionKnee) * 0.5f));

	TArray<FSolidDeformBone> Empty;
	SolidBodyDeform::FitHeadLengths(Empty);
	TestEqual(TEXT("empty fit stays empty"), Empty.Num(), 0);

	TArray<FSolidDeformBone> NoHead;
	NoHead.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	NoHead.Add(MakeBone(0, ESolidBodyBoneRole::Leg, FVector(0.f, 0.f, -40.f)));
	NoHead.Add(MakeBone(1, ESolidBodyBoneRole::Foot, FVector(0.f, 0.f, -35.f)));
	NoHead.Add(MakeBone(2, ESolidBodyBoneRole::Foot, FVector(0.f, 0.f, -30.f)));
	const FVector ShinBefore = NoHead[3].LocalTranslation;
	SolidBodyDeform::FitHeadLengths(NoHead);
	TestTrue(TEXT("no head leaves the shin"), Near(NoHead[3].LocalTranslation, ShinBefore));

	TArray<FSolidDeformBone> NoAnkle;
	NoAnkle.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	NoAnkle.Add(MakeBone(0, ESolidBodyBoneRole::Neck, FVector(0.f, 0.f, 10.f)));
	NoAnkle.Add(MakeBone(1, ESolidBodyBoneRole::Head, FVector(4.f, 0.f, 12.f)));
	const FVector ChinBefore = NoAnkle[2].LocalTranslation;
	SolidBodyDeform::FitHeadLengths(NoAnkle);
	TestTrue(TEXT("no ankle leaves the chin"), Near(NoAnkle[2].LocalTranslation, ChinBefore));

	TArray<FSolidDeformBone> OneSpine;
	OneSpine.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	OneSpine.Add(MakeBone(0, ESolidBodyBoneRole::Pelvis, FVector(0.f, 0.f, 80.f)));
	OneSpine.Add(MakeBone(1, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 20.f)));
	OneSpine.Add(MakeBone(2, ESolidBodyBoneRole::Neck, FVector(0.f, 0.f, 10.f)));
	OneSpine.Add(MakeBone(3, ESolidBodyBoneRole::Head, FVector(2.f, 1.f, 12.f)));
	OneSpine.Add(MakeBone(1, ESolidBodyBoneRole::Leg, FVector(6.f, 0.f, -1.f)));
	OneSpine.Add(MakeBone(5, ESolidBodyBoneRole::Foot, FVector(0.f, 0.f, -40.f)));
	OneSpine.Add(MakeBone(6, ESolidBodyBoneRole::Foot, FVector(0.f, 0.f, -35.f)));
	TArray<FSolidDeformBone> Yawed = OneSpine;
	Yawed[0].LocalRotation = FRotator(0.f, 90.f, 0.f).Quaternion();
	const FQuat YawBefore = Yawed[0].LocalRotation;

	SolidBodyDeform::FitHeadLengths(OneSpine);
	TArray<FVector> OneJoints;
	SolidBodyDeform::ComponentPositions(OneSpine, OneJoints);
	const float OneGround = static_cast<float>(OneJoints[7].Z);
	TestTrue(TEXT("one spine is the chest"), CloseCm(OneJoints[2].Z, Station(OneGround, SolidBody::ProportionChest)));
	TestTrue(TEXT("one-spine neck is the shoulder line"), CloseCm(OneJoints[3].Z, Station(OneGround, SolidBody::ProportionNeck)));
	TestTrue(TEXT("one-spine chin is one head down"), CloseCm(OneJoints[4].Z, Station(OneGround, SolidBody::ProportionChin)));
	TestTrue(TEXT("chin sits over the neck"), CloseCm(OneJoints[4].X, OneJoints[3].X) && CloseCm(OneJoints[4].Y, OneJoints[3].Y));
	TestTrue(TEXT("one-spine hips are the pubic bone"), CloseCm(OneJoints[1].Z, Station(OneGround, SolidBody::ProportionPubic)));
	TestTrue(TEXT("one-spine knee is just below the knee line"), CloseCm(OneJoints[6].Z, Station(OneGround, SolidBody::ProportionKnee)));
	TestTrue(
		TEXT("mid-thigh is the hip-to-knee midpoint"),
		CloseCm((OneJoints[5].Z + OneJoints[6].Z) * 0.5, Station(OneGround, SolidBody::ProportionMidThigh)));

	TArray<FVector> YawBeforePos;
	SolidBodyDeform::ComponentPositions(Yawed, YawBeforePos);
	SolidBodyDeform::FitHeadLengths(Yawed);
	TArray<FVector> YawJoints;
	SolidBodyDeform::ComponentPositions(Yawed, YawJoints);
	const float YawGround = static_cast<float>(YawBeforePos[7].Z);
	TestTrue(TEXT("yawed chin keeps the head-length station"), CloseCm(YawJoints[4].Z, Station(YawGround, SolidBody::ProportionChin)));
	TestTrue(TEXT("yawed chest keeps its station"), CloseCm(YawJoints[2].Z, Station(YawGround, SolidBody::ProportionChest)));
	TestTrue(TEXT("yawed chin follows the neck"), CloseCm(YawJoints[4].X, YawJoints[3].X) && CloseCm(YawJoints[4].Y, YawJoints[3].Y));
	TestTrue(TEXT("yaw stays on the root"), Yawed[0].LocalRotation.Equals(YawBefore, 0.f));

	constexpr int32 Root = 0;
	constexpr int32 Pelvis = 1;
	constexpr int32 Waist = 2;
	constexpr int32 Mid = 3;
	constexpr int32 Chest = 4;
	constexpr int32 Neck = 5;
	constexpr int32 Head = 6;
	constexpr int32 HeadTop = 7;
	constexpr int32 Clavicle = 8;
	constexpr int32 Arm = 9;
	constexpr int32 Thigh = 10;
	constexpr int32 Knee = 11;
	constexpr int32 Twist = 12;
	constexpr int32 Ankle = 13;
	constexpr int32 Toe = 14;
	constexpr int32 OtherThigh = 15;
	constexpr int32 OtherKnee = 16;
	constexpr int32 OtherAnkle = 17;

	TArray<FSolidDeformBone> Bones;
	Bones.Add(MakeBone(INDEX_NONE, ESolidBodyBoneRole::Root, FVector::ZeroVector));
	Bones.Add(MakeBone(Root, ESolidBodyBoneRole::Pelvis, FVector(0.f, 0.f, 100.f)));
	Bones.Add(MakeBone(Pelvis, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 12.f)));
	Bones.Add(MakeBone(Waist, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 12.f)));
	Bones.Add(MakeBone(Mid, ESolidBodyBoneRole::Spine, FVector(0.f, 0.f, 12.f)));
	Bones.Add(MakeBone(Chest, ESolidBodyBoneRole::Neck, FVector(0.f, 0.f, 6.f)));
	Bones.Add(MakeBone(Neck, ESolidBodyBoneRole::Head, FVector(4.f, -3.f, 14.f)));
	Bones.Add(MakeBone(Head, ESolidBodyBoneRole::Head, FVector(0.f, 0.f, 10.f)));
	Bones.Add(MakeBone(Chest, ESolidBodyBoneRole::Clavicle, FVector(18.f, 0.f, 1.f)));
	Bones.Add(MakeBone(Clavicle, ESolidBodyBoneRole::Arm, FVector(22.f, 0.f, -4.f)));
	Bones.Add(MakeBone(Pelvis, ESolidBodyBoneRole::Leg, FVector(9.f, 1.f, -3.f)));
	Bones.Add(MakeBone(Thigh, ESolidBodyBoneRole::Foot, FVector(0.f, 2.f, -30.f)));
	FSolidDeformBone CalfTwist = MakeBone(Knee, ESolidBodyBoneRole::Foot, FVector(0.f, 0.f, -8.f));
	CalfTwist.bTwist = true;
	Bones.Add(CalfTwist);
	Bones.Add(MakeBone(Twist, ESolidBodyBoneRole::Foot, FVector(0.f, 0.f, -28.f)));
	FSolidDeformBone ToeBone = MakeBone(Ankle, ESolidBodyBoneRole::Foot, FVector(0.f, 14.f, -3.f));
	ToeBone.bDigit = true;
	Bones.Add(ToeBone);
	Bones.Add(MakeBone(Pelvis, ESolidBodyBoneRole::Leg, FVector(-9.f, 1.f, -3.f)));
	Bones.Add(MakeBone(OtherThigh, ESolidBodyBoneRole::Foot, FVector(0.f, -2.f, -20.f)));
	Bones.Add(MakeBone(OtherKnee, ESolidBodyBoneRole::Foot, FVector(0.f, 0.f, -20.f)));

	const TArray<FSolidDeformBone> Original = Bones;
	TArray<FVector> Before;
	SolidBodyDeform::ComponentPositions(Bones, Before);
	SolidBodyDeform::FitHeadLengths(Bones);
	TArray<FVector> Joints;
	SolidBodyDeform::ComponentPositions(Bones, Joints);

	const float GroundZ = static_cast<float>(Before[Ankle].Z);
	const float HeadCm = SolidBody::HeadLengthCm;
	TestTrue(TEXT("soles stay on the current ground"), CloseCm(Joints[Ankle].Z, GroundZ));
	TestTrue(TEXT("the high foot drops to that ground"), CloseCm(Joints[OtherAnkle].Z, GroundZ));
	TestTrue(TEXT("chin is one head above the neck"), CloseCm(Joints[Head].Z - Joints[Neck].Z, HeadCm));
	TestTrue(
		TEXT("chin to sole is the standing height"),
		CloseCm(Joints[Head].Z - Joints[Ankle].Z, (SolidBody::ProportionSole - SolidBody::ProportionChin) * HeadCm));
	TestTrue(TEXT("chin is one head down from the crown"), CloseCm(Joints[Head].Z, Station(GroundZ, SolidBody::ProportionChin)));
	TestTrue(TEXT("neck and shoulders share the shoulder line"), CloseCm(Joints[Neck].Z, Station(GroundZ, SolidBody::ProportionNeck)));
	TestTrue(TEXT("clavicle is on the shoulder line"), CloseCm(Joints[Clavicle].Z, Joints[Neck].Z));
	TestTrue(TEXT("chest is the nipple line"), CloseCm(Joints[Chest].Z, Station(GroundZ, SolidBody::ProportionChest)));
	TestTrue(TEXT("lowest spine is the navel"), CloseCm(Joints[Waist].Z, Station(GroundZ, SolidBody::ProportionWaist)));
	TestTrue(
		TEXT("middle spine splits navel and chest"),
		CloseCm(Joints[Mid].Z, Station(GroundZ, (SolidBody::ProportionWaist + SolidBody::ProportionChest) * 0.5f)));
	TestTrue(TEXT("pelvis is the pubic bone"), CloseCm(Joints[Pelvis].Z, Station(GroundZ, SolidBody::ProportionPubic)));
	TestTrue(TEXT("hip joint is the pubic bone"), CloseCm(Joints[Thigh].Z, Joints[Pelvis].Z));
	TestTrue(TEXT("other hip matches"), CloseCm(Joints[OtherThigh].Z, Joints[Pelvis].Z));
	TestTrue(TEXT("knee is just below the knee line"), CloseCm(Joints[Knee].Z, Station(GroundZ, SolidBody::ProportionKnee)));
	TestTrue(TEXT("other knee matches"), CloseCm(Joints[OtherKnee].Z, Joints[Knee].Z));
	TestTrue(
		TEXT("mid-thigh falls halfway down the thigh"),
		CloseCm((Joints[Thigh].Z + Joints[Knee].Z) * 0.5, Station(GroundZ, SolidBody::ProportionMidThigh)));
	TestTrue(TEXT("chin is centered on the neck"), CloseCm(Joints[Head].X, Joints[Neck].X) && CloseCm(Joints[Head].Y, Joints[Neck].Y));
	TestTrue(TEXT("skull helper stays above the chin"), CloseCm(Joints[HeadTop].Z, Joints[Head].Z + 10.0));
	TestTrue(
		TEXT("arm keeps its reach and rises with the shoulder"),
		CloseCm(Joints[Arm].X, Before[Arm].X)
			&& CloseCm(Joints[Arm].Y, Before[Arm].Y)
			&& CloseCm(Joints[Arm].Z, Before[Arm].Z + (Joints[Clavicle].Z - Before[Clavicle].Z)));
	TestTrue(
		TEXT("calf twist follows the knee"),
		CloseCm(Joints[Twist].Z, Before[Twist].Z + (Joints[Knee].Z - Before[Knee].Z)));
	TestTrue(TEXT("toes stay on the planted foot"), CloseCm(Joints[Toe].X, Before[Toe].X) && CloseCm(Joints[Toe].Y, Before[Toe].Y) && CloseCm(Joints[Toe].Z, Before[Toe].Z));
	TestTrue(TEXT("knee keeps its sideways place"), CloseCm(Joints[Knee].X, Before[Knee].X) && CloseCm(Joints[Knee].Y, Before[Knee].Y));

	bool bRotationsStayed = true;
	for (int32 Index = 0; bRotationsStayed && Index < Bones.Num(); ++Index)
	{
		bRotationsStayed = Bones[Index].LocalRotation.Equals(Original[Index].LocalRotation, 0.f);
	}
	TestTrue(TEXT("fit keeps every rotation"), bRotationsStayed);

	TArray<FSolidDeformBone> Again = Bones;
	SolidBodyDeform::FitHeadLengths(Again);
	TArray<FVector> Second;
	SolidBodyDeform::ComponentPositions(Again, Second);
	bool bStable = Joints.Num() == Second.Num();
	for (int32 Index = 0; bStable && Index < Joints.Num(); ++Index)
	{
		bStable = CloseCm(Second[Index].X, Joints[Index].X)
			&& CloseCm(Second[Index].Y, Joints[Index].Y)
			&& CloseCm(Second[Index].Z, Joints[Index].Z);
	}
	TestTrue(TEXT("a second fit leaves the stations"), bStable);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
