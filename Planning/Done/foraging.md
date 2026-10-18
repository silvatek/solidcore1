# Foraging

Status: Done

## Story

The player walks through forest and finds sticks and pitch scattered on the ground. Those are the materials this story places. A pile is one stick or one pitch, on forest ground, because forest is where they occur. A stick and a pitch are the same simple shape. The name is what tells them apart. The player points at it, close to the captain, and the name shows. A click picks it up into the pack, and the pile is gone. A click the pack cannot hold leaves the pile where it is. A pile is seeded when that ground's fog falls below full. Ground that stays fully fogged never receives one. A pile already taken is still gone after the player leaves a stronghold or a lair and comes back.

## Acceptance

- A pile is one catalog item, with a count of one, lying on open-world ground.
- Stick and pitch are the piles this story places. No other catalog item is placed.
- A stick occurs on Forest. A pitch occurs on Forest. No other biome gets either.
- Its terrain point's biome is one that item is allowed to occur in. No other biome has that pile.
- Sea is not a biome an item occurs in.
- A forest point holds at most one pile. A forest point that has a tree holds no pile.
- The piles are a scatter of the remaining forest points, not every one. At most 64 are chosen.
- Within 100 m of Iglin, every forest cell on the world map has at least one stick and one pitch. The scatter's 64 does not limit those. A cell the scatter already filled is left as it is.
- When at least two are placed, both a stick and a pitch are among them. They alternate, and the first is a stick.
- Every pile uses the same simple mesh. A stick and a pitch are that same shape. The catalog name on the pointer is what differs.
- This story does not give a stick its own mesh, or a pitch its own. A later story does that, one mesh for each kind of pile.
- The dungeon, the tower, and the Iglin tunnels have no piles.
- A point whose `FSolidTerrainPoint.Fog` is `1.0f` is not seeded. The player cannot see that ground, and they may never go there.
- When that point's `Fog` falls below `1.0f`, a chosen pile on it is seeded then. Until then, the pile is not there.
- A point that stays at `1.0f` never receives a pile.
- The pointer shows the item's catalog name when it is over a drawn pile and the captain is within 4 m of it. Farther away, that pile has no name on the pointer.
- A click within 4 m takes the pile. The item joins the pack, and the pile is gone.
- When the pointer ray meets more than one pile, the nearest one is the pile named, and the pile a click takes.
- In raven sight, that click also stops a walk already under way.
- The pack already holding that item below its stack limit takes the one onto that stack. Otherwise it goes in the first empty pack cell.
- A stick stacks to 10. Pitch stacks to 10. A full stack does not take another. The next empty cell does.
- When every stack of that item is full and no pack cell is empty, the click changes nothing. The pile stays.
- Worn boxes, crafting slots, and the cave chest stay as they were.
- The pickup writes no journal line.
- In raven sight, a click on a pile within 4 m does not start a walk. A click on that pile from farther away walks, as a ground click does now.
- In true sight, a click within 4 m picks the pile up. A farther click does nothing.
- A passage under the pointer still wins. That click enters or leaves, and the pile stays.
- A click while the pointer is hidden, or while a menu, the inventory, a message, or the squad builder is open, does not take a pile.
- After the player enters and leaves a stronghold, or enters and leaves the Iglin tunnels, a taken pile is still gone and an untaken pile is still there. The pack still holds what was picked up. A taken pile is remembered by its index in the scatter list, so that same slot stays empty.
- A new game has no pile taken. The pack is the starting pack.
- Closing the monster message still clears the pack down to Rags on the body. It does not put taken piles back on the ground.
- This story does not add rough stones.

## Implementation notes

- Stick is `6c2a91e8`, Inferior, stacks to 10. Pitch is `d4f07b3a`, Common, stacks to 10. Coins are not a ground find. Rough stone is [Rough stones](../rough-stones.md). This story does not add that id and does not place those piles.
- Scatter the piles the way `SolidForestTrees::CollectSpawnPositions` scatters trees. The input set is forest points that did not receive a tree. Fisher–Yates, then `round(count * density)` clamped to the max and to the count. Density is `0.05`. The max is 64. Jitter is 90 cm, the same as `ForestTreeJitterCm`. Trees use `ForestTreeSeed` 42. The piles use their own seed, 43, so a tree-seed change does not move them.
- Walking that chosen list, an even index is a stick and an odd index is a pitch. The list is the choice of points. It is not a spawn of actors on ground the player has not reached. A test map with 40 treeless forest points chooses two piles, stick then pitch. A map with no forest chooses none. One placed stick is still enough to test the click and the pack.
- After that scatter, walk world-map forest cells (`#`) whose center is within 100 m of Iglin. A cell missing a stick or a pitch gets one, on the lowest-index treeless forest point in that cell that does not already hold a pile. No jitter, so the pile stays on that point. A point with a tree still holds no pile.
- `SolidInventory::PlacePack` writes a cell and replaces what was there. It does not merge. Add an insert that fills the first same-id stack under `FSolidItemType::MaxStacked`, then the first empty cell, and returns false when neither exists.
- The pack is `FGameState::Inventory`, six cells, carried on the travel session. The taken set is the indexes in the chosen scatter list, and it rides on that state too. `OpenLevel` builds a new world. The same list comes back, and a taken index is skipped.
- A pickup calls `ClearClickMove`, the same way `ActivatePassage` does, so a raven walk already under way stops. The nearest pile is the first hit along the pointer ray. The 4 m test still applies to that pile.
- `SolidInventory::Clear` empties the pack and leaves Rags on the body. Leave the taken ids taken.
- Full fog is `FSolidTerrainPoint.Fog` of `1.0f`. Bands are in [Source/SolidCore1/Fog/README.md](../../Source/SolidCore1/Fog/README.md). Do not seed a chosen point while its `Fog` is `1.0f`. `CenterExplorationFogOn` opens the first bubble, and `ApplyExplorationFogAround` lowers fog as the pawn moves and never raises it. After each of those, seed chosen points whose `Fog` is now below `1.0f`, that are not taken, and that have no pile yet. A test with every point at `1.0f` seeds nothing. Lowering one chosen point below `1.0f` seeds that pile and leaves the others unseeded. Trees and buildings use the same fog value in [Hide trees and buildings in full fog](../hide-props-in-full-fog.md).
- A left click already prefers a passage (`PassageUnderPointer`), then `ClickMoveFromInput`. A pile in reach sits between those two. `SolidClickMove::AllowsClick` is raven sight only. `IsMenuOverlayOpen` includes the inventory, the journal, the message, and the squad builder. The passage ray is `SolidClickMove::MaxRayCm` (800 m). The 4 m test is the captain's ground distance to the pile, not that ray.
- Terrain points are 2 m apart (`PointSpacing` 200 cm). 4 m is two of those, so a pile on a neighbouring point is in reach.
- The tip is the catalog name, drawn the way `CavePassageTip` is drawn. Empty when the pointer is not over a pile in reach. The pointer has to be showing, the same check as `SolidDungeonPassage::AcceptsPointer`.
- The placeholder is one Engine cube, `/Engine/BasicShapes/Cube.Cube`, about 40 cm on a side, sitting on the ground, in one dull color. The same component draws every pile. One function of the item id picks the mesh. This story returns the cube for every id, so a later story can swap in a mesh per type without changing the pickup. Do not load a Fab mesh here.
- The cube is a query hit for the pointer. It does not block the captain's walk.
- The point's own biome is the test. A road cell is Road, not the biome the road replaced. A road through forest does not keep the sticks.
- No piles on `?SolidLevel=Dungeon`, `Tower`, or the lair.
