#include "Terrain/SolidCore1TerrainWorldSubsystem.h"
#include "Terrain/SolidCore1TerrainStreamer.h"
#include "SolidCore1.h"
#include "Engine/World.h"

void USolidCore1TerrainWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bStreamerEnsured = false;
	TimeSinceRetry = 0.f;
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] TerrainWorldSubsystem Initialize"));
	UE_LOG(LogSolidCore1, Warning, TEXT("TerrainWorldSubsystem Initialize"));
}

void USolidCore1TerrainWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] TerrainWorldSubsystem OnWorldBeginPlay (%s)"), *InWorld.GetName());
	TryEnsureStreamer(TEXT("OnWorldBeginPlay"));
}

void USolidCore1TerrainWorldSubsystem::Tick(float DeltaTime)
{
	if (bStreamerEnsured)
	{
		return;
	}

	TimeSinceRetry += DeltaTime;
	if (TimeSinceRetry < 0.5f)
	{
		return;
	}

	TimeSinceRetry = 0.f;
	TryEnsureStreamer(TEXT("TickRetry"));
}

TStatId USolidCore1TerrainWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USolidCore1TerrainWorldSubsystem, STATGROUP_Tickables);
}

bool USolidCore1TerrainWorldSubsystem::IsTickable() const
{
	return !bStreamerEnsured;
}

void USolidCore1TerrainWorldSubsystem::TryEnsureStreamer(const TCHAR* Reason)
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}

	// Only for game worlds (PIE / game), not editor preview worlds without play.
	if (World->WorldType != EWorldType::PIE && World->WorldType != EWorldType::Game)
	{
		return;
	}

	ASolidCore1TerrainStreamer* Streamer = ASolidCore1TerrainStreamer::EnsureExists(World);
	if (Streamer)
	{
		bStreamerEnsured = true;
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Terrain streamer ready via %s"), Reason);
		UE_LOG(LogSolidCore1, Warning, TEXT("Terrain streamer ready via %s"), Reason);
	}
}
