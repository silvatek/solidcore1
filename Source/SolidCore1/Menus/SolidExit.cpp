#include "SolidExit.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_EDITOR
#include "Editor/UnrealEdEngine.h"
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
	if (Request.bCloseEditor && GUnrealEd)
	{
		GUnrealEd->CloseEditor();
	}
#endif
}
