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
	PartyFormationDrill.Phase = SolidPartyDrill::EPhase::Walking;
	PartyFormationDrill.PhaseSecondsRemaining = 0.f;
	BeginPartyFormationDrillLeg();
}

void ASolidCharacter::StopPartyFormationDrill()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = PartyFormationDrill.bSavedOrientRotationToMovement;
	}

	PartyFormationDrill.bActive = false;
	PartyFormationDrill.CurrentLeg = 0;
	PartyFormationDrill.Phase = SolidPartyDrill::EPhase::Walking;
	PartyFormationDrill.PhaseSecondsRemaining = 0.f;
	ApplyWalkSpeed();
}

bool ASolidCharacter::IsPartyFormationDrillActive() const
{
	return PartyFormationDrill.bActive;
}

bool ASolidCharacter::IsPartyFormationDrillTurning() const
{
	return PartyFormationDrill.bActive
		&& PartyFormationDrill.Phase == SolidPartyDrill::EPhase::Turning;
}

void ASolidCharacter::BeginPartyFormationDrillWalk()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = PartyFormationDrill.bSavedOrientRotationToMovement;
		Move->MaxWalkSpeed = WalkSpeed;
	}
	bIsSprinting = false;

	PartyFormationDrill.Phase = SolidPartyDrill::EPhase::Walking;
	PartyFormationDrill.PhaseSecondsRemaining = SolidPartyDrill::LegDurationSeconds;
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

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		PartyFormationDrill.bSavedOrientRotationToMovement = Move->bOrientRotationToMovement;
		Move->MaxWalkSpeed = WalkSpeed;
	}
	bIsSprinting = false;

	if (SolidPartyDrill::TurnsBeforeWalk(Leg))
	{
		const float StartYaw = GetActorRotation().Yaw;
		PartyFormationDrill.TurnStartYaw = StartYaw;
		PartyFormationDrill.TurnTargetYaw = StartYaw + SolidPartyDrill::TurnYawDegrees;
		PartyFormationDrill.Phase = SolidPartyDrill::EPhase::Turning;
		PartyFormationDrill.PhaseSecondsRemaining = SolidPartyDrill::TurnDurationSeconds;

		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->bOrientRotationToMovement = false;
			Move->StopMovementImmediately();
		}

		UE_LOG(LogSolid, Warning,
			TEXT("Formation drill leg %d/%d → F%d turn +%.0f° over %.2fs"),
			Leg + 1,
			SolidPartyDrill::NumLegs,
			PlanSlot + 1,
			SolidPartyDrill::TurnYawDegrees,
			SolidPartyDrill::TurnDurationSeconds);
		return;
	}

	BeginPartyFormationDrillWalk();
	UE_LOG(LogSolid, Warning,
		TEXT("Formation drill leg %d/%d → F%d walk %.1fs"),
		Leg + 1,
		SolidPartyDrill::NumLegs,
		PlanSlot + 1,
		SolidPartyDrill::LegDurationSeconds);
}

void ASolidCharacter::ApplyPartyFormationDrillYaw(const float YawDegrees)
{
	SetActorRotation(FRotator(0.f, YawDegrees, 0.f));
	if (AController* C = GetController())
	{
		FRotator ControlRot = C->GetControlRotation();
		ControlRot.Yaw = YawDegrees;
		C->SetControlRotation(ControlRot);
	}
}

void ASolidCharacter::TickPartyFormationDrill(const float DeltaTime)
{
	if (!PartyFormationDrill.bActive)
	{
		return;
	}

	if (PartyFormationDrill.Phase == SolidPartyDrill::EPhase::Turning)
	{
		const float Duration = FMath::Max(SolidPartyDrill::TurnDurationSeconds, KINDA_SMALL_NUMBER);
		PartyFormationDrill.PhaseSecondsRemaining -= DeltaTime;
		const float Alpha = FMath::Clamp(
			1.f - (PartyFormationDrill.PhaseSecondsRemaining / Duration),
			0.f,
			1.f);
		const float Yaw = FMath::Lerp(
			PartyFormationDrill.TurnStartYaw,
			PartyFormationDrill.TurnTargetYaw,
			Alpha);
		ApplyPartyFormationDrillYaw(Yaw);

		if (PartyFormationDrill.PhaseSecondsRemaining > 0.f)
		{
			return;
		}

		ApplyPartyFormationDrillYaw(PartyFormationDrill.TurnTargetYaw);
		BeginPartyFormationDrillWalk();
		UE_LOG(LogSolid, Warning,
			TEXT("Formation drill leg %d/%d walk %.1fs"),
			PartyFormationDrill.CurrentLeg + 1,
			SolidPartyDrill::NumLegs,
			SolidPartyDrill::LegDurationSeconds);
		return;
	}

	AddMovementInput(GetActorForwardVector(), 1.f);

	PartyFormationDrill.PhaseSecondsRemaining -= DeltaTime;
	if (PartyFormationDrill.PhaseSecondsRemaining > 0.f)
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
