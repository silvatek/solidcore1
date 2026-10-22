# Combat preparation

Status: Done

## Story

This story depends on [Early game battle plans](early-game-battle-plans.md), which is Done, and [Basic monsters](basic-monsters.md), which is Done. Basic monsters ships first.

A weapon has Speed and Power. Armour has Strength. The specific amounts are chosen in play, and they stay off the screen. The captain swaps a weapon or their armour in the inventory. The body slot can be replaced with other armour. It cannot be dragged off.

Each companion is assigned one fixed weapon and one fixed armour. Those stay on the companion. A monster has one fixed weapon, such as claws or teeth, and one fixed armour. Those stay on the monster.

The squad selects from the battle plans it already has. Each of those plans stores attack, hold, or retreat on itself. Starter, Attack, Tight mob, and Loose mob store attack. Parade stores hold. Defend, Line, and Column store retreat. Each plan also stores TargetStrategy on itself. The only value is Random, and Random picks a target purely at random. A group of monsters has a set of battle plans too. The three reptiles beside the Iglin bridge are one group, and that set holds Attack. Attack is the active plan.

## Acceptance

- A weapon has Speed and Power. The crude spear and the rusty sword each have both.
- The captain swaps that weapon in the inventory.
- A monster has one fixed weapon, with its own Speed and Power. Claws and teeth are such weapons. The monster does not swap it.
- Armour has Strength. Armour is the piece on the body. Rags have Strength. A helmet, boots, and a belt do not add Strength.
- The captain swaps that armour in the inventory.
- Each companion is assigned the crude spear and Rags. Sam, Alex, Max, Morgan, and Quinn each have that pair. The companion does not swap it. The inventory page stays the captain's.
- A monster has one fixed armour, with Strength. The monster does not swap it.
- Speed, Power, and Strength stay off the screen. Their amounts are chosen in play.
- The body slot can be replaced. Dragging another armour onto it swaps, and the body still holds armour. Dragging the body armour onto an empty pack cell, an empty chest cell, or a crafting slot does nothing. The body armour stays.
- The squad's battle plans are the set it can already select. This story leaves that set, and which plan is active, as [Early game battle plans](early-game-battle-plans.md) left them.
- Starter, Attack, Tight mob, and Loose mob each store attack on the plan. Parade stores hold. Defend, Line, and Column each store retreat on the plan.
- Each of those plans stores TargetStrategy on the plan. The only value is Random.
- A group of monsters has a set of battle plans. The three reptiles are one group. Their set holds Attack, and Attack is the active plan.

## Implementation notes

- Play-testing chooses the amounts of Speed, Power, and Strength. This story does not fix those numbers.
- An item is one instance of an item type. A stack is an item whose count is greater than one, and that item has no Power. `SolidItemType::PowerFor` converts a weapon item's quality into its Power. A monster's weapon uses that same roll, with size in place of quality. Tiny is the first step. Immense is the top step and narrows the high side. The Iglin raptors wear Claws and Hide. Speed and Strength are the starting figures, chosen in play.
- The captain's loadout already puts a weapon in the main hand and armour on the body. The body drag refuses a drop that would empty the slot, and accepts a swap onto other armour. A companion stores the crude spear and Rags on the company record. This story does not open an inventory page for a companion. The monster's weapon and armour are not catalog ids. Each monster holds that pair. The group does not.
- The company writes Attack in `AddDefaultBattlePlans`: the name Attack, the Mob formation, standard spacing, attack, and Random. Starter and Defend stay on Line at standard spacing. Each plan stores attack, hold, or retreat. Starter, Attack, Tight mob, and Loose mob store attack. Parade stores hold. Defend, Line, and Column store retreat. Each plan stores TargetStrategy, and the only value is Random. Picking a target waits for [Basic combat](basic-combat.md). A battle plan is the plain class `FSolidBattlePlan`, in `Source/SolidCore1/BattlePlans/`. The company catalog holds the squad's plans. A monster group holds its own available plans and the index of the active one. The Iglin Bridge group names Attack, and that name is found in the monster battle-plan array. That array is written once in the encounter code. The company's Attack plan is separate. Hold on Parade is the aggression. [Hold battle plan](../hold-battle-plan.md) and [Drill battle plan](../drill-battle-plan.md) are other stories. The values here are on the plans [Early game battle plans](early-game-battle-plans.md) names.
- [Basic combat](basic-combat.md) depends on this story. This story ships first.
