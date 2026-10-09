#include "SolidCharacter.h"
#include "SolidCore1.h"
#include "Party/SolidPartyDrill.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

void ASolidCharacter::StartPartyFormationDrill()
{
	if (PartyFormationDrill.bActive)
	{
		UE_LOG(LogSolid, Warning, TEXT("Formation drill restarted."));
	}

	PartyFormationDrill.bActive = true;
	PartyFormationDrill.CurrentLeg = 0;
	BeginPartyFormationDrillLeg();
}

void ASolidCharacter::StopPartyFormationDrill()
{
	PartyFormationDrill.bActive = false;
	PartyFormationDrill.CurrentLeg = 0;
	PartyFormationDrill.WalkSecondsRemaining = 0.f;
	ApplyWalkSpeed();
}

bool ASolidCharacter::IsPartyFormationDrillActive() const
{
	return PartyFormationDrill.bActive;
}

void ASolidCharacter::BeginPartyFormationDrillLeg()
{
	const int32 Leg = PartyFormationDrill.CurrentLeg;
	if (Leg < 0 || Leg >= SolidPartyDrill::NumLegs)
	{
		StopPartyFormationDrill();
		return;
	}

	const int32 PlanSlot = SolidPartyDrill::PlanSlotForLeg(Leg);
	SelectBattlePlanSlot(PlanSlot);

	if (SolidPartyDrill::TurnsBeforeWalk(Leg))
	{
		const float NewYaw = GetActorRotation().Yaw + SolidPartyDrill::TurnYawDegrees;
		SetActorRotation(FRotator(0.f, NewYaw, 0.f));
		if (AController* C = GetController())
		{
			FRotator ControlRot = C->GetControlRotation();
			ControlRot.Yaw = NewYaw;
			C->SetControlRotation(ControlRot);
		}
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = WalkSpeed;
	}
	bIsSprinting = false;

	PartyFormationDrill.WalkSecondsRemaining = SolidPartyDrill::LegDurationSeconds;
	UE_LOG(LogSolid, Warning,
		TEXT("Formation drill leg %d/%d → F%d%s, walk %.1fs"),
		Leg + 1,
		SolidPartyDrill::NumLegs,
		PlanSlot + 1,
		SolidPartyDrill::TurnsBeforeWalk(Leg) ? TEXT(" (+90°)") : TEXT(""),
		SolidPartyDrill::LegDurationSeconds);
}

void ASolidCharacter::TickPartyFormationDrill(const float DeltaTime)
{
	if (!PartyFormationDrill.bActive)
	{
		return;
	}

	AddMovementInput(GetActorForwardVector(), 1.f);

	PartyFormationDrill.WalkSecondsRemaining -= DeltaTime;
	if (PartyFormationDrill.WalkSecondsRemaining > 0.f)
	{
		return;
	}

	++PartyFormationDrill.CurrentLeg;
	if (PartyFormationDrill.CurrentLeg >= SolidPartyDrill::NumLegs)
	{
		UE_LOG(LogSolid, Warning, TEXT("Formation drill complete."));
		StopPartyFormationDrill();
		return;
	}

	BeginPartyFormationDrillLeg();
}
