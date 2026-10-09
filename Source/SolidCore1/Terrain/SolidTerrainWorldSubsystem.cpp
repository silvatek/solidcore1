#include "SolidTerrainWorldSubsystem.h"
#include "SolidTerrainStreamer.h"
#include "SolidCore1.h"
#include "Engine/World.h"

void USolidTerrainWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bStreamerEnsured = false;
	TimeSinceRetry = 0.f;
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] TerrainWorldSubsystem Initialize"));
	UE_LOG(LogSolid, Warning, TEXT("TerrainWorldSubsystem Initialize"));
}

void USolidTerrainWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] TerrainWorldSubsystem OnWorldBeginPlay (%s)"), *InWorld.GetName());
	TryEnsureStreamer(TEXT("OnWorldBeginPlay"));
}

void USolidTerrainWorldSubsystem::Tick(float DeltaTime)
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

TStatId USolidTerrainWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USolidTerrainWorldSubsystem, STATGROUP_Tickables);
}

bool USolidTerrainWorldSubsystem::IsTickable() const
{
	return !bStreamerEnsured;
}

void USolidTerrainWorldSubsystem::TryEnsureStreamer(const TCHAR* Reason)
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

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::EnsureExists(World);
	if (Streamer)
	{
		bStreamerEnsured = true;
		UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Terrain streamer ready via %s"), Reason);
		UE_LOG(LogSolid, Warning, TEXT("Terrain streamer ready via %s"), Reason);
	}
}
