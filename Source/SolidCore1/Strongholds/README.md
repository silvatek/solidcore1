# Stronghold mockups

How agents draw tower, fortress, and dungeon plans before those plans land in a story or in C++.

## Deliverable

Use a **Cursor canvas** (not an in-transcript HTML fragment, and not a repo file). The fortress and Imperious Tower plans were canvases the owner opened beside the chat. Keep one canvas per stronghold type while iterating; update it in place as the owner steers.

When the owner likes the plan, **write the learnings into the open Planning story** (hex splits, interior sizes, stairs, elevations, what is not chosen). That story commit uses the `Plan: ` prefix. The canvas itself is not committed.

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

## Hex total for a size

A size has one room kit. Dungeon, Fortress, and Tower at that size use the **same number and sizes of rooms**. The tower is the constraint: its kept-hex floors set the bands. Rounding those bands down to a few fixed room sizes can cut the overall hex count below the old per-type totals; that is fine.

### Imperious kit (in progress)

From the Imperious Tower kept hexes per floor (`35, 28, 22, 19, 16, 12, 12, 8, 6, 7`):

| Band | Tower floors | Room size | Count |
|------|--------------|-----------|------:|
| Large | 0–2 | 20 hexes | 3 |
| Medium | 3–6 | 12 hexes | 4 |
| Small | 7–9 | 6 hexes | 3 |

Kit total **126** (tower kept total was 165). How the rooms are arranged differs by type (tower shaft, fortress yard, dungeon passages). Passages and yards hold no usable hexes.

- **Meagre** — still the shipped fourteen-cell cave until a smaller size adopts the kit.
- Smaller sizes (Humble through Stately) — not chosen yet; same idea: take the tower floors for that size and band them into large / medium / small rooms.

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

- **Tower** — one round shaft, taper, spiral stair band, kept hexes inside the interior radius. Imperious is the large end; Average through Stately step with `SizeAlong`.
- **Fortress** — rectangular outer wall, yard (yard hexes are **not** in the total), buildings with straight walls. Keep is largest footprint and tallest. Stairs and landings live inside multi-floor buildings.
- **Dungeon** — the shared rooms joined by **passages** and **stairs**. Passages have no usable hexes and are at least as long as they are wide. No hard limit on the overall footprint. Rooms may be rectangular or irregular so long as the hex counts match the kit. **Level 1 is the top** (the mouth); deeper levels have higher numbers. That is the opposite of a tower or fortress, where floor numbers rise as you climb.

## Precedents

- Tower plan canvas: Imperious Tower plan (ground / mid / top floors, elevation, kept-hex table).
- Fortress plan canvas: Fortress mockups (Humble, Impressive, Imperious; plan, elevations, south-east oblique).
- Dungeon / shared kit canvas: Imperious room kit (3×20, 4×12, 3×6) with a dungeon layout.
