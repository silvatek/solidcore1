#include "SolidCharacter.h"
#include "SolidCore1.h"
#include "Squad/SolidSquadDrill.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

void ASolidCharacter::StartSquadFormationDrill()
{
	if (SquadFormationDrill.bActive)
	{
		UE_LOG(LogSolid, Warning, TEXT("Formation drill restarted."));
	}

	SquadFormationDrill.bActive = true;
	SquadFormationDrill.CurrentLeg = 0;
	SquadFormationDrill.Phase = SolidSquadDrill::EPhase::Walking;
	SquadFormationDrill.PhaseSecondsRemaining = 0.f;
	BeginSquadFormationDrillLeg();
}

void ASolidCharacter::StopSquadFormationDrill()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = SquadFormationDrill.bSavedOrientRotationToMovement;
	}

	SquadFormationDrill.bActive = false;
	SquadFormationDrill.CurrentLeg = 0;
	SquadFormationDrill.Phase = SolidSquadDrill::EPhase::Walking;
	SquadFormationDrill.PhaseSecondsRemaining = 0.f;
	ApplyWalkSpeed();
}

bool ASolidCharacter::IsSquadFormationDrillActive() const
{
	return SquadFormationDrill.bActive;
}

bool ASolidCharacter::IsSquadFormationDrillTurning() const
{
	return SquadFormationDrill.bActive
		&& SquadFormationDrill.Phase == SolidSquadDrill::EPhase::Turning;
}

void ASolidCharacter::BeginSquadFormationDrillWalk()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = SquadFormationDrill.bSavedOrientRotationToMovement;
		Move->MaxWalkSpeed = WalkSpeed;
	}
	bIsSprinting = false;

	SquadFormationDrill.Phase = SolidSquadDrill::EPhase::Walking;
	SquadFormationDrill.PhaseSecondsRemaining = SolidSquadDrill::LegDurationSeconds;
}

void ASolidCharacter::BeginSquadFormationDrillLeg()
{
	const int32 Leg = SquadFormationDrill.CurrentLeg;
	if (Leg < 0 || Leg >= SolidSquadDrill::NumLegs)
	{
		StopSquadFormationDrill();
		return;
	}

	const int32 PlanSlot = SolidSquadDrill::PlanSlotForLeg(Leg);
	SelectBattlePlanSlot(PlanSlot);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		SquadFormationDrill.bSavedOrientRotationToMovement = Move->bOrientRotationToMovement;
		Move->MaxWalkSpeed = WalkSpeed;
	}
	bIsSprinting = false;

	if (SolidSquadDrill::TurnsBeforeWalk(Leg))
	{
		const float StartYaw = GetActorRotation().Yaw;
		SquadFormationDrill.TurnStartYaw = StartYaw;
		SquadFormationDrill.TurnTargetYaw = StartYaw + SolidSquadDrill::TurnYawDegrees;
		SquadFormationDrill.Phase = SolidSquadDrill::EPhase::Turning;
		SquadFormationDrill.PhaseSecondsRemaining = SolidSquadDrill::TurnDurationSeconds;

		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->bOrientRotationToMovement = false;
			Move->StopMovementImmediately();
		}

		UE_LOG(LogSolid, Warning,
			TEXT("Formation drill leg %d/%d → F%d turn +%.0f° over %.2fs"),
			Leg + 1,
			SolidSquadDrill::NumLegs,
			PlanSlot + 1,
			SolidSquadDrill::TurnYawDegrees,
			SolidSquadDrill::TurnDurationSeconds);
		return;
	}

	BeginSquadFormationDrillWalk();
	UE_LOG(LogSolid, Warning,
		TEXT("Formation drill leg %d/%d → F%d walk %.1fs"),
		Leg + 1,
		SolidSquadDrill::NumLegs,
		PlanSlot + 1,
		SolidSquadDrill::LegDurationSeconds);
}

void ASolidCharacter::ApplySquadFormationDrillYaw(const float YawDegrees)
{
	SetActorRotation(FRotator(0.f, YawDegrees, 0.f));
	if (AController* C = GetController())
	{
		FRotator ControlRot = C->GetControlRotation();
		ControlRot.Yaw = YawDegrees;
		C->SetControlRotation(ControlRot);
	}
}

void ASolidCharacter::TickSquadFormationDrill(const float DeltaTime)
{
	if (!SquadFormationDrill.bActive)
	{
		return;
	}

	if (SquadFormationDrill.Phase == SolidSquadDrill::EPhase::Turning)
	{
		const float Duration = FMath::Max(SolidSquadDrill::TurnDurationSeconds, KINDA_SMALL_NUMBER);
		SquadFormationDrill.PhaseSecondsRemaining -= DeltaTime;
		const float Alpha = FMath::Clamp(
			1.f - (SquadFormationDrill.PhaseSecondsRemaining / Duration),
			0.f,
			1.f);
		const float Yaw = FMath::Lerp(
			SquadFormationDrill.TurnStartYaw,
			SquadFormationDrill.TurnTargetYaw,
			Alpha);
		ApplySquadFormationDrillYaw(Yaw);

		if (SquadFormationDrill.PhaseSecondsRemaining > 0.f)
		{
			return;
		}

		ApplySquadFormationDrillYaw(SquadFormationDrill.TurnTargetYaw);
		BeginSquadFormationDrillWalk();
		UE_LOG(LogSolid, Warning,
			TEXT("Formation drill leg %d/%d walk %.1fs"),
			SquadFormationDrill.CurrentLeg + 1,
			SolidSquadDrill::NumLegs,
			SolidSquadDrill::LegDurationSeconds);
		return;
	}

	AddMovementInput(GetActorForwardVector(), 1.f);

	SquadFormationDrill.PhaseSecondsRemaining -= DeltaTime;
	if (SquadFormationDrill.PhaseSecondsRemaining > 0.f)
	{
		return;
	}

	++SquadFormationDrill.CurrentLeg;
	if (SquadFormationDrill.CurrentLeg >= SolidSquadDrill::NumLegs)
	{
		UE_LOG(LogSolid, Warning, TEXT("Formation drill complete."));
		StopSquadFormationDrill();
		return;
	}

	BeginSquadFormationDrillLeg();
}
