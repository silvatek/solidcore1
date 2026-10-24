# Leave a fight

Status: New

## Story

This story depends on [Basic combat](Done/basic-combat.md). That story ships first. It does not end a fight by walking away, and a retreat plan does not move anyone.

The Iglin bridge fight is the one in play now. That fight kills the captain. [Iglin Bridge Fight Enhancements](iglin-bridge-fight-enhancements.md) keeps that encounter going past EscapeDistance. This story is the later rule: walking out ends the fight, and a retreat plan moves the fighter back.

This story introduces no game concept. EscapeDistance is a fact on the monster, as DetectionRange already is.

## Acceptance

- Each monster has EscapeDistance. It starts at 5 m, separate from DetectionRange. Play-testing may change it on its own. The starting figure is 5 m.
- The battle ends when the captain and every companion in the squad are outside every living monster's EscapeDistance. The attack cycle and the retreat stop. Entering a DetectionRange again starts the battle again.
- While the captain is alive, one living companion still inside an EscapeDistance keeps the battle going.
- A monster moves no farther than its EscapeDistance from its spawn point. That point is the place [Basic monsters](Done/basic-monsters.md) put it.
- A plan that stores retreat moves that captain, companion, or monster back from enemies. The 75% reduction already landed in [Basic combat](Done/basic-combat.md) stays as it is.

## Implementation notes

- EscapeDistance starts at 5 m, which is 500 cm. Distance is horizontal. A distance beyond EscapeDistance is outside it.
- The Iglin bridge encounter does not use this ending. The three Small raptors keep fighting until the captain dies.
