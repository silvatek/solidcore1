# Gear left at the death

Status: New

## Story

This story depends on [Basic combat](Done/basic-combat.md). That story ships first.

When the captain dies, the wake clears the inventory to Rags and sends them to the company stronghold. This story keeps the gear that left that inventory. It stays on the ground where the captain died. The stronghold chest stays as it was.

This story introduces the death pile. It is the gear from that death, lying at that place.

## Acceptance

- The wake still puts Rags on the body and empties the pack, the crafting slots, and every other worn box.
- The items that left the captain stay on the ground at the place the captain died.
- The pile is those items. A death with only Rags on the body leaves no pile.
- The stronghold chest is not in the pile.
- The living captain is in the stronghold, wearing Rags. The pile is not on the captain.
- Taking an item from the pile puts it in the pack. The pile is gone when its last item is taken.
- A second death leaves its own pile. The first pile stays where it was.
- The cave, a tower, and the Iglin tunnels do not hold this pile. It is on the open-world ground.

## Implementation notes

- `SolidInventory::Clear` deletes the items today. Copy them before that clear. The death position is the captain's position when the fight's copy reaches 0, before the wake travels.
- A foraging pile is one material and one click. This pile holds the items from one death, so it is not that pile. The click can still put one item into the pack.
- [Basic combat](Done/basic-combat.md) fires Killed by monsters and clears the kit. This story does not change that event. It keeps the copy of what the clear removed.
