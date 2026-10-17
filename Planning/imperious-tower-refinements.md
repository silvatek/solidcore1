# Imperious Tower refinements

## Story

The player climbs the Imperious Tower. Where a flight reaches the next floor, a short flat landing sits in the stair band before the next flight starts up. The top floor has that landing and no flight above it. The ground floor still starts its flight with no landing of its own.

A floor number is fixed to the wall beside each landing. The ground floor is floor 1. The first landing reads 2. The top landing of this tower reads 10.

A handrail runs along the inner edge of each climbing flight, and another runs along the open edge of each arrival hole. Both rails stop at the landing, so the player can step off the stair onto the floor.

The yellow hex grid on each floor is shifted or rotated a little, so the cells sit better in the circle. The Meagre den's grid does not move.

## Acceptance

- Each floor the stairs arrive on has a flat landing in the stair band, about one metre along the centerline, at that floor's walking surface. The next flight, when there is one, starts at the far end of that landing.
- The ground floor has the first flight and no landing. The top floor has a landing and no flight up.
- The arrival hole is still the annular sector of the flight below. The landing is floored. The middle of the disc stays floored.
- Beside each landing, the wall shows that floor's number. Ground is 1, so Imperious landings read 2 through 10. A smaller tower uses the same counting, and its top sign is its floor count.
- A rail follows the inner edge of each climbing flight and stops for the landing.
- A rail follows the floor edge of each arrival hole and stops for the landing.
- The rails block a step off the hole and off the inner edge of the flight. The landing stays open onto the floor.
- Each tower floor may shift or rotate its hex grid by a small amount. The Meagre den's fourteen hexes stay where they are.
- On the landed floor plan, a floor that shifts or rotates keeps at least as many hexes as that same floor keeps with the grid still centered and unrotated.
- F10 Enter cave still opens the Meagre den from the open world, and does nothing inside the tower. Leaving the tower still stands the captain on the grass outside its door.

## Implementation notes

- The shipped visit is `ASolidTowerInterior`, built from `SolidTowerStronghold::FPlan`. Imperious is the tower on cell 17,31. Average, Impressive, Grand, and Stately use the same actor. Build the landing, the rails, and the signs from the plan, so a smaller tower gets them too. Only Imperious is on the map.
- Today the flights join with no landing. Seventeen risers and sixteen treads of 26 cm climb one floor. The last tread stops one riser below the next floor. `FlightStartRad` starts each flight where the previous sweep ended. A landing is an extra arc, about 100 cm along the centerline, inserted before the next flight. `GapWedge` stays the arriving flight, not the landing.
- The hole is an annular sector. The inner disc stays solid. Treads sit on the flight. Do not cut the landing out of the floor.
- Rails are part of the interior, not furnishings. A simple rail about waist height is enough. No rail across the landing.
- Signs are text on the curved wall beside the landing, facing the room. Do not reuse `ASolidTownSign`. Ground is floor 1 even though the plan's floor index starts at 0.
- The hex grid is pointy-top, 1.2 m flat to flat, origin at the tower center, in `SolidTowerStronghold::CellCenter`. A per-floor offset or rotation belongs in that tower grid, not in `SolidStrongholdGrid`. Leave `MeagreOrigin` and the fourteen-cell den alone.
- Kept hexes are cells whose six vertices lie inside the interior circle and that are neither stair nor hole. `SolidTowerStrongholdTests` pins the counts, Imperious ground to top: 34, 29, 22, 21, 14, 13, 13, 10, 6, 5. Average, Impressive, Grand, and Stately have their own pins. A landing changes the wedges, and a grid nudge changes the counts again. Update those pins in the same commit as the geometry and say so in the commit. Do not delete the pins in an earlier commit.
- The yellow lines are `SolidStrongholdGrid::LineColor`. This story does not recolor them.
- Do not add a map marker. Do not change the shell, the door click, or the grass outside the door.
