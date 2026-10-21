# Basic combat

Status: New

## Story

This story depends on [Early game battle plans](early-game-battle-plans.md), which is In Progress, and [Basic monsters](basic-monsters.md), which is Ready. Those stories ship first.

A weapon has Speed and Power. Armour has Strength. The specific amounts are chosen in play. The captain and a companion swap a weapon or their armour in the inventory, and the fight uses what they have on. A monster has one fixed weapon, such as claws or teeth, and one fixed armour. Those stay on the monster. Each monster has DetectionRange. It starts at 5 m. When the captain or a companion in the squad enters that range, the monsters attack and combat starts. When the captain and every companion in the squad are beyond it, combat ends. Combat also ends when one side is completely killed, and when the captain dies. A monster stays within a few metres of its spawn point.

The squad selects from the battle plans it already has. Each of those plans stores attack or retreat on itself. Starter, Attack, Tight mob, and Loose mob store attack. Defend, Parade, Line, and Column store retreat. Each plan also stores TargetStrategy on itself. The only value is Random, and Random picks a target purely at random. A group of monsters has a set of battle plans too. The three reptiles beside the Iglin bridge are one group, and that set holds Starter.

When combat starts, the captain, each companion in the squad, and each monster reads the active plan. A plan that stores attack sends them to select a target by that plan's TargetStrategy, prepare, attack, and recover, and then through that again. Speed is how long prepare takes and how long recover takes. The attack lowers the target's armour Strength by the weapon's Power. Strength at 0 or below kills them. When the captain dies, the fight ends. The captain loses all of their inventory and wakes in the company stronghold. The body and the gear stay where the captain died. A plan that stores retreat sends them back from their enemies, and damage that lands while they retreat is reduced by 75%. Play-testing chooses that reduction.

A click on those reptiles still leaves the fight unstarted. Entering DetectionRange starts it.

## Acceptance

- A weapon has Speed and Power. The crude spear and the rusty sword each have both. The weapon in the main hand is the one the fight uses.
- The captain and a companion swap that weapon in the inventory. The fight then uses the weapon they have on.
- A monster has one fixed weapon, with its own Speed and Power. Claws and teeth are such weapons. The monster does not swap it.
- Armour has Strength. Armour is the piece on the body. Rags have Strength. A helmet, boots, and a belt do not add Strength.
- The captain and a companion swap that armour in the inventory. An attack lands on the armour they have on.
- A monster has one fixed armour, with Strength. The monster does not swap it.
- Each monster has DetectionRange. The three reptiles each start at 5 m. Play-testing chooses that distance. The starting figure is 5 m.
- The captain or a companion in the squad entering any of those ranges starts combat. The group then follows its plan. While the captain is alive, one living companion still inside keeps combat going.
- Combat ends when the captain and every companion in the squad are outside every monster's DetectionRange. The attack cycle and the retreat stop. Entering a range again starts combat again.
- Combat ends when every monster in the fight is dead, or when the captain and every companion in the squad are dead. That ends it even while the other side is still inside a DetectionRange. The attack cycle and the retreat stop. A dead monster does not start combat again.
- A click on a monster does not start combat.
- A monster moves no more than a few metres from its spawn point. That point is the place [Basic monsters](basic-monsters.md) put it.
- Speed, Power, and Strength stay off the screen. Their amounts are chosen in play. The fight uses the amounts on the weapon and the armour.
- The squad's battle plans are the set it can already select. This story leaves that set, and which plan is active, as [Early game battle plans](early-game-battle-plans.md) left them.
- Starter, Attack, Tight mob, and Loose mob each store attack on the plan. Defend, Parade, Line, and Column each store retreat on the plan. The fight reads that value from the plan.
- Each of those plans stores TargetStrategy on the plan. The only value is Random. Random chooses any opponent still in the fight, and each of them is equally likely.
- A group of monsters has a set of battle plans. The three reptiles are one group. Their set holds Starter, and Starter is the active plan.
- When combat starts, the captain, each companion in the squad, and each monster in the fight reads the active plan and follows it.
- A plan that stores attack selects a target by that plan's TargetStrategy, then prepares, attacks, and recovers. Prepare takes that weapon's Speed. Recover takes that weapon's Speed. After recover, the cycle starts again.
- The attack reduces the target's armour Strength by the weapon's Power. Strength at 0 or below kills that captain, companion, or monster.
- When the captain dies, combat ends. The attack cycle and the retreat stop. It ends even while a companion is still inside a DetectionRange. A companion who is already inside does not start combat again by staying there.
- When the captain dies, every worn box, the pack, and the crafting slots are emptied. Rags do not go back on. The captain wakes alive in the company stronghold. The body and the gear stay where the captain died. A companion in the squad stays where they are. They are out of that fight.
- A plan that stores retreat moves that captain, companion, or monster back from enemies. Incoming damage is reduced by 75% while they are retreating, so the Strength lost is a quarter of the weapon's Power. Play-testing chooses that reduction. The starting figure is 75%.
- A companion who is not in the squad stays at the stronghold.
- Standing on the Iglin ramp still fires Reach broken bridge. The three reptiles still do not.

## Implementation notes

- Play-testing chooses the amounts of Speed, Power, and Strength. This story does not fix those numbers.
- The retreat reduction starts at 75%. A blow on someone who is retreating takes 25% of the weapon's Power off their Strength. Play-testing may change the 75%.
- An opponent of the captain or a companion is a living monster in the fight. An opponent of a monster is the living captain, or a living companion in the squad.
- A side is the monsters in the fight, or the captain and the companions in the squad. Strength at 0 or below is dead. The blow that kills the last one on a side ends combat. The blow that kills the captain ends combat even when a companion is still alive.
- The captain's death is not the monster-message close. It does not enable Meagre, it does not clear the inventory down to Rags, and it does not record Killed by monsters. The wake uses the company stronghold, the cave until the company has moved. The stronghold chest stays as it was.
- DetectionRange starts at 5 m, which is 500 cm. Distance is horizontal. Play-testing may change the 5 m. The leash is a few metres from the spawn point, and play-testing chooses the exact metres.
- A companion who is not in the squad stays at the stronghold, so that companion does not enter a range and does not keep combat going.
- Attack or retreat, and TargetStrategy, are fields on the battle plan, beside its name, formation, and spacing. The fight reads them from the plan it is using. [Hold battle plan](hold-battle-plan.md) and [Drill battle plan](drill-battle-plan.md) are other stories. The values here are on the plans [Early game battle plans](early-game-battle-plans.md) names.
- `SolidItemType::PowerFor` rolls a hidden power from the item's grade. This story's Power is the attribute on the weapon. Leave that roll as it is. The attack spends the weapon's Power.
- The captain's loadout already puts a weapon in the main hand and armour on the body. A companion has those same two slots. The monster's weapon and armour are not catalog ids.
- Damage in the retreat rule is the Power taken off Strength.
