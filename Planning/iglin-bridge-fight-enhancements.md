# Iglin Bridge Fight Enhancements

Status: New

## Story

The player fights the three Small raptors of the Iglin Bridge encounter, on the grass beside the broken Iglin–Relion bridge. This story is the enhancements to that fight. The enhancements are still to be agreed.

This story depends on [Basic combat](basic-combat.md), which is Ready. That story ships first. [Combat preparation](combat-preparation.md) is In Progress, and it ships before Basic combat.

## Acceptance

Not written yet.

## Implementation notes

These are the facts the enhancements sit on. They are already specified or already in the code.

- [Basic monsters](Done/basic-monsters.md) is Done. The catalog row is `Iglin Bridge` in `SolidEncounter::Definitions`. One group of three Small raptors. They stand, idle, and block a walk. A click does nothing. Standing beside one leaves Reach broken bridge unfired. A point at full fog is not seeded. The tunnels stay empty.
- The focus is the outer end of the Iglin ramp, site `IglinBridgeRamp`. Near limit 8 m, far limit 40 m, separation 4 m. A point is grassland, walkable, and clear of a mountain quad, a tree, and a foraging pile. They face that outer end. Yaw only. Standing height is 100 cm. Small is six times the vertex-span scale. Capsule radius is 27 cm. The capsule blocks the pawn and ignores the visibility channel.
- [Place encounter markers](place-encounter-markers.md) is Ready. It removes `ESite`. The focus becomes a chart mark, `Encounter (Iglin Bridge)`. The three raptors, the band, and the separation stay on the definition. A definition with no mark is absent from the world.
- [Combat preparation](combat-preparation.md) gives a weapon Speed and Power, and armour Strength. The three reptiles are one group. Their plan set holds Starter, and Starter is the active plan. The amounts stay off the screen. Play-testing chooses them. `FSolidMonster` holds one weapon and one armour. The Iglin raptors wear Claws and Hide, and the weapon's Power is the size roll. `FSolidMonsterGroup` holds members and seeded indexes. The comment on that type leaves battle plans and detection for a later story.
- [Basic combat](basic-combat.md) starts the fight when the captain or a companion in the squad enters DetectionRange. Each of DetectionRange and EscapeDistance starts at 5 m. A click still does not start the fight. Standing on an Iglin ramp still fires Reach broken bridge. The reptiles still do not. A death in the fight enables Meagre. It does not record Killed by monsters. The captain's death ends the fight, despawns the companions who were out, and wakes the captain in the company stronghold in Rags.
- Reach broken bridge is ACT_0, id `a17c4e58`. It requires Enter Iglin. Standing on either ramp of the Iglin–Relion bridge shows "You are killed by monsters". The deck and the gap past the lip do not count. Closing that message is Killed by monsters. That close enables Meagre, clears the inventory to Rags, disables Starter, and enables Attack and Defend. Enter Relion sets `bIglinBridgeRepaired`. The center is then drawn. The road is `Iglin -> Relion, track`.
