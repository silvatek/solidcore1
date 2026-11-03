#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Sites/SolidStrongholdSite.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidStrongholdSiteTestPrivate
{
	bool EdgeAtZ(const UStaticMesh* Mesh, const float Z, float& OutEdge)
	{
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return false;
		}

		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		float MinX = TNumericLimits<float>::Max();
		float MaxX = -MinX;
		float MinY = MinX;
		float MaxY = -MinX;
		bool bFound = false;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			if (!FMath::IsNearlyEqual(Position.Z, Z, 0.05f))
			{
				continue;
			}
			if (FMath::IsNearlyZero(Position.X) && FMath::IsNearlyZero(Position.Y))
			{
				continue;
			}
			bFound = true;
			MinX = FMath::Min(MinX, Position.X);
			MaxX = FMath::Max(MaxX, Position.X);
			MinY = FMath::Min(MinY, Position.Y);
			MaxY = FMath::Max(MaxY, Position.Y);
		}
		if (!bFound)
		{
			return false;
		}
		OutEdge = MaxX - MinX;
		return FMath::IsNearlyEqual(MaxY - MinY, OutEdge, 0.05f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdSitePyramidTest,
	"SolidCore1.Stronghold.SitePyramid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdSitePyramidTest::RunTest(const FString& Parameters)
{
	ASolidStrongholdSite* Site = NewObject<ASolidStrongholdSite>();
	UTEST_NOT_NULL(TEXT("site"), Site);
	Site->BuildVisuals();

	UStaticMeshComponent* Block = Site->GetBlockMesh();
	UTEST_NOT_NULL(TEXT("block"), Block);
	UStaticMesh* Mesh = Block->GetStaticMesh().Get();
	UTEST_NOT_NULL(TEXT("pyramid"), Mesh);

	const FVector Scale = Block->GetRelativeScale3D();
	TestTrue(TEXT("the pyramid is not scaled"), Scale.Equals(FVector::OneVector, 0.01f));
	TestTrue(
		TEXT("the foot sits on the actor origin"),
		Block->GetRelativeLocation().IsNearlyZero(0.05f));

	float FootEdge = 0.f;
	float TopEdge = 0.f;
	TestTrue(TEXT("the foot is a square"), SolidStrongholdSiteTestPrivate::EdgeAtZ(Mesh, 0.f, FootEdge));
	TestTrue(
		TEXT("the top is a square"),
		SolidStrongholdSiteTestPrivate::EdgeAtZ(Mesh, ASolidStrongholdSite::BlockCm, TopEdge));
	TestTrue(
		TEXT("the foot is the base edge"),
		FMath::IsNearlyEqual(FootEdge, ASolidStrongholdSite::BlockCm, 0.05f));
	TestTrue(
		TEXT("the top is 35% shorter"),
		FMath::IsNearlyEqual(TopEdge, ASolidStrongholdSite::BlockCm * ASolidStrongholdSite::TopScale, 0.05f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdSiteHoverTest,
	"SolidCore1.Stronghold.SiteHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdSiteHoverTest::RunTest(const FString& Parameters)
{
	ASolidStrongholdSite* Site = NewObject<ASolidStrongholdSite>();
	UTEST_NOT_NULL(TEXT("site"), Site);

	Site->SetSite(FIntPoint(16, 36), ESolidStrongholdSize::Imperious);
	TestEqual(
		TEXT("imperious site"),
		Site->Hover(),
		FString(TEXT("Build Imperious stronghold")));

	Site->SetSite(FIntPoint(1, 2), ESolidStrongholdSize::Meagre);
	TestEqual(
		TEXT("meagre site"),
		Site->Hover(),
		FString(TEXT("Build Meagre stronghold")));
	return true;
}

#endif
