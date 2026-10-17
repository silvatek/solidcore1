# Rough stones

Status: Ready

## Story

The player walks beside a river or a mountain and finds rough stones scattered there. A pile is one rough stone. It is a foraging pile, so it is the same shape as a stick or a pitch, and the player picks it up the same way. It does not lie in the channel, on the mountain, or in the sea.

The player puts a rusty sword and a rough stone into the crafting slots and presses Craft. Those two are spent, and the pack gains one cheap sword. When the slots can make more than one thing, a warning sits under the slots and the button stays dim.

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
- Cheap sword is a catalog weapon. The page name is Cheap sword. It is Common and stacks to 1.
- The recipe is named Cheap sword. It is known. It takes one rusty sword and one rough stone, and it makes one cheap sword.
- The Craft button spends those two and puts the cheap sword in the first empty pack cell. A stack that reaches zero is cleared. The rusty sword slot clears, because that sword stacks to 1. A rough-stone stack of two or more loses one.
- The button stays dim when those inputs are short, or when the pack has no empty cell. The slots stay as they were.
- When the slots can make two or more known recipes, the line "More than one thing can be made." sits under the input slots, and the button stays dim. A press does nothing. The slots stay as they were.
- An extra item that does not finish a second recipe leaves the button lit, and that line stays hidden. Stick and pitch still make a torch. One rusty sword and one rough stone, with no second recipe filled, still make a cheap sword.
- Taking one ingredient out, so only one recipe fits, hides the line. The button lights when the pack has an empty cell.
- The cheap sword is not on the ground, and it is not in the cave chest at the start. The chest still starts with its rusty sword.

## Implementation notes

- Rough stone is `e7b31a52`. Kind is Material, quality is Inferior, and the stack limit is 10. Cheap sword is `3c8f41b6`. Kind is Weapon, quality is Common, and the stack limit is 1. Add both to `SolidItemType::Catalog`. `MaxItems` is 16, and the catalog has room. The suite line in `README.md` that lists the catalog items gains both in the same commit. The crafting line gains this recipe in that same commit.
- [Foraging](foraging.md) is the pile, the cube, the click, the pack insert, and the fog test. The mesh function returns that same cube for this id. This story does not give rough stone its own mesh.
- A neighbor is one of the eight points at one grid step, 200 cm. A diagonal step counts. The point's stored biome is the test, the value `SampleBiome` already wrote. River means the 4 m channel, not the whole `*` cell. The dry bank inside that cell can hold a pile when its biome is allowed and a channel point is a neighbor.
- Mountain takes the whole cell, so the piles sit on the points just outside it.
- Town is excluded, so a bank that took a town biome has none. Rathen sits on such a bank. Road is excluded the way a road through forest has no sticks.
- Skip a point chosen for a tree, and a point chosen for a stick or a pitch, before this scatter. Use the point index, before jitter.
- Gather them the way the forest piles are gathered. Density is `0.05`. The max is 64. Jitter is 90 cm. The seed is 44, so a change to the forest seed 43 does not move these. A test set of 40 eligible points places two piles. A map with no river and no mountain places none.
- If the jittered spot is impassable, leave the pile on the terrain point. `SolidGroundMove::IsImpassable` is sea, mountain, and the channel.
- Store these taken indexes on `FGameState` in their own list. The forest list stays the forest indexes. `SolidInventory::Clear` still does not put either list back.
- The recipe is the second entry in `SolidCrafting::Recipes`, after Torch. `MaxRecipes` is 16. `CraftButtonEnabled` is true only when exactly one known recipe fits and the pack has an empty cell. `Craft` uses that same test, so a second match cannot spend the slots. The warning is that string, in the hint ink, left-aligned with the first crafting slot and below the row. It is hidden when fewer than two recipes fit, and the page is the height it is now. While it shows, the board grows to fit the line. The button stays beside the slots.
- A test that fills the slots with one rusty sword and one rough stone, and leaves an empty pack cell, gets one cheap sword and empty slots. A test that also leaves stick and pitch in the other two slots sees the warning and a dim button, and `Craft` changes nothing. Removing the rough stone hides the warning and lights the button. `SolidCore1.Inventory.CraftButton` still passes: the rusty sword beside stick and pitch does not block the torch. The existing torch tests keep passing.
