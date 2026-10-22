# Basic combat

Status: In Progress

## Story

This story depends on [Combat preparation](Done/combat-preparation.md), which is Done. That story ships first.

This story introduces Battle. A battle is one fight. `FSolidBattle` links to one Squad and one MonsterGroup. The squad already holds its battle plans, and the monster group already holds its own. The battle reads the active plan on each. It answers whether it has started, what each of them is doing, and whether it has ended. DetectionRange and EscapeDistance are facts on the monster. The weapon and the armour stay as they are. Prepare, attack, and recover are steps the battle runs. A side is that monster group, or that squad, and the battle answers whether that side is dead. The captain's death is that end. A simulation is a test that sets a battle up between a squad and a monster group. It is not a class in the game.

Each monster has DetectionRange and EscapeDistance. Each starts at 5 m. Play can change either one on its own. When the captain or a companion in the squad enters DetectionRange, the monsters attack and the battle starts. When the captain and every companion in the squad are beyond EscapeDistance, the battle ends. The battle also ends when one side is completely killed, and when the captain dies. A monster stays within its EscapeDistance of its spawn point.

When the battle starts, the captain, each companion in the squad, and each monster reads the active plan. A plan that stores attack sends them to select a target by that plan's TargetStrategy, prepare, attack, and recover, and then through that again. Speed is how long prepare takes and how long recover takes. The attack lowers the target's armour Strength by the weapon's Power. It lands at any distance. An empty main hand makes no attack. Strength at 0 or below kills them. Any death enables the Meagre stronghold, the cave. When the captain dies, the battle ends, and the companions who were out are despawned. The captain loses their inventory and wakes in the company stronghold with Rags in the armour slot. [Gear left at the death](gear-left-at-the-death.md) is where that gear stays. Leaving the stronghold opens Build squad when there is something to arrange. The companions selected when that page closes spawn on the grass outside. When prepare finishes, the plan is read again. If it stores retreat, the attack is abandoned. A plan that stores retreat sends them back from their enemies, and damage that lands while they retreat is reduced by 75%. Play-testing chooses that reduction.

A click on those reptiles still leaves the battle unstarted. Entering DetectionRange starts it.

## Acceptance

