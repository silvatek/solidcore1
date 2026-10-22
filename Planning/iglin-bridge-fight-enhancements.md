# Iglin Bridge Fight Enhancements

Status: New

## Story

The captain starts in Iglin. Three Small raptors wait on the grass beside the broken bridge. The player can leave them alone. This is an open world. When the captain does come into the fight, it kills them. The player cannot avoid that death.

Any death of the captain, at any point in the game, lands them in their current stronghold without the gear they had when they died. This fight is the first time that happens. Until that death, the company stronghold stays locked, the Iglin tunnels stay locked, and the broken bridge still blocks the river. The current stronghold, once that death opens it, is the cave. A later story adds the NPC Rethil, who guides the player toward the bridge. This story does not.

When the three Small raptors kill the captain, the story moves on. When the captain kills one of those Small raptors before dying, or runs away, one Large raptor spawns as well. The captain cannot move until the fight kills them.

The fight takes the place of the message that standing on the bridge shows today. Reach broken bridge is removed.

The first time the captain dies, FirstDeath fires. Like every event, it fires once. A later death does not fire it again. FirstDeath disables Starter and enables Attack and Defend. Driven from the village requires FirstDeath.

This story depends on [Basic combat](basic-combat.md), which is Ready. That story ships first. [Combat preparation](combat-preparation.md) is In Progress, and it ships before Basic combat.

This story introduces no game concept. Raptor, the encounter, the captain's death, and the event list are already named. FirstDeath is another event on that list.

## Acceptance

