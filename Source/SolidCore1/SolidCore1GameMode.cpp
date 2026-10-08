#include "SolidCore1GameMode.h"
#include "SolidCore1Character.h"
#include "SolidCore1PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ASolidCore1GameMode::ASolidCore1GameMode()
{
	// Prefer an editor Blueprint pawn if present (mesh/anim assigned in Content).
	static ConstructorHelpers::FClassFinder<APawn> BpPawn(TEXT("/Game/Characters/BP_SolidCore1Character"));
	if (BpPawn.Succeeded())
	{
		DefaultPawnClass = BpPawn.Class;
	}
	else
	{
		static ConstructorHelpers::FClassFinder<APawn> TpPawn(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
		if (TpPawn.Succeeded())
		{
			DefaultPawnClass = TpPawn.Class;
		}
		else
		{
			DefaultPawnClass = ASolidCore1Character::StaticClass();
		}
	}

	PlayerControllerClass = ASolidCore1PlayerController::StaticClass();
}
