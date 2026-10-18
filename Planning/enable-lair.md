# Enable a lair from an event

Status: Ready

## Story

Driven from the village enables the Iglin tunnels. That is an `EnableLair` change on `IdDrivenFromVillage`, and the change names that lair. The mouths stay shut until the change has run. Opening them is not a side effect of the event id having fired.

## Acceptance

- `EChange` has `EnableLair`. `Change::EnableLair` and `FEvent::EnableLair` add it. The change names one lair. On `IdDrivenFromVillage` that lair is the Iglin tunnels (`SolidLair::IglinTunnels`).
- `IdDrivenFromVillage` still shows "You are driven from the village", titles the captain Outcast, and requires `IdKilledByMonsters`. It does not send the captain inside. The menu does not make the tunnel trip.
- Before that change, the Iglin tunnels are shut. A click on either mouth does nothing, and `TravelToLair` does not enter. Leave stays available.
- After the change, both mouths accept a click. Enter Iglin and Killed by monsters still leave the tunnels shut.
- The portal to the grass north of the X mouth still records `IdDrivenFromVillage` when Killed by monsters has not fired. That recording runs `EnableLair`, so the tunnels open. It does not open the cave and it does not enter the tunnels. A second use does not record the event again.
- `SolidLair::TunnelsOpen` does not read `bFired` for `IdDrivenFromVillage`. It reads whether the Iglin tunnels have been enabled.
- The journal for Driven from the village names the lair as enabled, with the message and the title.

## Implementation notes

- Today `SolidLair::TunnelsOpen` is true when `IdDrivenFromVillage` is in `State.bFired`. `ASolidGameMode::AreTunnelsOpen` calls that. `TravelToLair` returns immediately when it is false. `ASolidCharacter::AllowPassage` passes the result into `SolidLair::AllowPassage`, which drops an enter hit on a lair mouth.
- Store the enabled lair on `FGameState`, the way `bCaveEnabled` stores the cave. `ApplyChange` sets it from the lair the change names. One lair ships. The change still names it, so a later lair is another `EnableLair` line and not another hard-coded event id.
- `FChange` can hold the lair name the way it holds a message id. `SolidEvent.h` should not include `SolidLair.h`. `SolidLair.h` includes `SolidGameState.h`, which includes the event list.
- `EnableLair` does not set `bEnterCave` and does not travel. Driven from the village already has a message and a title, so one more change fits in `MaxChangesPerEvent`.
- The comment on that event says it opens the tunnel mouths. Once `EnableLair` says that, drop that clause. Keep the portal sentence. The portal is `MarkComplete` on `IdDrivenFromVillage`, which forces the event and will apply the new change.
- Update `Content/Lairs.md` so the shipped lair opens from `EnableLair`, not from the event id. Update the `SolidCore1.Events.*` and `SolidCore1.Lair.*` lines in `README.md`.
- `SolidCore1.Lair.Gate` and `SolidCore1.Portal.TunnelMouth` assert `TunnelsOpen` after Driven from the village. They stay valid when the open check follows the change. Add an event test that the definition carries `EnableLair` for the Iglin tunnels and that applying it enables that lair only.
- Read `Content/Lairs.md` before editing lair code. Leave the straight run. Do not use the word den. See [Remove den from the code](remove-den-from-the-code.md).
