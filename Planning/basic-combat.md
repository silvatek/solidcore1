# Basic combat

Status: New

## Story

This story depends on [Early game battle plans](early-game-battle-plans.md), which is In Progress, and [Basic monsters](basic-monsters.md), which is Ready. Those stories ship first.

A weapon has Speed and Power. Armour has Strength. The specific amounts are chosen in play. The captain and a companion swap a weapon or their armour in the inventory, and the fight uses what they have on. A monster has one fixed weapon, such as claws or teeth, and one fixed armour. Those stay on the monster. Each monster has DetectionRange and EscapeDistance. Each starts at 5 m. Play can change either one on its own. When the captain or a companion in the squad enters DetectionRange, the monsters attack and combat starts. When the captain and every companion in the squad are beyond EscapeDistance, combat ends. Combat also ends when one side is completely killed, and when the captain dies. A monster stays within its EscapeDistance of its spawn point.

The squad selects from the battle plans it already has. Each of those plans stores attack or retreat on itself. Starter, Attack, Tight mob, and Loose mob store attack. Defend, Parade, Line, and Column store retreat. Each plan also stores TargetStrategy on itself. The only value is Random, and Random picks a target purely at random. A group of monsters has a set of battle plans too. The three reptiles beside the Iglin bridge are one group, and that set holds Starter.

When combat starts, the captain, each companion in the squad, and each monster reads the active plan. A plan that stores attack sends them to select a target by that plan's TargetStrategy, prepare, attack, and recover, and then through that again. Speed is how long prepare takes and how long recover takes. The attack lowers the target's armour Strength by the weapon's Power. It lands at any distance. An empty main hand makes no attack. Strength at 0 or below kills them. Any death enables the Meagre stronghold, the cave. When the captain dies, the fight ends, and the companions who were out are despawned. The captain loses their inventory and wakes in the company stronghold with Rags in the armour slot. The body and the gear stay where the captain died. Leaving that stronghold opens Build squad, and the selected companions spawn on the grass outside. The body slot can be replaced with other armour. It cannot be dragged off. When prepare finishes, the plan is read again. If it stores retreat, the attack is abandoned. A plan that stores retreat sends them back from their enemies, and damage that lands while they retreat is reduced by 75%. Play-testing chooses that reduction.

A click on those reptiles still leaves the fight unstarted. Entering DetectionRange starts it.

## Acceptance

- A weapon has Speed and Power. The crude spear and the rusty sword each have both. The weapon in the main hand is the one the fight uses.
- The captain and a companion swap that weapon in the inventory. The fight then uses the weapon they have on.
- A monster has one fixed weapon, with its own Speed and Power. Claws and teeth are such weapons. The monster does not swap it.
- Armour has Strength. Armour is the piece on the body. Rags have Strength. A helmet, boots, and a belt do not add Strength.
- The captain and a companion swap that armour in the inventory. An attack lands on the armour they have on.
- A monster has one fixed armour, with Strength. The monster does not swap it.
- Each monster has DetectionRange and EscapeDistance. The three reptiles start each at 5 m. The two values are separate. Play-testing may change either one. The starting figure for each is 5 m.
- The captain or a companion in the squad entering any monster's DetectionRange starts combat. The group then follows its plan. While the captain is alive, one living companion still inside an EscapeDistance keeps combat going.
- Combat ends when the captain and every companion in the squad are outside every monster's EscapeDistance. The attack cycle and the retreat stop. Entering a DetectionRange again starts combat again.
- Combat ends when every monster in the fight is dead, or when the captain and every companion in the squad are dead. That ends it even while the other side is still inside an EscapeDistance. The attack cycle and the retreat stop. A dead monster does not start combat again.
- A click on a monster does not start combat.
- A monster moves no farther than its EscapeDistance from its spawn point. That point is the place [Basic monsters](basic-monsters.md) put it.
- An empty main hand makes no attack. Prepare, attack, and recover do not run for that captain or companion. A monster's fixed weapon is never empty.
- Speed, Power, and Strength stay off the screen. Their amounts are chosen in play. The fight uses the amounts on the weapon and the armour.
- The squad's battle plans are the set it can already select. This story leaves that set, and which plan is active, as [Early game battle plans](early-game-battle-plans.md) left them.
- Starter, Attack, Tight mob, and Loose mob each store attack on the plan. Defend, Parade, Line, and Column each store retreat on the plan. The fight reads that value from the plan.
- Each of those plans stores TargetStrategy on the plan. The only value is Random. Random chooses any opponent still in the fight, and each of them is equally likely.
- A group of monsters has a set of battle plans. The three reptiles are one group. Their set holds Starter, and Starter is the active plan.
- When combat starts, the captain, each companion in the squad, and each monster in the fight reads the active plan and follows it.
- A plan that stores attack selects a target by that plan's TargetStrategy, then prepares, attacks, and recovers. Prepare takes that weapon's Speed. Recover takes that weapon's Speed. After recover, the cycle starts again.
- When prepare finishes, the plan is read again. If it stores retreat, the attack is abandoned. The blow does not land, and recover does not run. That fighter follows retreat.
- The attack reduces the target's armour Strength by the weapon's Power. The blow lands at any distance. Strength at 0 or below kills that captain, companion, or monster. Attack range is a later story.
- A death of the captain, a companion, or a monster enables the Meagre stronghold, the cave. Meagre that is already enabled stays enabled. The mouth then accepts a click while the cave is still the company stronghold. The death does not record Killed by monsters, and it does not send the captain there unless the captain is the one who died.
- When the captain dies, combat ends. The attack cycle and the retreat stop. It ends even while a companion is still inside an EscapeDistance. A companion who is already inside does not start combat again by staying there.
- When the captain dies, the companions who were out are despawned. The squad's selection is kept. The pack, the crafting slots, and every worn box except the body are emptied. The armour slot holds Rags. The captain wakes alive in the company stronghold wearing them. The body and the gear stay where the captain died.
- The next leave after that wake opens Build squad. The companions selected when the page closes spawn on the grass outside the stronghold. A companion who is not selected stays unspawned.
- The body slot can be replaced. Dragging another armour onto it swaps, and the body still holds armour. Dragging the body armour onto an empty pack cell, an empty chest cell, or a crafting slot does nothing. The body armour stays. A leave is allowed with that armour on.
- A plan that stores retreat moves that captain, companion, or monster back from enemies. Incoming damage is reduced by 75% while they are retreating, so the Strength lost is a quarter of the weapon's Power. Play-testing chooses that reduction. The starting figure is 75%.
- A companion who is not in the squad stays at the stronghold.
- Standing on the Iglin ramp still fires Reach broken bridge. The three reptiles still do not.

