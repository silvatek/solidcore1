# Roads end on any mark

Status: Ready

## Story

This story follows [Place encounter markers](place-encounter-markers.md), which is In Progress. That story ships first. Encounter marks and the members that stand at a mark stay as that story placed them. A road joins any two marks on the chart. A town end is the town name, and a stronghold end is the stronghold name. A stronghold site, a lair mouth, a custom note, or an encounter is an end as well. This story introduces no game concept. Road and Encounter are already named.

## Acceptance

- A road joins any two marks. A town end is the town name, and a stronghold end is the stronghold name, as in `Content/WorldMap.txt`. A stronghold site, a lair entrance, a lair exit, a custom note, and an encounter use the kind and the name, in the form of a `!` line: `StrongholdSite (Humble)`, `LairEntrance (Iglin tunnels)`, `LairExit (Iglin tunnels)`, `Custom (note)`, `Encounter (Iglin Bridge)`. `LairEntrance (Iglin tunnels)` and `LairExit (Iglin tunnels)` are different ends. The route finishes on a cell of that kind and name. Several marks of the same kind and name are one end, and the route finishes on one of those cells, as Dragongate's two town cells are one town. A road whose end matches no mark is stored, and its route is empty. The tool names that missing end.
- `Content/WorldMap.txt` has no encounter line, and its roads name towns and strongholds. The mode reports a disagreement for every other difference. An encounter mark on its own leaves that line empty. So does a road whose end is a stronghold site, a lair mouth, a custom note, or an encounter. Importing the text file keeps an encounter mark on a cell the import left empty, and keeps each of those roads.
- Tests solve a road to `LairEntrance (Iglin tunnels)` that finishes on the entrance cell, and a road to `LairExit (Iglin tunnels)` that finishes on the exit cell. They also solve a road to a site, a custom note, and an encounter. A road whose end matches no mark has an empty route. The tests run without a viewport. The editor session on the Windows machine is the check that the mode matches. A cloud agent does not run the editor.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

Stage 1 is road ends. Stage 2 is import and disagreement. The notes under each stage are the design already agreed on [Place encounter markers](place-encounter-markers.md). [Remove WorldMap.txt](remove-world-map-txt.md) is Ready. Once that story has shipped, there is no disagreement line and no import to keep.

### Stage 1 — road ends

- A town or stronghold end stays the name a road line already uses. `Iglin` and `MeagreDungeon` keep solving. A site, a lair mouth, a note, or an encounter is the kind word and the name. Parentheses match the `!` lines. The comparison ignores case and spaces. `SolidRoad::Parse` keeps that end, and the chart resolves it to the marks of that kind and name. `LairEntrance (Iglin tunnels)` is the entrance cell. `LairExit (Iglin tunnels)` is the exit cell. The bare name `Iglin tunnels` matches neither on its own. Several cells of one kind and name are alternative ends, and the route finishes on one, as it does for Dragongate. `RoadEndError` reports a missing end when no mark matches. A lair mouth resolves as mountain, and that cell stays on the route. The spur slide leaves an end cell where it is. Encounter marks stay off the bang list. A lair mouth stays a bang, and its cell is still an end. `RouteForRoad` solves through `LoadFromChart`.

### Stage 2 — import and disagreement

- Importing `WorldMap.txt` rebuilds cells, towns, strongholds, sites, lair mouths, and roads, then puts back each encounter mark whose cell has no imported mark. It also puts back each road whose end is a stronghold site, a lair mouth, a custom note, or an encounter. A cell the import filled drops that encounter mark.
- Disagreement compares the chart to the text file with encounter marks set aside, and with roads set aside when an end is a stronghold site, a lair mouth, a custom note, or an encounter. The `SolidCore1.Road.*` line that says an end is a town or a stronghold gains the other marks.
