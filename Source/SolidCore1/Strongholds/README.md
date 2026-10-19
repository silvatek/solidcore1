# Stronghold mockups

How agents draw tower, fortress, and dungeon plans before those plans land in a story or in C++.

## Deliverable

Use a **Cursor canvas** (not an in-transcript HTML fragment, and not a repo file). The fortress and Imperious Tower plans were canvases the owner opened beside the chat. Keep one canvas per stronghold type while iterating; update it in place as the owner steers.

When the owner likes the plan, **write the learnings into the open Planning story** (hex splits, interior sizes, stairs, elevations, what is not chosen). That story commit uses the `Plan: ` prefix. The canvas itself is not committed.

Canvas colours are for reading the plan only (for example light walkable floor and accent hexes on a dungeon mockup). They are **not** the in-game look. Shipped interiors use stone, as the Meagre cave and the towers already do.

## Shared constants

Match the shipped grid and the tower plan. Do not invent a second hex size.

| Constant | Value | Source |
|----------|-------|--------|
| Flat to flat | 120 cm | `SolidStrongholdGrid::FlatToFlatCm` |
| Pointy-top radius | `FlatToFlat / √3` | `SideCm()` |
| Row step | `1.5 ×` radius | `RowStepCm()` |
| Wall thickness | 50 cm | `FPlan::WallCm` |
| Stair clear width | 140 cm | `FPlan::StairWidthCm` |
| Landing run | 200 cm | `LandingRunCm` |
| Storey pitch | 300 cm | `FPlan::FloorPitchCm` |
| Stair tread depth (mockup) | 26 cm | tower tread |

Odd rows shift half a flat-to-flat along X, the same as `HexCenter`.

## Room kit (settled)

Three room sizes: **Large** = 20 hexes, **Medium** = 12, **Small** = 6.

A size has one kit. Dungeon, Fortress, and Tower at that size use the **same number and sizes of rooms**. Larger sizes add more cells, stepping relatively smoothly. Passages and yards hold no usable hexes.

| Size | Large (20) | Medium (12) | Small (6) | Hex total |
|------|-----------:|------------:|----------:|----------:|
| Meagre | 0 | 1 | 0 | 12 |
| Humble | 0 | 1 | 1 | 18 |
| Average | 0 | 1 | 2 | 24 |
| Impressive | 0 | 3 | 2 | 48 |
| Grand | 1 | 4 | 2 | 80 |
| Stately | 2 | 4 | 2 | 100 |
| Imperious | 3 | 4 | 3 | 126 |

The shipped Meagre dungeon is still the fourteen-cell cave. The Meagre kit row (one medium) is the shared target when a type at that size uses rooms; it does not change the cave until a story does.

### Tower floors

Tower rooms fit in a **constant tower geometry**: a contiguous band of floors taken from the Imperious tower (floors numbered **1 at the ground** through **10 at the top**). Each floor is one room. The Imperious kept-hex counts per floor (`35, 28, 22, 19, 16, 12, 12, 8, 6, 7` on floors 1–10) are larger than the room sizes; the room is the usable kit cell count that fits that floor.

| Size | Floors (inclusive) | Rooms |
|------|--------------------|-------|
| Average | 7–9 | 1× medium, 2× small |
| Impressive | 5–9 | 3× medium, 2× small |
| Grand | 3–9 | 1× large, 4× medium, 2× small |
| Stately | 2–9 | 2× large, 4× medium, 2× small |
| Imperious | 1–10 | 3× large, 4× medium, 3× small |

Meagre and Humble have no tower. Floor 1–3 are large, 4–7 medium, 8–10 small.

### Fortress keep

Every fortress has a large **Keep** as its main building. Keep floors are kit rooms stacked in that building:

| Size | Keep floors | Keep hexes | Remainder (other buildings) |
|------|-------------|----------:|-----------------------------|
| Humble | 1× medium | 12 | 1× small |
| Average | 1× medium | 12 | 2× small |
| Impressive | 2× medium | 24 | 1× medium, 2× small |
| Grand | 1× large, 2× medium | 44 | 2× medium, 2× small |
| Stately | 2× large, 1× medium | 52 | 3× medium, 2× small |
| Imperious | 3× large | 60 | 4× medium, 3× small |

Meagre is not a fortress size. How the remainder is split across Barracks, Gatehouse, Workshops, and Stores is a fortress layout choice; the keep column above is fixed.

### Dungeon

A dungeon joins the same kit with **passages** and **stairs**. Layout is flexible. **Level 1 is the top** (the mouth); deeper levels have higher numbers — the opposite of tower and fortress floor numbers.

## Usable hex rule

Straight walls (or the tower's interior circle). A hex counts only when **all six vertices** lie inside the inner face.

Exclude a hex whose **center** lies on a stair flight or on a landing. Same idea as `HexOnStair` / `HexOnLanding` on a tower. A one-floor room has no stair and no landing.

Embed that count in the canvas so the stats stay honest when a room moves. Python in the shell is fine for searching dimensions that hit a target count before drawing.

## What to draw

1. **Plan** — floor plan on the shared grid, with walls, usable hexes, stairs, landings, and entrance (mouth, door, or gate). North up unless the story says otherwise.
2. **Size switcher** — pills or tabs when more than one size is drawn. Floor switcher when a plan has storeys.
3. **Outside views** when the exterior matters — elevation and a simple oblique, using the tower pitch for storey height so heights stay comparable.
4. **Table** — per room or building: floors, hexes each floor, total, and how it meets the size budget.

## Loop with the owner

1. Sketch the roster (which rooms or buildings, how many floors, rough hex split that sums to the size total).
2. Find straight interiors (or irregular outlines) on the grid that produce those counts.
3. Draw the canvas. Iterate from owner feedback (walls, stairs, placement, outside views).
4. Record settled numbers and rules in the Planning story. Leave undrawn sizes undrawn; do not invent their plans unless asked.

## Type cues

- **Tower** — one round shaft, taper, spiral stair band. Usable rooms are the kit sizes on the Imperious floor band for that size (table above). Average through Imperious only.
- **Fortress** — rectangular outer wall, yard (yard hexes are **not** in the total), buildings with straight walls. Keep is the main building and uses the keep floors above. Stairs and landings live inside multi-floor buildings.
- **Dungeon** — the shared rooms joined by **passages** and **stairs**. Passages have no usable hexes and are at least as long as they are wide. No hard limit on the overall footprint. Rooms may be rectangular or irregular so long as the hex counts match the kit. **Level 1 is the top** (the mouth); deeper levels have higher numbers.

## Precedents

- Tower plan canvas: Imperious Tower plan (ground / mid / top floors, elevation, kept-hex table).
- Fortress plan canvas: Fortress mockups (Humble, Impressive, Imperious; plan, elevations, south-east oblique). Older per-building hex counts there predate the room kit; new fortress plans follow the keep and remainder tables above.
- Dungeon / shared kit canvas: Imperious room kit with a dungeon layout.
