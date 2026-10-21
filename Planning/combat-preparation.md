# Combat preparation

Status: New

## Story

This story depends on [Early game battle plans](early-game-battle-plans.md), which is In Progress, and [Basic monsters](basic-monsters.md), which is Ready. Those stories ship first.

A weapon has Speed and Power. Armour has Strength. The specific amounts are chosen in play, and they stay off the screen. The captain and a companion swap a weapon or their armour in the inventory. The body slot can be replaced with other armour. It cannot be dragged off.

A monster has one fixed weapon, such as claws or teeth, and one fixed armour. Those stay on the monster.

The squad selects from the battle plans it already has. Each of those plans stores attack or retreat on itself. Starter, Attack, Tight mob, and Loose mob store attack. Defend, Parade, Line, and Column store retreat. Each plan also stores TargetStrategy on itself. The only value is Random, and Random picks a target purely at random. A group of monsters has a set of battle plans too. The three reptiles beside the Iglin bridge are one group, and that set holds Starter.

## Acceptance

- A weapon has Speed and Power. The crude spear and the rusty sword each have both.
- The captain and a companion swap that weapon in the inventory.
- A monster has one fixed weapon, with its own Speed and Power. Claws and teeth are such weapons. The monster does not swap it.
- Armour has Strength. Armour is the piece on the body. Rags have Strength. A helmet, boots, and a belt do not add Strength.
- The captain and a companion swap that armour in the inventory.
- A monster has one fixed armour, with Strength. The monster does not swap it.
- Speed, Power, and Strength stay off the screen. Their amounts are chosen in play.
- The body slot can be replaced. Dragging another armour onto it swaps, and the body still holds armour. Dragging the body armour onto an empty pack cell, an empty chest cell, or a crafting slot does nothing. The body armour stays.
- The squad's battle plans are the set it can already select. This story leaves that set, and which plan is active, as [Early game battle plans](early-game-battle-plans.md) left them.
- Starter, Attack, Tight mob, and Loose mob each store attack on the plan. Defend, Parade, Line, and Column each store retreat on the plan.
- Each of those plans stores TargetStrategy on the plan. The only value is Random.
- A group of monsters has a set of battle plans. The three reptiles are one group. Their set holds Starter, and Starter is the active plan.

## Implementation notes

- Play-testing chooses the amounts of Speed, Power, and Strength. This story does not fix those numbers.
- `SolidItemType::PowerFor` rolls a hidden power from the item's grade. This story's Power is the attribute on the weapon. Leave that roll as it is.
- The captain's loadout already puts a weapon in the main hand and armour on the body. A companion has those same two slots. The monster's weapon and armour are not catalog ids. The body drag refuses a drop that would empty the slot, and accepts a swap onto other armour.
- Attack or retreat, and TargetStrategy, are fields on the battle plan, beside its name, formation, and spacing. [Hold battle plan](hold-battle-plan.md) and [Drill battle plan](drill-battle-plan.md) are other stories. The values here are on the plans [Early game battle plans](early-game-battle-plans.md) names.
- [Basic combat](basic-combat.md) depends on this story. This story ships first.
