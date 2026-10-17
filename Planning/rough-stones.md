# Rough stones

Status: Ready

## Story

The player walks ground that is no longer in full fog, beside a river or a mountain, and finds rough stones scattered there. A pile is one rough stone. It is a foraging pile, so it is the same shape as a stick or a pitch, and the player picks it up the same way. It does not lie in the channel, on the mountain, or in the sea.

## Acceptance

- Rough stone is a catalog material. The page name is Rough stone. It is Inferior and stacks to 10.
- A pile is one rough stone, lying on open-world ground.
- The pile's terrain point is walkable. Its biome is not Mountain, River, Sea, Road, or Town.
- One of the eight neighboring terrain points, a step of 2 m away, is Mountain or River. A point with no such neighbor has no rough stone.
- The channel and the mountain itself have none. The sea has none.
- A point that has a tree has none. A point that already has a stick or a pitch has none.
- A point holds at most one rough stone.
- The piles are a scatter of those points, not every one. At most 64 are placed.
- Fog, the click, the pack, the cube, and a trip through a stronghold or a lair follow [Foraging](foraging.md). This story does not change those rules.
- The taken indexes are this scatter's indexes, kept apart from the forest pile indexes. Taking a rough stone does not remove a stick or a pitch.
- The dungeon, the tower, and the Iglin tunnels have none.
- This story adds no recipe.

## Implementation notes

- The id is `e7b31a52`. Kind is Material, quality is Inferior, and the stack limit is 10. Add it to `SolidItemType::Catalog`. `MaxItems` is 16, and the catalog has room. The suite line in `README.md` that lists the catalog items gains rough stone in the same commit.
- [Foraging](foraging.md) is the pile, the cube, the click, the pack insert, and the fog test. The mesh function returns that same cube for this id. This story does not give rough stone its own mesh.
- A neighbor is one of the eight points at one grid step, 200 cm. A diagonal step counts. The point's stored biome is the test, the value `SampleBiome` already wrote. River means the 4 m channel, not the whole `*` cell. The dry bank inside that cell can hold a pile when its biome is allowed and a channel point is a neighbor.
- Mountain takes the whole cell, so the piles sit on the points just outside it.
- Town is excluded, so a bank that took a town biome has none. Rathen sits on such a bank. Road is excluded the way a road through forest has no sticks.
- Skip a point chosen for a tree, and a point chosen for a stick or a pitch, before this scatter. Use the point index, before jitter.
- Gather them the way the forest piles are gathered. Density is `0.05`. The max is 64. Jitter is 90 cm. The seed is 44, so a change to the forest seed 43 does not move these. A test set of 40 eligible points places two piles. A map with no river and no mountain places none.
- If the jittered spot is impassable, leave the pile on the terrain point. `SolidGroundMove::IsImpassable` is sea, mountain, and the channel.
- Store these taken indexes on `FGameState` in their own list. The forest list stays the forest indexes. `SolidInventory::Clear` still does not put either list back.
- No recipe. The torch recipe stays one stick and one pitch.
