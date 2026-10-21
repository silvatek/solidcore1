# Basic combat

Status: New

## Story

This story depends on [Early game battle plans](early-game-battle-plans.md), which is Ready, and [Basic monsters](basic-monsters.md), which is Ready. Those stories ship first.

A weapon has Speed and Power. Armour has Strength. The specific amounts are chosen in play. The captain and a companion swap a weapon or their armour in the inventory, and the fight uses what they have on. A monster has one fixed weapon, such as claws or teeth, and one fixed armour. Those stay on the monster.

The squad selects from the battle plans it already has. A group of monsters has a set of battle plans too. The three reptiles beside the Iglin bridge are one group, and that set holds one plan.

When combat starts, the captain, each companion in the squad, and each monster reads the active plan. A plan that says to attack sends them to select a target, prepare, attack, and recover, and then through that again. Speed is how long prepare takes and how long recover takes. The attack lowers the target's armour Strength by the weapon's Power. Strength at 0 or below kills them. A plan that says to retreat sends them back from their enemies, and damage that lands while they retreat is reduced significantly.

[Basic monsters](basic-monsters.md) leaves a click on those reptiles with the fight unstarted. This story is that fight.

## Acceptance

- A weapon has Speed and Power. The crude spear and the rusty sword each have both. The weapon in the main hand is the one the fight uses.
- The captain and a companion swap that weapon in the inventory. The fight then uses the weapon they have on.
- A monster has one fixed weapon, with its own Speed and Power. Claws and teeth are such weapons. The monster does not swap it.
- Armour has Strength. Armour is the piece on the body. Rags have Strength. A helmet, boots, and a belt do not add Strength.
- The captain and a companion swap that armour in the inventory. An attack lands on the armour they have on.
- A monster has one fixed armour, with Strength. The monster does not swap it.
- Speed, Power, and Strength stay off the screen. Their amounts are chosen in play. The fight uses the amounts on the weapon and the armour.
- The squad's battle plans are the set it can already select. This story leaves that set, and which plan is active, as [Early game battle plans](early-game-battle-plans.md) left them.
- A group of monsters has a set of battle plans. The three reptiles are one group. Their set holds one plan, and that plan is the active plan.
- When combat starts, the captain, each companion in the squad, and each monster in the fight reads the active plan and follows it.
- A plan that says to attack selects a target, then prepares, attacks, and recovers. Prepare takes that weapon's Speed. Recover takes that weapon's Speed. After recover, the cycle starts again.
- The attack reduces the target's armour Strength by the weapon's Power. Strength at 0 or below kills that captain, companion, or monster.
- A plan that says to retreat moves that captain, companion, or monster back from enemies. Incoming damage is reduced significantly while they are retreating.
- A companion who is not in the squad stays at the stronghold.
- Standing on the Iglin ramp still fires Reach broken bridge. The three reptiles still do not.

## Implementation notes

- Play-testing chooses the amounts of Speed, Power, and Strength. This story does not fix those numbers.
- Which catalog plan says to attack, which says to retreat, how a target is chosen, what starts the fight, and how far the retreat reduction goes are still to be named before this story is Ready.
- `SolidItemType::PowerFor` rolls a hidden power from the item's grade. This story's Power is the attribute on the weapon. Leave that roll as it is. The attack spends the weapon's Power.
- The captain's loadout already puts a weapon in the main hand and armour on the body. A companion has those same two slots. The monster's weapon and armour are not catalog ids.
- Damage in the retreat rule is the Power taken off Strength.
