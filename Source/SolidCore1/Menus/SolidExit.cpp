#include "SolidExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#endif

bool SolidExit::IsEditorPlay(const UWorld* World)
{
	return World && World->WorldType == EWorldType::PIE;
}

bool SolidExit::CanCloseEditor()
{
#if WITH_EDITOR
	return GUnrealEd != nullptr;
#else
	return false;
#endif
}

void SolidExit::Perform(const UObject* WorldContext, APlayerController* Player)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const FRequest Request = RequestForYes(IsEditorPlay(World), CanCloseEditor());
	if (!Request.bQuitGame)
	{
		return;
	}

	UKismetSystemLibrary::QuitGame(WorldContext, Player, EQuitPreference::Quit, false);

#if WITH_EDITOR
	if (Request.bCloseEditor && GEngine)
	{
		// Yes is handled from Tick, and CloseEditor waits on that same queue.
		// QUIT_EDITOR is the editor's own deferred close, so it runs after this stack returns.
		GEngine->DeferredCommands.Add(TEXT("QUIT_EDITOR"));
	}
#endif
}
