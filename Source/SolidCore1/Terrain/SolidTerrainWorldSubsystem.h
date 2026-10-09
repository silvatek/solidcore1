#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SolidTerrainWorldSubsystem.generated.h"

/**
 * Always-on world subsystem so terrain streaming does not depend on Blueprint BeginPlay
 * calling the C++ parent (a common reason the streamer never spawns).
 */
UCLASS()
class SOLIDCORE1_API USolidTerrainWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override { return false; }

private:
	void TryEnsureStreamer(const TCHAR* Reason);

	bool bStreamerEnsured = false;
	float TimeSinceRetry = 0.f;
};
