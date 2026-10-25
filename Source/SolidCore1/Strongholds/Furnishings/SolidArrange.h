#pragma once

#include "CoreMinimal.h"

/**
 * Arrange mode inside a company stronghold.
 * Stage 1 is the mode itself: on or off. A later stage lifts a piece.
 */
class FSolidArrange
{
public:
	struct FContext
	{
		/** The company stronghold. The open world and the tunnels are not. */
		bool bInStronghold = false;
		/** A menu, the inventory, the journal, a message, or the squad builder. */
		bool bOverlay = false;
	};

	bool IsOn() const { return bOn; }

	/**
	 * `\` or the sign.
	 * Turning on needs the stronghold and a clear screen.
	 * An overlay leaves the mode as it is.
	 */
	bool Toggle(const FContext& Context);

	/** Esc, or leaving the stronghold. The captain is drawn again. */
	void Leave();

private:
	bool bOn = false;
};
