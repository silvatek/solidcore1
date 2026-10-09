#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStreamerNullWorldTest,
	"SolidCore1.Streamer.NullWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStreamerNullWorldTest::RunTest(const FString& Parameters)
{
	TestNull(TEXT("FindExisting null world"), ASolidTerrainStreamer::FindExisting(nullptr));
	TestNull(TEXT("EnsureExists null world"), ASolidTerrainStreamer::EnsureExists(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStreamerEnsureExistsIdempotentTest,
	"SolidCore1.Streamer.EnsureExistsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStreamerEnsureExistsIdempotentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("SolidStreamerTestWorld"));
	TestNotNull(TEXT("created world"), World);
	if (!World)
	{
		return false;
	}

	TestNull(TEXT("no streamer before EnsureExists"), ASolidTerrainStreamer::FindExisting(World));

	ASolidTerrainStreamer* First = ASolidTerrainStreamer::EnsureExists(World);
	TestNotNull(TEXT("EnsureExists spawns"), First);
	TestEqual(TEXT("FindExisting returns spawned"), ASolidTerrainStreamer::FindExisting(World), First);

	ASolidTerrainStreamer* Second = ASolidTerrainStreamer::EnsureExists(World);
	TestEqual(TEXT("second EnsureExists is idempotent"), Second, First);

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStreamerStartTownRelocateFlagTest,
	"SolidCore1.Streamer.StartTownRelocateFlag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStreamerStartTownRelocateFlagTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("SolidStreamerRelocateFlagWorld"));
	TestNotNull(TEXT("created world"), World);
	if (!World)
	{
		return false;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::EnsureExists(World);
	TestNotNull(TEXT("streamer"), Streamer);
	// No player pawn → relocate waits; flag stays false until a focus exists (or no Z town).
	TestFalse(TEXT("relocate not attempted without focus"), Streamer->HasAttemptedStartTownRelocate());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Focus = World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FVector(100.f, 200.f, 300.f), FRotator::ZeroRotator, SpawnParams);
	TestNotNull(TEXT("focus pawn"), Focus);
	Streamer->FocusActor = Focus;

	// Relocate only — do not Tick/UpdateStreaming (chunk mesh builds spam Error under NullRHI).
	Streamer->EnsureStartTownRelocate();

	TestTrue(TEXT("relocate attempted once focus exists"), Streamer->HasAttemptedStartTownRelocate());

	if (Streamer->GetTerrainMap() && Streamer->GetTerrainMap()->IsBuilt())
	{
		FVector2D TownXY = FVector2D::ZeroVector;
		if (Streamer->GetTerrainMap()->GetStartTownWorldXY(TownXY))
		{
			const FVector2D ExpectedXY = TownXY + Streamer->StartTownPawnOffsetXY;
			const FVector Loc = Focus->GetActorLocation();
			TestTrue(TEXT("focus X near offset spawn"), FMath::IsNearlyEqual(Loc.X, ExpectedXY.X, 50.f));
			TestTrue(TEXT("focus Y near offset spawn"), FMath::IsNearlyEqual(Loc.Y, ExpectedXY.Y, 50.f));
			TestTrue(TEXT("focus cleared of monolith centroid"),
				FVector2D::Distance(FVector2D(Loc.X, Loc.Y), TownXY) > 200.f);
		}
	}

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStreamerRelocateCompanionsWithFocusTest,
	"SolidCore1.Streamer.RelocateCompanionsWithFocus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStreamerRelocateCompanionsWithFocusTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("SolidStreamerPartyRelocateWorld"));
	TestNotNull(TEXT("created world"), World);
	if (!World)
	{
		return false;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::EnsureExists(World);
	TestNotNull(TEXT("streamer"), Streamer);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector StartLoc(500.f, 500.f, 200.f);
	ACharacter* Focus = World->SpawnActor<ACharacter>(
		ACharacter::StaticClass(), StartLoc, FRotator::ZeroRotator, SpawnParams);
	TestNotNull(TEXT("focus"), Focus);
	Streamer->FocusActor = Focus;

	// Deferred spawn skips BeginPlay mesh/anim (avoids Error logs under NullRHI).
	const FTransform SamXform(FRotator::ZeroRotator, StartLoc + FVector(-200.f, 80.f, 0.f));
	ASolidCompanionCharacter* Sam = World->SpawnActorDeferred<ASolidCompanionCharacter>(
		ASolidCompanionCharacter::StaticClass(),
		SamXform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestNotNull(TEXT("sam"), Sam);
	if (!Sam)
	{
		World->DestroyWorld(false);
		return false;
	}
	Sam->SetFollowTarget(Focus);
	const FVector SamBefore = Sam->GetActorLocation();

	Streamer->EnsureStartTownRelocate();

	TestTrue(TEXT("relocate attempted"), Streamer->HasAttemptedStartTownRelocate());

	FVector2D TownXY = FVector2D::ZeroVector;
	if (Streamer->GetTerrainMap()
		&& Streamer->GetTerrainMap()->GetStartTownWorldXY(TownXY))
	{
		const FVector FocusLoc = Focus->GetActorLocation();
		const FVector SamAfter = Sam->GetActorLocation();
		const FVector ExpectedDelta(FocusLoc.X - StartLoc.X, FocusLoc.Y - StartLoc.Y, 0.f);
		TestTrue(TEXT("sam moved with focus X"),
			FMath::IsNearlyEqual(SamAfter.X, SamBefore.X + ExpectedDelta.X, 50.f));
		TestTrue(TEXT("sam moved with focus Y"),
			FMath::IsNearlyEqual(SamAfter.Y, SamBefore.Y + ExpectedDelta.Y, 50.f));
		TestTrue(TEXT("sam near captain after relocate"),
			FVector::Dist2D(SamAfter, FocusLoc) < 1000.f);
	}
	else
	{
		AddError(TEXT("expected WorldMap start town for companion relocate test"));
	}

	World->DestroyWorld(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
