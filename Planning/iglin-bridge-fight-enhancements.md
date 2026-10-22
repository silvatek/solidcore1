# Iglin Bridge Fight Enhancements

Status: New

## Story

The captain starts in Iglin and goes to investigate the broken bridge. Three Small raptors wait on the grass beside it. The fight kills the captain. The player cannot avoid that death.

Any death of the captain, at any point in the game, lands them in their current stronghold without the gear they had when they died. This fight is the first time that happens, and it is mandatory. The current stronghold is the cave.

When the three Small raptors kill the captain, the story moves on. When the captain kills one of those Small raptors before dying, or runs away, one Large raptor spawns as well. The captain cannot move until the fight kills them.

The fight takes the place of the message that standing on the bridge shows today. Reach broken bridge is removed.

This story depends on [Basic combat](basic-combat.md), which is Ready. That story ships first. [Combat preparation](combat-preparation.md) is In Progress, and it ships before Basic combat.

This story introduces no game concept. Raptor, the encounter, and the captain's death are already named.

## Acceptance

- The Iglin Bridge encounter still places three Small raptors on the Iglin side of the broken Iglin–Relion bridge, as [Basic monsters](Done/basic-monsters.md) placed them.
- The fight starts as [Basic combat](basic-combat.md) says, when the captain comes into DetectionRange. From then on this encounter kills the captain.
- When those three Small raptors kill the captain, and the captain has killed none of them, the fight ends on that death. No Large raptor spawns.
- When the captain kills one Small raptor, one Large raptor spawns into the same group. Killing another Small does not spawn a second Large.
- When the captain runs away, that same Large spawns, once. Running away is getting beyond every living monster's EscapeDistance, which is how [Basic combat](basic-combat.md) would otherwise end the fight. This encounter keeps going.
- The Large stands taller than the Small three. It fights with them, on the group's plan. Play-testing chooses how much taller.
- When the Large spawns, the captain cannot move. Walking and click-to-move leave the captain where they are. The block lasts until the captain dies. After the wake, the captain can move again.
- Any death of the captain, in this fight or later, anywhere in the game, uses one landing. The captain wakes in the company stronghold they have then, without the gear they had when they died. The first time, that stronghold is the cave. The wake is the one [Basic combat](basic-combat.md) specifies: Rags on the body, the rest of what they were carrying left where they died, the companions who were out despawned, and the next leave opening Build squad.
- This bridge death is that landing's first use. The opening cannot skip it. A later captain death uses the same landing, in whatever stronghold the company has by then.
- Reach broken bridge is removed. Standing on either ramp of the Iglin bridge records nothing and shows nothing. Reaching the bridge does not show "You are killed by monsters".
- The captain's death leaves Killed by monsters unfired. [Basic combat](basic-combat.md) already keeps that event off a death. This story does not disable Starter, and it does not enable Attack or Defend. Driven from the village still requires Killed by monsters.
- Enter Relion still repairs the bridge. The center is drawn. The road stays `Iglin -> Relion, track`.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- The placed catalog row stays three Small raptors. The Large is spawned during the fight, into the group's members, and is not a fourth placed member. [Place encounter markers](place-encounter-markers.md) can still move the focus onto a chart mark. The band, the separation, and the count of three stay.
- Small stands 100 cm, at `FSolidRaptor::SmallDrawnBodyFactor`. Large scales up from Small. The factor waits on play-testing, the way Speed and Strength do. The Large uses the same green mesh and the same idle until a later body.
- Each raptor holds Claws and Hide. Power is `SolidMonster::PowerFor` of that raptor's size, so the Large hits above the Small three. Speed starts at 1 and Hide's Strength starts at 1.
- Spawn the Large on a grassland point beside the captain, inside DetectionRange, clear of a mountain quad, a tree, and a foraging pile. The captain is already stopped, so the point has to be close enough for the fight to continue. One spawn. A second trigger finds the Large already there.
- The movement block is this encounter only. It covers the captain's move, including click-to-move. Look can stay. The block clears on the wake.
- The opening loadout is Rags on the body and the crude spear in the main hand. `SolidInventory::Clear` is the wake's inventory: Rags on the body, and the spear and anything else they were carrying gone from the captain. The stronghold chest stays. The gear left at the death site is the drop [Basic combat](basic-combat.md) already specifies.
- A captain death outside a fight uses that same landing. [Basic combat](basic-combat.md) specifies it for a death in a fight. This story is what makes it every captain death. The bridge is only the first, mandatory, time.
- Reach broken bridge is `IdReachBrokenBridge` (`a17c4e58`), trigger `ETrigger::ReachBrokenBridge`. `NoteBrokenBridgeRamp` calls `NotifyReachBrokenBridge` when the captain stands on an Iglin ramp. The event shows `SolidMessages::IdKilledByMonsters`. Closing that message is the separate event Killed by monsters (`e51b8c36`), which enables Meagre, clears the inventory, disables Starter, and enables Attack and Defend. [Return to the stronghold](return-to-stronghold.md) moves the trip onto that close. Removing the ramp event means that message is not shown, so that close does not run, unless some later story fires it. This story does not.
- [Basic combat](basic-combat.md) says standing on the Iglin ramp still fires Reach broken bridge. This story removes that.
- Tests that reach the later chain by standing on the ramp, or by calling `NotifyReachBrokenBridge`, need another setup once the event is gone. `SolidCore1.Events.ReachBrokenBridge` goes. The product commit updates the `SolidCore1.Events.*` line in `Source/SolidCore1/Tests/README.md`. Monster tests gain the Large spawn, the single spawn, the movement block, and the wake.
