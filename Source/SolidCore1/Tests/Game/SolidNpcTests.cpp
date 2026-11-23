#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Content/SolidContentLookup.h"
#include "Engine/SkeletalMesh.h"
#include "Game/SolidNpc.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNpcPlaceTest,
	"SolidCore1.Game.NpcPlace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNpcPlaceTest::RunTest(const FString& Parameters)
{
	const FVector2D Mark(25200.f, 28000.f);
	const FVector2D Place = ASolidNpc::Place(Mark);
	TestTrue(TEXT("the stand keeps the mark's east and west"), FMath::IsNearlyEqual(Place.X, Mark.X));
	TestTrue(
		TEXT("ten metres south"),
		FMath::IsNearlyEqual(Place.Y, Mark.Y - ASolidNpc::SouthOfDragongateCm));
	TestTrue(TEXT("south is one thousand centimetres"), FMath::IsNearlyEqual(ASolidNpc::SouthOfDragongateCm, 1000.f));
	TestTrue(
		TEXT("the face turns north"),
		FMath::IsNearlyEqual(ASolidNpc::FacesCaptainYawDeg, 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNpcRetargetChainsTest,
	"SolidCore1.Game.NpcRetargetChains",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNpcRetargetChainsTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* Figure = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/MH1/exports1.exports1"));
	USkeletalMesh* Viking = SolidContentLookup::FindVikingMesh();
	UTEST_NOT_NULL(TEXT("exports1"), Figure);
	UTEST_NOT_NULL(TEXT("the viking mesh"), Viking);

	TestEqual(TEXT("six chains"), ASolidNpc::RetargetChainCount, 6);
	TestTrue(
		TEXT("the spine starts at the pelvis"),
		FCString::Strcmp(ASolidNpc::RetargetChain(0).StartBone, TEXT("pelvis")) == 0);
	TestTrue(
		TEXT("the head ends at the head"),
		FCString::Strcmp(ASolidNpc::RetargetChain(5).EndBone, TEXT("head")) == 0);

	const USkeletalMesh* Meshes[] = { Figure, Viking };
	const TCHAR* Names[] = { TEXT("figure"), TEXT("viking") };
	for (int32 MeshIndex = 0; MeshIndex < 2; ++MeshIndex)
	{
		const FReferenceSkeleton& Bones = Meshes[MeshIndex]->GetRefSkeleton();
		for (int32 Index = 0; Index < ASolidNpc::RetargetChainCount; ++Index)
		{
			const FSolidNpcRetargetChain& Chain = ASolidNpc::RetargetChain(Index);
			TestTrue(
				*FString::Printf(TEXT("%s %s starts at %s"), Names[MeshIndex], Chain.Name, Chain.StartBone),
				Bones.FindBoneIndex(Chain.StartBone) != INDEX_NONE);
			TestTrue(
				*FString::Printf(TEXT("%s %s ends at %s"), Names[MeshIndex], Chain.Name, Chain.EndBone),
				Bones.FindBoneIndex(Chain.EndBone) != INDEX_NONE);
		}
	}
	return true;
}

#endif
