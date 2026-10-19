# Tower stronghold by size

Status: Ready

## Story

Do not start this story until [Stronghold type classes](Done/stronghold-type-classes.md) is Done.

The player visits a tower whose floors and usable rooms follow **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Average through Imperious share one constant tower geometry: floors taken from the Imperious tower. Floors are numbered **1 at the ground** through **10 at the top**. Each floor is one kit room. Large rooms are 20 hexes, medium 12, small 6. Meagre and Humble have no tower.

| Size | Floors | Rooms | Hexes |
|------|--------|-------|------:|
| Average | 7–9 | 1× medium, 2× small | 24 |
| Impressive | 5–9 | 3× medium, 2× small | 48 |
| Grand | 3–9 | 1× large, 4× medium, 2× small | 80 |
| Stately | 2–9 | 2× large, 4× medium, 2× small | 100 |
| Imperious | 1–10 | 3× large, 4× medium, 3× small | 126 |

Floor 1–3 are large, 4–7 medium, 8–10 small. The Imperious kept-hex counts per floor (`35, 28, 22, 19, 16, 12, 12, 8, 6, 7`) are the geometry budget; the usable room on that floor is the kit size that fits inside it. Stairs, landings, and the stone rim outside the room still hold no usable hex.

A smaller tower is the contiguous band of Imperious floors for its size. Its ground floor is the lowest floor of that band. Its door faces the grass as today. Enter and leave stay a click on that door. Companions wait outside. The Meagre cave and its mouth stay as they are.

Dungeon and fortress at the same size use the same kit totals. [Fortress stronghold](fortress-stronghold.md) and [Imperious dungeon](imperious-dungeon.md) meet those totals with their own layouts. This story changes the tower only.

## Acceptance

- Tower `Supported` stays true for Average, Impressive, Grand, Stately, and Imperious, and false for Meagre and Humble.
- An Average tower has floors 7–9 of the Imperious geometry: one medium room and two small rooms, 24 usable hexes in all.
- An Impressive tower has floors 5–9: three medium and two small, 48 hexes.
- A Grand tower has floors 3–9: one large, four medium, and two small, 80 hexes.
- A Stately tower has floors 2–9: two large, four medium, and two small, 100 hexes.
- An Imperious tower has floors 1–10: three large, four medium, and three small, 126 hexes.
- Each floor has exactly one kit room. Usable hexes on that floor equal that room's size (20, 12, or 6), not the old full `CollectKeptHexes` count for the disc.
- Stairs and landings hold no usable hexes. A usable hex still needs all six vertices inside the inner face.
- The door is on the ground floor of that size's band and still faces the grass. A click on the door enters; a click inside leaves onto the grass outside that door. Companions stay outside.
- The Meagre dungeon mouth, interior, and closed-until-the-message rule are unchanged.
- Fortress and dungeon visits are unchanged by this story.
- This story adds no world-map `!` line.

## Implementation notes

- The plan of record is **Strongholds by size and type** in `Source/SolidCore1/Strongholds/README.md`. Do not invent a second kit or a second floor numbering.
- Today's tower uses `SizeAlong` to lerp floor counts and radii from Average (four floors) to Imperious (ten), and `CollectKeptHexes` keeps every hex inside the disc off the stair and landing. That kept total is larger than the kit (Imperious kept sum 165 vs kit 126). This story replaces that with the Imperious floor band and the kit room per floor.
- Keep the shared spiral stair, wall thickness, stair width, landing run, and floor pitch unless the band forces a change. The exterior still turns so the door faces the grass.
- A test for each supported size sums usable hexes across floors and equals the kit hex total in the table. A test per floor checks the room size for that Imperious floor index. Name any existing tests that assumed the old kept counts or the old `SizeAlong` floor counts, and update those tests in a prior commit when the policy requires it.
- [Fortress stronghold](fortress-stronghold.md) and [Imperious dungeon](imperious-dungeon.md) already target the same kit totals. They do not need this story to ship first for their acceptance, and this story does not implement them.
- Furnishings stay unset on tower floors. This story does not place the cave's fireplace, bed, or chest in the tower.
