#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
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
	FSolidNpcRetargetTest,
	"SolidCore1.Game.NpcRetarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNpcRetargetTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/MH1/exports1.exports1"));
	UAnimSequence* Idle = LoadObject<UAnimSequence>(
		nullptr,
		TEXT("/Game/Viking/Animations/Anim_Viking_idle1.Anim_Viking_idle1"));
	UTEST_NOT_NULL(TEXT("exports1"), Mesh);
	UTEST_NOT_NULL(TEXT("the viking idle"), Idle);

	USkeleton* Target = Mesh->GetSkeleton();
	const USkeleton* Source = Idle->GetSkeleton();
	UTEST_NOT_NULL(TEXT("the figure skeleton"), Target);
	UTEST_NOT_NULL(TEXT("the idle skeleton"), Source);

	ASolidNpc::RetargetFrom(Target, Source);

	TestEqual(
		TEXT("the root keeps the figure's length"),
		Target->GetBoneTranslationRetargetingMode(0, false),
		EBoneTranslationRetargetingMode::Skeleton);
	if (Target->GetReferenceSkeleton().GetRawBoneNum() > 1)
	{
		TestEqual(
			TEXT("a later bone keeps the figure's length"),
			Target->GetBoneTranslationRetargetingMode(1, false),
			EBoneTranslationRetargetingMode::Skeleton);
	}

	bool bListed = false;
	for (const TSoftObjectPtr<USkeleton>& Listed : Target->GetCompatibleSkeletons())
	{
		if (Listed.Get() == Source)
		{
			bListed = true;
		}
	}
	TestTrue(TEXT("the viking skeleton is compatible"), bListed);
	return true;
}

#endif
