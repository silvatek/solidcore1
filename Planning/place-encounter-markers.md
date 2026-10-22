# Place encounter markers

Status: New

## Story

This story follows [Basic monsters](basic-monsters.md), which is In Progress. That story ships first. The developer then places an encounter marker in the World chart mode. The marker names an encounter the catalog already lists. On the open world, that encounter stands at the marker. A new encounter is still a row in the catalog. This story introduces no game concept. Encounter is already `FSolidEncounter`. The marker is a mark on the chart.

## Acceptance

- The World chart mode's Place list includes Encounter, with Town, Stronghold, Stronghold site, Lair entrance, Lair exit, and Custom.
- Encounter asks for a name from `SolidEncounter::Definitions`. The shipped name is Iglin Bridge. A click puts that mark on the cell. The position is the cell centre, in centimetres.
- One encounter name has one mark. Placing it on another cell moves it. Placing it on a cell that already has a mark replaces that mark.
- Delete clears the encounter mark. Paint leaves it where it is. Undo returns the chart to the edit before this one. Save writes `/Game/World/WorldChart`.
- The painted biome stays as it is. An encounter is not a road end. The world map leaves the cell as painted. It does not become a town, a stronghold, a site, or a lair mouth.
- Read, on hover, names the mark as an encounter and gives its name. The mode shows that name on the cell, as it shows any other mark's name.
- The shipped chart has one encounter mark, Iglin Bridge. Its cell is the grassland cell whose centre is nearest the outer end of the Iglin ramp. The outer end is the ramp sample farthest from the deck. The Iglin ramp is the ramp whose outer end is closer to the Iglin town cell.
- On the open world, Iglin Bridge stands around that mark. The focus is the mark's position. The three Small raptors are the nearest eligible points to that focus. A point is grassland, walkable, and clear of a mountain quad. It lies between 8 m and 40 m from the mark. A later point is at least 4 m from one already chosen. A tree point has none. A foraging point has none. They face the mark. Yaw only. Fewer than three eligible points places as many as there are.
- A definition with no mark on the chart places nobody. Repairing the Iglin bridge leaves the three on the points the mark chose.
- The mesh, the idle, the capsule, the click, the fog seeding, and the empty tunnels stay as [Basic monsters](basic-monsters.md) says. A closed visit has no encounters.
- `Content/WorldMap.txt` has no encounter line. The mode reports a disagreement for every other difference, and an encounter mark on its own leaves that line empty. Importing the text file keeps an encounter mark on a cell the import left empty.
- Tests place, move, and delete an encounter mark, and place the members from that mark, without a viewport. The editor session on the Windows machine is the check that the mode matches. A cloud agent does not run the editor.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- The mode is `FSolidWorldChartEdMode`, shown as World chart. `PlaceEncounter` sits with `PlaceTown` and `PlaceCustom` on `USolidWorldChart`. The editor calls it. The tests call it.
- Add `Encounter` to `ESolidWorldChartMarkKind`, and add that row to the Place combo at the same index. The combo index is the enum value. A name the catalog does not list is refused. The editor asks for the name only. Kind, size, and count stay on the definition.
- The focus moves from the ramp sample to that cell centre. The cell is the grassland cell nearest the sample, so the three stay on the Iglin side of the bridge.
- `RememberChartMark` skips an encounter mark the way it skips a custom note. `LoadFromChart` keeps the encounter name and its position in centimetres. `LoadFromString` has none.
- `ESite` leaves. Iglin Bridge no longer names `IglinBridgeRamp`. `Place` takes the mark position as the focus. The Iglin town point and the bridge list leave that call. The band, the separation, the cliff check, and the tree and forage skips stay. The `SolidCore1.Monster.*` line that says a missing Iglin bridge places none becomes: a missing mark places none.
- The shipped mark is saved in the editor on the Windows machine and committed with `/Game/World/WorldChart`. A test loads that asset and checks the mark against the grassland cell nearest the ramp outer end.
- Importing `WorldMap.txt` rebuilds cells, towns, strongholds, sites, lair mouths, and roads, then puts back each encounter mark whose cell has no imported mark. A cell the import filled drops that encounter mark.
- Disagreement compares the chart to the text file with encounter marks set aside. [Remove WorldMap.txt](remove-world-map-txt.md) is Ready. Once that story has shipped, there is no disagreement line and no import to keep.
- The encounter billboard is amber, `FLinearColor(0.95f, 0.72f, 0.15f)`.
- Encounter-mark cases live in `SolidCore1.WorldChart.*`. The focus-from-mark case lives in `SolidCore1.Monster.*`. The product commit updates those lines in `Source/SolidCore1/Tests/README.md`. Add `SolidCore1.WorldChart.*` to that table if it is still missing.