- The captain's weapon in the main hand is the one the captain's attack uses. A companion's attack uses that companion's fixed weapon and armour. An attack lands on the armour the target has on.
- Each monster has DetectionRange and EscapeDistance. The three reptiles start each at 5 m. The two values are separate. Play-testing may change either one. The starting figure for each is 5 m.
- The captain or a companion in the squad entering any monster's DetectionRange starts the battle. The group then follows its plan. While the captain is alive, one living companion still inside an EscapeDistance keeps the battle going.
- The battle ends when the captain and every companion in the squad are outside every monster's EscapeDistance. The attack cycle and the retreat stop. Entering a DetectionRange again starts the battle again.
- The battle ends when every monster in the battle is dead, or when the captain and every companion in the squad are dead. That ends it even while the other side is still inside an EscapeDistance. The attack cycle and the retreat stop. A dead monster does not start the battle again.
- A click on a monster does not start the battle.
- While a fight is underway, a board in the journal's oak and bronze sits on the right and lists the battle log. The newest lines stay when the board fills. The world stays visible and the controls stay with the captain. The board stays up after the last blow.
- A monster moves no farther than its EscapeDistance from its spawn point. That point is the place [Basic monsters](basic-monsters.md) put it.
- An empty main hand makes no attack. Prepare, attack, and recover do not run for that captain. A companion's fixed weapon is never empty. A monster's fixed weapon is never empty.
- The battle reads attack or retreat, and TargetStrategy, from the plan. Random chooses any opponent still in the battle, and each of them is equally likely.
- When the battle starts, the captain, each companion in the squad, and each monster in the battle reads the active plan and follows it.
- A plan that stores attack selects a target by that plan's TargetStrategy, then prepares, attacks, and recovers. Prepare takes that weapon's Speed. Recover takes that weapon's Speed. After recover, the cycle starts again.
- When prepare finishes, the plan is read again. If it stores retreat, the attack is abandoned. The blow does not land, and recover does not run. That fighter follows retreat.
- The attack reduces the target's armour Strength by the weapon's Power. The blow lands at any distance. Strength at 0 or below kills that captain, companion, or monster. Attack range is a later story.
- A death of the captain, a companion, or a monster enables the Meagre stronghold, the cave. Meagre that is already enabled stays enabled. The mouth then accepts a click while the cave is still the company stronghold. The captain's death records Killed by monsters when that event has not already fired. A companion's death or a monster's death leaves that event unfired. The death sends the captain to the stronghold when the captain is the one who died.
- When the captain dies, the battle ends. The attack cycle and the retreat stop. It ends even while a companion is still inside an EscapeDistance. A companion who is already inside does not start the battle again by staying there.
- When the captain dies, the companions who were out are despawned. The squad's selection is kept. The pack, the crafting slots, and every worn box except the body are emptied. The armour slot holds Rags. The captain wakes alive in the company stronghold wearing them.
- The leave after that wake uses the same Build squad check as any other leave. No companion is allowed yet, and the catalog is the eight plans the company starts with, so the page stays closed. A companion the events allow, or a plan beyond those eight, opens the page. The companions selected when the page closes spawn on the grass outside the stronghold. A companion who is not selected stays unspawned.
- A plan that stores retreat moves that captain, companion, or monster back from enemies. Incoming damage is reduced by 75% while they are retreating, so the Strength lost is a quarter of the weapon's Power. Play-testing chooses that reduction. The starting figure is 75%.
- A companion who is not in the squad stays at the stronghold.
- Standing on the Iglin ramp still fires Reach broken bridge. The three reptiles still do not.

## Implementation notes

- The retreat reduction starts at 75%. A blow on someone who is retreating takes 25% of the weapon's Power off their Strength. Play-testing may change the 75%.
- An opponent of the captain or a companion is a living monster in the battle. An opponent of a monster is the living captain, or a living companion in the squad.
- A side is the monsters in the battle, or the captain and the companions in the squad. Strength at 0 or below is dead. The blow that kills the last one on a side ends the battle. The blow that kills the captain ends the battle even when a companion is still alive.
- The captain's death records Killed by monsters when that event has not already fired. Closing the monster message still records it too, and a second record does nothing. The death enables Meagre, as any death in this battle does. The wake leaves Rags in the armour slot and empties the rest of the inventory. The wake uses the company stronghold, the cave until the company has moved. The stronghold chest stays as it was. The journal records `Meagre size unlocked` when this death is what enables it.
- DetectionRange and EscapeDistance each start at 5 m, which is 500 cm. Distance is horizontal. A distance inside or equal to DetectionRange can start the battle. A distance beyond EscapeDistance is outside it. Play-testing may change either figure on its own.
- The blow has no range. A later story adds attack range. This story does not. The attack spends the weapon's Power from [Combat preparation](Done/combat-preparation.md).
- The battle board reads a copy of the captain's kit. When that copy's captain dies, the wake fires Killed by monsters. The event clears the play kit to Rags, opens the cave, and sends the captain inside.
- [Open the squad builder on leaving a stronghold](Done/squad-builder-on-stronghold-exit.md) is the Build squad page. The leave after this wake uses that check. No companion is allowed, and the catalog is only the plans the company starts with, so the page stays closed. Closing the page, when it did open, spawns the selected companions outside, on the grass the leave already uses.
- A companion who is not in the squad stays at the stronghold, so that companion does not enter a range and does not keep the battle going. A new squad starts with every box empty. Sam, Alex, and the rest of the company stay unselected until the player puts them in the squad.
- Damage in the retreat rule is the Power taken off Strength.
