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

#endif // WITH_DEV_AUTOMATION_TESTS