- The Iglin Bridge encounter still places three Small raptors on the Iglin side of the broken Iglin–Relion bridge, as [Basic monsters](Done/basic-monsters.md) placed them.
- The fight starts as [Basic combat](basic-combat.md) says, when the captain comes into DetectionRange. Staying outside that range leaves the raptors idle. From the moment the fight starts, this encounter kills the captain.
- When those three Small raptors kill the captain, and the captain has killed none of them, the fight ends on that death. No Large raptor spawns.
- When the captain kills one Small raptor, one Large raptor spawns into the same group. Killing another Small does not spawn a second Large.
- When the captain runs away, that same Large spawns, once. Running away is getting beyond every living monster's EscapeDistance, which is how [Basic combat](basic-combat.md) would otherwise end the fight. This encounter keeps going.
- The Large stands taller than the Small three. It fights with them, on the group's plan. Play-testing chooses how much taller. Its armour Strength is huge.
- When the Large spawns, the captain cannot move. Walking and click-to-move leave the captain where they are. The block lasts until the captain dies. After the wake, the captain can move again.
- Any death of the captain, in this fight or later, anywhere in the game, uses one landing. The captain wakes in the company stronghold they have then, without the gear they had when they died. The first time, that stronghold is the cave. The wake is the one [Basic combat](basic-combat.md) specifies: Rags on the body, the rest of what they were carrying left where they died, the companions who were out despawned, and the next leave opening Build squad.
- This bridge death is that landing's first use. A later captain death uses the same landing, in whatever stronghold the company has by then.
- Until that death, the cave mouth stays shut, the Iglin tunnels stay shut, and the Iglin–Relion bridge stays broken. The river still cannot be crossed there, so the wider world stays out of reach. The death opens the stronghold. The tunnels still wait on Driven from the village. The bridge still waits on Enter Relion.
- This story does not add Rethil. A later story adds that NPC, who guides the player toward the bridge.
- Reach broken bridge is removed. Standing on either ramp of the Iglin bridge records nothing and shows nothing. Reaching the bridge does not show "You are killed by monsters".
- FirstDeath fires when the captain dies. It is ACT_0. The journal names it `First death`. It fires once. A second death leaves it unfired and leaves its changes as they are.
- FirstDeath disables Starter and enables Attack and Defend. The squad's selection becomes Attack, then Defend. The active plan becomes Attack. The battle-plan list shows F1 Attack and F2 Defend. The journal records `Starter disabled`, `Attack enabled`, and `Defend enabled`.
- Driven from the village requires FirstDeath. It does not require Killed by monsters. The portal to the grass north of the X mouth still records Driven from the village when FirstDeath has not fired.
- Killed by monsters no longer disables Starter and no longer enables Attack or Defend. The gear they lose, and the wake in the current stronghold, stay on every captain death. FirstDeath does not take that landing. [Basic combat](basic-combat.md) still enables Meagre on a death.
- Enter Relion still repairs the bridge. The center is drawn. The road stays `Iglin -> Relion, track`.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- The placed catalog row stays three Small raptors. The Large is spawned during the fight, into the group's members, and is not a fourth placed member. [Place encounter markers](place-encounter-markers.md) can still move the focus onto a chart mark. The band, the separation, and the count of three stay.
- Small stands 100 cm, at `FSolidRaptor::SmallDrawnBodyFactor`. Large scales up from Small. The height factor waits on play-testing. The Large uses the same green mesh and the same idle until a later body.
- Each raptor holds Claws and Hide. Power is `SolidMonster::PowerFor` of that raptor's size, so the Large hits above the Small three. Speed starts at 1. The Small raptors' Hide starts at Strength 1. The Large's Hide has a huge Strength, so a rooted captain does not cut it down before the fight kills them.
- Spawn the Large on a grassland point beside the captain, inside DetectionRange, clear of a mountain quad, a tree, and a foraging pile. The captain is already stopped, so the point has to be close enough for the fight to continue. One spawn. A second trigger finds the Large already there.
- The movement block is this encounter only. It covers the captain's move, including click-to-move. Look can stay. The block clears on the wake.
- The opening loadout is Rags on the body and the crude spear in the main hand. `SolidInventory::Clear` is the wake's inventory: Rags on the body, and the spear and anything else they were carrying gone from the captain. The stronghold chest stays. The gear left at the death site is the drop [Basic combat](basic-combat.md) already specifies.
- A captain death outside a fight uses that same landing. [Basic combat](basic-combat.md) specifies it for a death in a fight. This story is what makes it every captain death. The bridge is the first time, and the player chooses when to walk into it.
- Before that death the cave mouth ignores a click, because Meagre is still locked. Driven from the village still opens the Iglin tunnels, and it requires FirstDeath, so the tunnels stay shut while the raptors are left alone. Enter Relion still repairs the bridge. With both the bridge broken and the tunnels shut, the river is the edge of the world the captain can reach. Rethil is not in this story. [Basic NPCs](basic-npcs.md) does not name him either.
- Reach broken bridge is `IdReachBrokenBridge` (`a17c4e58`), trigger `ETrigger::ReachBrokenBridge`. `NoteBrokenBridgeRamp` calls `NotifyReachBrokenBridge` when the captain stands on an Iglin ramp. The event shows `SolidMessages::IdKilledByMonsters`. Closing that message is Killed by monsters (`e51b8c36`). Today that close enables Meagre, clears the inventory, disables Starter, and enables Attack and Defend. [Return to the stronghold](return-to-stronghold.md) moves the trip onto that close.
- FirstDeath is a new definition, `IdFirstDeath`, trigger captain death, name `First death`, ACT_0. `ETrigger` gains that trigger. The captain's death notifies it. The once-only rule is the one every event already has. Its changes are `DisableBattlePlan(Starter)`, `EnableBattlePlan(Attack)`, and `EnableBattlePlan(Defend)`. Driven from the village's `Require` names `IdFirstDeath` instead of `IdKilledByMonsters`. The portal still uses `MarkComplete` on Driven from the village, so it still records that event when FirstDeath has not fired.
- The every-death landing keeps the gear loss and the wake. Those stay off FirstDeath, so a later death still loses the gear and still wakes in the current stronghold. Meagre stays on the death, as [Basic combat](basic-combat.md) has it. [Return to the stronghold](return-to-stronghold.md) currently puts the trip on the message close. This story puts the trip on every captain death.
- Killed by monsters keeps the message id. Nothing in this story shows that message, and the plan changes have left that event. [Basic combat](basic-combat.md) says standing on the Iglin ramp still fires Reach broken bridge. This story removes that.
- Tests that reach the later chain by standing on the ramp, or by calling `NotifyReachBrokenBridge`, notify FirstDeath from a captain death instead. `SolidCore1.Events.ReachBrokenBridge` goes. A FirstDeath test checks the one fire, the plan changes, and Driven from the village's new requirement. The product commit updates the `SolidCore1.Events.*` line in `Source/SolidCore1/Tests/README.md`. Monster tests gain the Large spawn, the single spawn, the movement block, and the wake.
