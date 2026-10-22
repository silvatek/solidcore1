# Place encounter markers

Status: New

## Story

This story follows [Basic monsters](basic-monsters.md), which is In Progress. That story ships first. The World chart mode can place an encounter marker. The marker names an encounter the catalog already lists. Each marker is one encounter in the open world, standing at that marker. A catalog encounter with no marker is absent from the world. It remains in the catalog. A new encounter is still a row in the catalog. A road joins any two marks on the chart. This story introduces no game concept. Encounter is already `FSolidEncounter`. The marker is a mark on the chart.

## Acceptance

- The World chart mode's Place list includes Encounter, with Town, Stronghold, Stronghold site, Lair entrance, Lair exit, and Custom.
- Encounter offers the names in `SolidEncounter::Definitions`. A click stores the chosen name on that cell. The position is the cell centre, in centimetres.
- A cell holds one mark. An encounter mark replaces the mark already on that cell. Delete clears the encounter mark. Paint leaves it where it is. Undo returns the chart to the edit before this one. Save writes `/Game/World/WorldChart`.
- The painted biome stays as it is. The world map leaves the cell as painted. A road joins any two marks. A town, a stronghold, a stronghold site, a lair entrance, a lair exit, a custom note, and an encounter are ends. The end is the mark's name. The solved route includes every cell with a mark of that name. A road whose end names nothing on the chart is stored, and its route is empty. The tool names that missing end.
- Read, on hover, names the mark as an encounter and gives its name. The mode shows that name on the cell, as it shows any other mark's name.
- Every encounter mark on the chart is one encounter in the open world. The focus is the mark's position. The members are that definition's groups. A point is grassland, walkable, and clear of a mountain quad. It lies between that definition's near limit and far limit from the mark. A later point is at least that definition's separation from one already chosen. A tree point has none. A foraging point has none. They face the mark. Yaw only. Fewer eligible points than the group's count places as many as there are.
- Iglin Bridge remains three Small raptors, with a near limit of 8 m, a far limit of 40 m, and a separation of 4 m. Those figures stay on the definition.
- A definition with no mark on the chart has no encounter in the world. The definition stays in the catalog. Two marks with the same name are two of that encounter.
- The mesh, the idle, the capsule, the click, the fog seeding, and the empty tunnels stay as [Basic monsters](basic-monsters.md) says. A closed visit has no encounters.
- `Content/WorldMap.txt` has no encounter line, and its roads name towns and strongholds. The mode reports a disagreement for every other difference. An encounter mark on its own leaves that line empty. So does a road whose end is a stronghold site, a lair mouth, a custom note, or an encounter. Importing the text file keeps an encounter mark on a cell the import left empty, and keeps each of those roads.
- Tests place and delete an encounter mark, place the members from a mark the test added, place nobody from a definition the chart does not mark, and solve a road between two marks. The pairs include a site, a lair mouth, a custom note, and an encounter. A road whose end names nothing on the chart has an empty route. The tests run without a viewport. The editor session on the Windows machine is the check that the mode matches. A cloud agent does not run the editor.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

- The mode is `FSolidWorldChartEdMode`, shown as World chart. `PlaceEncounter` sits with `PlaceTown` and `PlaceCustom` on `USolidWorldChart`. The editor calls it. The tests call it.
- Add `Encounter` to `ESolidWorldChartMarkKind`, and add that row to the Place combo at the same index. The combo index is the enum value. A name the catalog does not list is refused. The list asks for the name only. Kind, size, and count stay on the definition.
- `RememberChartMark` skips an encounter mark the way it skips a custom note. `LoadFromChart` keeps each encounter name and its position in centimetres. `LoadFromString` has none.
- A road end is a mark's name. The comparison ignores case and spaces. `SolidRoad::Parse` keeps that name, and the chart resolves it to every mark of that name, whatever the kind. Both mouths of a lair carry the lair's name, so both cells are ends of that name. `RoadEndError` reports a missing end when no mark has the name. The sentence says the name is not a mark on the chart. The route includes those cells. A lair mouth resolves as mountain, and that cell stays on the route. The spur slide leaves an end cell where it is. Encounter marks stay off the bang list. A lair mouth stays a bang, and its cell is still an end. `RouteForRoad` solves through `LoadFromChart`.
- `ESite` leaves. Iglin Bridge no longer names `IglinBridgeRamp`. `Place` takes the mark position as the focus. The Iglin town point and the bridge list leave that call. The band, the separation, the cliff check, and the tree and forage skips stay, and they are read from the definition. The `SolidCore1.Monster.*` line that says a missing Iglin bridge places none becomes: a missing mark places none.
- Basic monsters puts the three beside the ramp through that site. This story removes the site. With no encounter mark on the chart, those three are absent from the world. The catalog row remains.
- Importing `WorldMap.txt` rebuilds cells, towns, strongholds, sites, lair mouths, and roads, then puts back each encounter mark whose cell has no imported mark. It also puts back each road whose end is a stronghold site, a lair mouth, a custom note, or an encounter. A cell the import filled drops that encounter mark.
- Disagreement compares the chart to the text file with encounter marks set aside, and with roads set aside when an end is a stronghold site, a lair mouth, a custom note, or an encounter. [Remove WorldMap.txt](remove-world-map-txt.md) is Ready. Once that story has shipped, there is no disagreement line and no import to keep. The `SolidCore1.Road.*` line that says an end is a town or a stronghold gains the other marks.
- The encounter billboard is amber, `FLinearColor(0.95f, 0.72f, 0.15f)`.
- Encounter-mark cases live in `SolidCore1.WorldChart.*`. The focus-from-mark case lives in `SolidCore1.Monster.*`. The product commit updates those lines in `Source/SolidCore1/Tests/README.md`. Add `SolidCore1.WorldChart.*` to that table if it is still missing.