## Implementation notes

- Play-testing chooses the amounts of Speed, Power, and Strength. This story does not fix those numbers.
- The retreat reduction starts at 75%. A blow on someone who is retreating takes 25% of the weapon's Power off their Strength. Play-testing may change the 75%.
- An opponent of the captain or a companion is a living monster in the fight. An opponent of a monster is the living captain, or a living companion in the squad.
- A side is the monsters in the fight, or the captain and the companions in the squad. Strength at 0 or below is dead. The blow that kills the last one on a side ends combat. The blow that kills the captain ends combat even when a companion is still alive.
- The captain's death is not the monster-message close. It does not record Killed by monsters. It does enable Meagre, as any death in this fight does. The wake leaves Rags in the armour slot and empties the rest of the inventory. The body drag refuses a drop that would empty the slot, and accepts a swap onto other armour. The wake uses the company stronghold, the cave until the company has moved. The stronghold chest stays as it was. The journal records `Meagre size unlocked` when this death is what enables it.
- DetectionRange and EscapeDistance each start at 5 m, which is 500 cm. Distance is horizontal. A distance inside or equal to DetectionRange can start combat. A distance beyond EscapeDistance is outside it. Play-testing may change either figure on its own.
- The blow has no range. A later story adds attack range. This story does not.
- [Open the squad builder on leaving a stronghold](Done/squad-builder-on-stronghold-exit.md) is the Build squad page. The leave after this wake opens that page. The usual "nothing to arrange" check does not keep it closed. Closing the page spawns the selected companions outside, on the grass the leave already uses.
- A companion who is not in the squad stays at the stronghold, so that companion does not enter a range and does not keep combat going.
- Attack or retreat, and TargetStrategy, are fields on the battle plan, beside its name, formation, and spacing. The fight reads them from the plan it is using. [Hold battle plan](hold-battle-plan.md) and [Drill battle plan](drill-battle-plan.md) are other stories. The values here are on the plans [Early game battle plans](early-game-battle-plans.md) names.
- `SolidItemType::PowerFor` rolls a hidden power from the item's grade. This story's Power is the attribute on the weapon. Leave that roll as it is. The attack spends the weapon's Power.
- The captain's loadout already puts a weapon in the main hand and armour on the body. A companion has those same two slots. The monster's weapon and armour are not catalog ids.
- Damage in the retreat rule is the Power taken off Strength.
