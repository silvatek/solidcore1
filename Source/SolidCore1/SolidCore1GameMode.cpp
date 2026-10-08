#include "SolidCore1GameMode.h"
#include "SolidCore1Character.h"
#include "SolidCore1PlayerController.h"

ASolidCore1GameMode::ASolidCore1GameMode()
{
	DefaultPawnClass = ASolidCore1Character::StaticClass();
	PlayerControllerClass = ASolidCore1PlayerController::StaticClass();
}
