# Code structure

The stronghold type is a class that owns its measurements. A type would preferably not use a separate plan structure. The exterior actor (shell) and the interior actor (visit) hold a pointer to that stronghold.

`Strongholds/<Type>/` holds the stronghold class. `Strongholds/<Type>/Actors/` holds the exterior actor (shell) and the interior actor (visit). Shared stronghold bases stay in `Strongholds/Actors/`. The furnishing catalog stays in `Strongholds/Furnishings/`. Furniture actors stay in `Strongholds/Furnishings/Actors/`.

The class is the placed stronghold. Its fields are the measurements. Values derived from those fields are methods. Public methods cover what that stronghold does. Details only the placed instance needs stay private. Configure points the exterior actor (shell) and the interior actor (visit) at this instance.

The actors hold a pointer to the stronghold. It stays null until Configure, so the first construction draws nothing. The game state owns the stronghold. The exterior actor (shell) and the interior actor (visit) ask the stronghold for the measurements they draw.

## Shared constants

The shipped grid and the stair measurements. Do not invent a second hex size.

| Constant | Value | Source |
|----------|-------|--------|
| Flat to flat | 120 cm | `SolidStrongholdGrid::FlatToFlatCm` |
| Pointy-top radius | `FlatToFlat / √3` | `SideCm()` |
| Row step | `1.5 ×` radius | `RowStepCm()` |
| Wall thickness | 50 cm | `FPlan::WallCm` |
| Stair clear width | 140 cm | `FPlan::StairWidthCm` |
| Landing run | 200 cm | `LandingRunCm` |
| Storey pitch | 300 cm | `FPlan::FloorPitchCm` |
| Stair tread depth | 26 cm | `FPlan::StairTreadCm` |

Odd rows shift half a flat-to-flat along X, the same as `HexCenter`.

# Stronghold Details

## Strongholds by size and type

Three room sizes: **Large** (L) = 20 hexes, **Medium** (M) = 12, **Small** (S) = 6.

A size has one kit. Dungeon, Fortress, and Tower at that size use the **same number and sizes of rooms**. Larger sizes add more cells, stepping relatively smoothly. Passages and yards hold no usable hexes.

| Size | L | M | S | Hexes | Tower floors | Keep |
|------|--:|--:|--:|------:|--------------|------|
| Meagre | 0 | 1 | 0 | 12 | — | — |
| Humble | 0 | 1 | 1 | 18 | — | S, then M |
| Average | 0 | 1 | 2 | 24 | 7–9 | S, then M |
| Impressive | 0 | 3 | 2 | 48 | 5–9 | 2× M |
| Grand | 1 | 4 | 2 | 80 | 3–9 | 1× L, 2× M |
| Stately | 2 | 4 | 2 | 100 | 2–9 | 2× L, 1× M |
| Imperious | 3 | 4 | 3 | 126 | 1–10 | 3× L |

The shipped Meagre dungeon is still the fourteen-cell cave. The Meagre kit row (one medium) is the shared target when a type at that size uses rooms; it does not change the cave until a story does.

Tower rooms fit in a **constant tower geometry**: a contiguous band of floors taken from the Imperious tower (floors numbered **1 at the ground** through **10 at the top**). Each floor is one room. The Imperious kept-hex counts per floor (`35, 28, 22, 19, 16, 12, 12, 8, 6, 7` on floors 1–10) are larger than the room sizes; the room is the usable kit cell count that fits that floor. Floor 1–3 are large, 4–7 medium, 8–10 small. Meagre and Humble have no tower.

Every fortress has a **Keep** as its main building, and at least one building with at least two floors. Keep floors are kit rooms stacked in that building, ground floor first (column above). Humble and Average keep a small room on the ground and a medium room above it. Meagre is not a fortress size. The remainder of the kit (kit rooms minus Keep) goes to other buildings; how that split lands across Barracks, Gatehouse, Workshops, and Stores is a fortress layout choice.

A dungeon joins the same kit with **passages** and **stairs**. Layout is flexible. **Level 1 is the top** (the mouth); deeper levels have higher numbers — the opposite of tower and fortress floor numbers.

## Usable hex rule

Straight walls (or the tower's interior circle). A hex counts only when **all six vertices** lie inside the inner face.

Exclude a hex whose **center** lies on a stair flight or on a landing. Same idea as `HexOnStair` / `HexOnLanding` on a tower. A one-floor room has no stair and no landing.

## Type cues

- **Tower** — one round shaft, taper, spiral stair band. Usable rooms are the kit sizes on the Imperious floor band for that size (table above). Average through Imperious only.
- **Fortress** — rectangular outer wall, yard (yard hexes are **not** in the total), buildings with straight walls. Keep is the main building and uses the keep floors above. Stairs and landings live inside multi-floor buildings.
- **Dungeon** — the shared rooms joined by **passages** and **stairs**. Passages have no usable hexes and are at least as long as they are wide. No hard limit on the overall footprint. Rooms may be rectangular or irregular so long as the hex counts match the kit. **Level 1 is the top** (the mouth); deeper levels have higher numbers.

# Stronghold Mockups

How agents draw tower, fortress, and dungeon plans before those plans land in a story or in C++.

## Deliverable

Use a **Cursor canvas** (not an in-transcript HTML fragment, and not a repo file). The fortress and Imperious Tower plans were canvases the owner opened beside the chat. Keep one canvas per stronghold type while iterating; update it in place as the owner steers.

When the owner likes the plan, **write the learnings into the open Planning story** (hex splits, interior sizes, stairs, elevations, what is not chosen). That story commit uses the `Plan: ` prefix. The canvas itself is not committed.

Canvas colours are for reading the plan only (for example light walkable floor and accent hexes on a dungeon mockup). They are **not** the in-game look. Shipped interiors use stone, as the Meagre cave and the towers already do.

## What to draw

1. **Plan** — floor plan on the shared grid, with walls, usable hexes, stairs, landings, and entrance (mouth, door, or gate). North up unless the story says otherwise.
2. **Size switcher** — pills or tabs when more than one size is drawn. Floor switcher when a plan has storeys.
3. **Outside views** when the exterior matters — elevation and a simple oblique, using the tower pitch for storey height so heights stay comparable.
4. **Table** — per room or building: floors, hexes each floor, total, and how it meets the size budget.

Embed the usable-hex count in the canvas so the stats stay honest when a room moves. Python in the shell is fine for searching dimensions that hit a target count before drawing.

## Loop with the owner

1. Sketch the roster (which rooms or buildings, how many floors, rough hex split that sums to the size total).
2. Find straight interiors (or irregular outlines) on the grid that produce those counts.
3. Draw the canvas. Iterate from owner feedback (walls, stairs, placement, outside views).
4. Record settled numbers and rules in the Planning story. Leave undrawn sizes undrawn; do not invent their plans unless asked.

## Precedents

- Tower plan canvas: Imperious Tower plan (ground / mid / top floors, elevation, kept-hex table).
- Fortress plan canvas: Fortress mockups (Humble, Impressive, Imperious; plan, elevations, south-east oblique). Older per-building hex counts there predate the room kit; new fortress plans follow the keep and remainder tables above.
- Dungeon / shared kit canvas: Imperious room kit with a dungeon layout.
