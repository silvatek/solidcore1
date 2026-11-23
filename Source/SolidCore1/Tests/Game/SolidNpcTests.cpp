#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Content/SolidContentLookup.h"
#include "Engine/SkeletalMesh.h"
#include "Game/SolidNpc.h"
#if WITH_EDITOR
#include "RetargetEditor/IKRetargeterController.h"
#include "Retargeter/IKRetargeter.h"
#include "Retargeter/RetargetOps/FKChainsOp.h"
#include "Retargeter/RetargetOps/PelvisMotionOp.h"
#include "Rig/IKRigDefinition.h"
#include "RigEditor/IKRigController.h"
#endif

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
		TEXT("the spine starts at spine_01"),
		FCString::Strcmp(ASolidNpc::RetargetChain(0).StartBone, TEXT("spine_01")) == 0);
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

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNpcRetargetRigTest,
	"SolidCore1.Game.NpcRetargetRig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNpcRetargetRigTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* Figure = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/MH1/exports1.exports1"));
	USkeletalMesh* Viking = SolidContentLookup::FindVikingMesh();
	UTEST_NOT_NULL(TEXT("exports1"), Figure);
	UTEST_NOT_NULL(TEXT("the viking mesh"), Viking);

	const USkeletalMesh* Meshes[] = { Figure, Viking };
	const TCHAR* Names[] = { TEXT("figure"), TEXT("viking") };
	for (int32 MeshIndex = 0; MeshIndex < 2; ++MeshIndex)
	{
		UIKRigDefinition* Rig = ASolidNpc::MakeRetargetRig(const_cast<USkeletalMesh*>(Meshes[MeshIndex]));
		UTEST_NOT_NULL(*FString::Printf(TEXT("%s rig"), Names[MeshIndex]), Rig);
		UIKRigController* Controller = UIKRigController::GetController(Rig);
		UTEST_NOT_NULL(*FString::Printf(TEXT("%s controller"), Names[MeshIndex]), Controller);
		TestEqual(
			*FString::Printf(TEXT("%s pelvis"), Names[MeshIndex]),
			Controller->GetRetargetRoot(),
			FName(TEXT("pelvis")));
		TestEqual(
			*FString::Printf(TEXT("%s chain count"), Names[MeshIndex]),
			Controller->GetRetargetChains().Num(),
			ASolidNpc::RetargetChainCount);
		for (int32 Index = 0; Index < ASolidNpc::RetargetChainCount; ++Index)
		{
			const FSolidNpcRetargetChain& Chain = ASolidNpc::RetargetChain(Index);
			const FName ChainName(Chain.Name);
			TestEqual(
				*FString::Printf(TEXT("%s %s start"), Names[MeshIndex], Chain.Name),
				Controller->GetRetargetChainStartBone(ChainName),
				FName(Chain.StartBone));
			TestEqual(
				*FString::Printf(TEXT("%s %s end"), Names[MeshIndex], Chain.Name),
				Controller->GetRetargetChainEndBone(ChainName),
				FName(Chain.EndBone));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNpcIdleRetargetTest,
	"SolidCore1.Game.NpcIdleRetarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNpcIdleRetargetTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* Figure = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/MH1/exports1.exports1"));
	USkeletalMesh* Viking = SolidContentLookup::FindVikingMesh();
	UTEST_NOT_NULL(TEXT("exports1"), Figure);
	UTEST_NOT_NULL(TEXT("the viking mesh"), Viking);

	UIKRetargeter* Retargeter = ASolidNpc::MakeIdleRetargeter(Viking, Figure);
	UTEST_NOT_NULL(TEXT("idle retargeter"), Retargeter);
	UIKRetargeterController* Controller = UIKRetargeterController::GetController(Retargeter);
	UTEST_NOT_NULL(TEXT("retarget controller"), Controller);

	bool bFoundPelvis = false;
	bool bFoundChains = false;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		if (Cast<UIKRetargetPelvisMotionController>(Controller->GetOpController(Index)))
		{
			FIKRetargetOpBase* Op = Controller->GetRetargetOpByIndex(Index);
			const FIKRetargetPelvisMotionOpSettings* Settings = Op
				? static_cast<const FIKRetargetPelvisMotionOpSettings*>(Op->GetSettings())
				: nullptr;
			UTEST_NOT_NULL(TEXT("pelvis settings"), Settings);
			TestTrue(
				TEXT("pelvis translation stays at the standing pose"),
				FMath::IsNearlyZero(Settings->TranslationAlpha));
			TestEqual(TEXT("source pelvis"), Settings->SourcePelvisBone.BoneName, FName(TEXT("pelvis")));
			TestEqual(TEXT("target pelvis"), Settings->TargetPelvisBone.BoneName, FName(TEXT("pelvis")));
			bFoundPelvis = true;
		}
		if (UIKRetargetFKChainsController* Chains =
			Cast<UIKRetargetFKChainsController>(Controller->GetOpController(Index)))
		{
			const FIKRetargetFKChainsOpSettings Settings = Chains->GetSettings();
			TestEqual(TEXT("six chains"), Settings.ChainsToRetarget.Num(), ASolidNpc::RetargetChainCount);
			for (const FRetargetFKChainSettings& Chain : Settings.ChainsToRetarget)
			{
				TestEqual(
					*FString::Printf(TEXT("%s match scaled chain"), *Chain.TargetChainName.ToString()),
					static_cast<uint8>(Chain.RotationMode),
					static_cast<uint8>(EFKChainRotationMode::MatchScaledChain));
				TestEqual(
					*FString::Printf(TEXT("%s translation off"), *Chain.TargetChainName.ToString()),
					static_cast<uint8>(Chain.TranslationMode),
					static_cast<uint8>(EFKChainTranslationMode::None));
			}
			bFoundChains = true;
		}
	}
	TestTrue(TEXT("pelvis op"), bFoundPelvis);
	TestTrue(TEXT("chain op"), bFoundChains);
	return true;
}
#endif

#endif
