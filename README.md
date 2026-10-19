# SolidCore1

UE **5.8** C++ open-world starter with World Partition–oriented config, a Viking captain, and Enhanced Input (move / look / jump / sprint).

The design north star is [`GameVision.md`](GameVision.md). The world is [`Content/WorldLore.md`](Content/WorldLore.md). Coding guidelines are [`CodingGuidelines.md`](CodingGuidelines.md).

## To-do

Open stories live in [`Planning`](Planning). Each file is one idea. A shipped story moves to [`Planning/Done`](Planning/Done).

- [Fog stays cleared](Planning/fog-stays-cleared.md)
- [White material for full fog](Planning/white-fog-terrain.md)
- [Hide trees and buildings in full fog](Planning/hide-props-in-full-fog.md)
- [Better building and tree models](Planning/better-building-and-tree-models.md)
- [Objects sit in the terrain](Planning/objects-sit-in-the-terrain.md)
- [HUD and menu borders and themes](Planning/hud-menu-borders-and-themes.md)
- [Companions move only with the captain](Planning/companions-move-with-captain.md)
- [Drill battle plan](Planning/drill-battle-plan.md)
- [Falcon Sight and Eagle Sight](Planning/falcon-and-eagle-sight.md)
- [More terrain textures](Planning/more-terrain-textures.md)
- [Sight indicator after Raven Sight](Planning/sight-indicator-after-raven.md)
- [Early game battle plans](Planning/early-game-battle-plans.md)
- [Journal Filters and Scrollbar](Planning/journal-filters-and-scrollbar.md)
- [Replace FAB_Doctor with an automated test](Planning/fab-doctor-automated-test.md)
- [Hold battle plan](Planning/hold-battle-plan.md)
- [Events auto-trigger other events](Planning/events-auto-trigger.md)
- [Stamina](Planning/stamina.md)
- [Basic monsters](Planning/basic-monsters.md)
- [Basic NPCs](Planning/basic-npcs.md)
- [Pathfinding over bridges](Planning/pathfinding-over-bridges.md)
- [Rough stones](Planning/rough-stones.md)
- [Fortress stronghold](Planning/fortress-stronghold.md)
- [Imperious dungeon](Planning/imperious-dungeon.md)
- [Basic furniture management](Planning/basic-furniture-management.md)
- [Multiple storage containers](Planning/multiple-storage-containers.md)
- [Name the company's stronghold](Planning/name-the-stronghold.md)
- [Return to the stronghold](Planning/return-to-stronghold.md)
- [Package the game](Planning/package-the-game.md)
- [Drop the Assassin and Blacksmith packs](Planning/remove-assassin-and-blacksmith.md)
- [Generate the character mesh outside the editor](Planning/character-mesh-outside-the-editor.md)
- [Company lists use TArray](Planning/company-lists-as-tarray.md)

## Done

Shipped stories live in [`Planning/Done`](Planning/Done).

- [Captain + Squad](Planning/Done/captain-and-squad.md)
- [Company roster](Planning/Done/company-roster.md)
- [Battle plans](Planning/Done/battle-plans.md)
- [World + biomes](Planning/Done/world-and-biomes.md)
- [Lairs](Planning/Done/lairs.md)
- [Cave stronghold](Planning/Done/cave-stronghold.md)
- [Imperious Tower](Planning/Done/imperious-tower.md)
- [Imperious Tower refinements](Planning/Done/imperious-tower-refinements.md)
- [No starter monolith](Planning/Done/no-starter-monolith.md)
- [Fog follows the player](Planning/Done/fog-follows-the-player.md)
- [Procedural terrain](Planning/Done/procedural-terrain.md)
- [Town + forest](Planning/Done/town-and-forest.md)
- [Roads](Planning/Done/roads.md)
- [Bridges](Planning/Done/bridges.md)
- [Town signs](Planning/Done/town-signs.md)
- [True sight / raven sight](Planning/Done/true-sight-raven-sight.md)
- [Events](Planning/Done/events.md)
- [Journal](Planning/Done/journal.md)
- [Inventory](Planning/Done/inventory.md)
- [Crafting](Planning/Done/crafting.md)
- [Oak and bronze](Planning/Done/oak-and-bronze.md)
- [Debug HUD](Planning/Done/debug-hud.md)
- [Mouse pointer](Planning/Done/mouse-pointer.md)
- [Better mouse pointer](Planning/Done/better-mouse-pointer.md)
- [Click to move](Planning/Done/click-to-move.md)
- [Main menu](Planning/Done/main-menu.md)
- [Reorder the game menu](Planning/Done/reorder-game-menu.md)
- [Exit the game](Planning/Done/exit-the-game.md)
- [Content pipeline](Planning/Done/content-pipeline.md)
- [Automation](Planning/Done/automation.md)
- [Editor build](Planning/Done/editor-build.md)
- [Stronghold transition](Planning/Done/stronghold-transition.md)
- [Remove den from the code](Planning/Done/remove-den-from-the-code.md)
- [World map location markers](Planning/Done/world-map-location-markers.md)
- [Stronghold markers select size only](Planning/Done/stronghold-markers-size-only.md)
- [Foraging](Planning/Done/foraging.md)
- [Rename Party to Squad](Planning/Done/rename-party-to-squad.md)
- [Remove unused code](Planning/Done/remove-unused-code.md)
- [Enable a lair from an event](Planning/Done/enable-lair.md)
- [Squad builder drag and drop](Planning/Done/squad-builder-drag-and-drop.md)
- [Open the squad builder on leaving a stronghold](Planning/Done/squad-builder-on-stronghold-exit.md)
- [Dungeon exterior](Planning/Done/dungeon-exterior.md)
- [Stronghold type classes](Planning/Done/stronghold-type-classes.md)

## Controls (runtime defaults)

Keyboard and gamepad defaults are in [`Content/Controls.md`](Content/Controls.md), one table per mode.

## Automated tests

UE Automation tests live under `Source/SolidCore1/Tests/` (editor builds, `WITH_DEV_AUTOMATION_TESTS`). The rules for writing and changing them are in [`CodingGuidelines.md`](CodingGuidelines.md).

### Suite (filter `SolidCore1`)

| Filter | Covers |
|--------|--------|
| `SolidCore1.Fog.*` | Distance bands, units, mist sampling, fog mesh build guards, camera stays on clear ground, curtain above max zoom |
| `SolidCore1.Map.*` | Build smoke, trail clear, idempotent build, sampling, bounds, biome Z shelf |
| `SolidCore1.WorldMap.*` | ASCII overlay load, key colors, a `!` line is `Town`, `Stronghold`, `StrongholdSite`, `LairEntrance`, or `LairExit` (row, column; top-left is 1,1; `#` starts a comment), digits and the letters `C`, `Z`, `X`, and `Y` are not locations, sea is `~`, grass is `.`, a town cell is `o`, forest is `#`, a river is `*`, desert is `^`, 1024 m square world, cell size follows the map width and height, fog follows the player |
| `SolidCore1.River.*` | 4 m pond, orthogonal joins to river or sea, banks take the adjacent biome; a road bank uses the road texture |
| `SolidCore1.Road.*` | Trail, track, road, and highway widths; an end is a town name or a stronghold name; every route stays off mountains and the sea; roads avoid forest, desert, and extra river cells; a crossing runs straight onto both banks with no spur past the far bend; the strip is a road brown and a crossing keeps the river; `o` and a named town `!` are towns |
| `SolidCore1.Bridge.*` | A crossing gets an up ramp, a flat center, and a down ramp; the center matches that road's width and that river's span and lies on the painted strip; each ramp runs down into its own bank; the Iglin–Relion center starts left out; repairing it still marks that crossing; other crossings keep theirs |
| `SolidCore1.GroundMove.*` | Sea, mountain, and the river channel block a step; the rest of a river cell stays open; an intact bridge is the only crossing; a broken center does not open the channel; the gap in front of a broken ramp stays closed; a diagonal step slides along the edge; a step already on closed ground may only leave; a ramp is the ramp surface, not the deck or the gap past the lip |
| `SolidCore1.Towns.*` | Town names and type (Town or WallGate); Dragongate is the wall gate, with a 4 m gap and two equal towers; a two-cell gate follows the long axis |
| `SolidCore1.Stronghold.*` | Types and sizes; a defined stronghold names its shelf, Meagre Dungeon on grassland with its exit facing north, and Imperious Tower on grassland; the starting stronghold is a Meagre Dungeon named on the map, and a new game records that base at its cell and carries it with the company; the mouth faces the grassland neighbor; the cave starts closed until the monster message is closed; F10 cave travel keeps companions outside; a click on the dark opening enters and leaves once the cave is open; the Meagre Dungeon visit view levels the camera into the cave and out over the grass; the Meagre floor is fourteen irregular hexes; a furnishing catalog places the fireplace, bed, and chest on that single floor; the chest is storage and the fireplace and bed are not; the Imperious Tower plan is ten floors in 30 m, a 50 cm wall, and hexes that stay off the stair and the landing; a tower plan is one struct; Average Tower is four floors, 12 m tall, and 8 m across at the ground; Impressive, Grand, and Stately step evenly from Average to Imperious; the Imperious exterior is ten stone courses that meet edge to edge under a flat 6 m cap with a dark 1.4 by 2.4 m door, Average is four courses under a 5.6 m cap, and the exterior turns so that door faces the grass; a click on the tower door enters, the interior is a stone disc per floor with the arrival hole, a flat landing of about two metres in the stair band, with that floor's number on the wall beside it, a waist-high rail along each flight and each arrival hole that stops for the landing, the stair treads, and the yellow grid shifted a little so more cells sit in the circle, and leaving stands on the grass outside that door; a stronghold site lists a built type only when that size is unlocked and larger than the company base, Esc leaves the site, and choosing the type moves the furnishings into that interior and stops the old entrance |
| `SolidCore1.Lair.*` | Iglin tunnels: `LairEntrance` south of Iglin and `LairExit` east of Relion use a cliff frame on their own cliffs (those lines are the interior `X` and `Y` ends); the maze joins those ends, including a straight run for early testing; the mouths stay shut until an EnableLair change carries the Iglin tunnels id; a click enters and leaves by the end that was hit; each mouth is a dim light and the middle has none; a torch in either hand lights the captain, and a torch in the pack does not; his body does not cast a shadow in the tunnels |
| `SolidCore1.Noise.*` | Hash / value / fBm / height / grass tone |
| `SolidCore1.Types.*` | Biome names (incl. Sea/River), height offsets, border triangles use the higher shelf, `FSolidTerrainPoint` defaults |
| `SolidCore1.Vegetation.*` | Tree RNG variation, town building pack on cells marked `o` or a `!` named as a town, one welcome sign at the middle of each named town; Dragongate's sign stands east of the gate on the north edge, facing north |
| `SolidCore1.GameMode.*` | Default spawn flags, pawn BP resolution |
| `SolidCore1.Companion.*` | Defaults, SetFollowTarget |
| `SolidCore1.Body.*` | Bone roles; a seeded body on the Viking skeleton stays under the polygon budget and keeps shirt, trouser, and skin slots; a name picks a Unicode face |
| `SolidCore1.Company.*` | Viking, Assassin, and Blacksmith meshes; roster of five with vocations, torso colors, and leg colors; squad selection; fielded count follows the squad limit; selection survives ensure |
| `SolidCore1.Streamer.*` | FindExisting / EnsureExists (null + idempotent) |
| `SolidCore1.Clip.*` | SelectClip idle/walk/run/jump rules |
| `SolidCore1.NameLabel.*` | Style sizes/colors/plates; Outcast / Sam defaults; vocation initial on the companion plate; captain plate hidden when alone or in a stronghold |
| `SolidCore1.BattlePlan.*` | Formation slots; Company/Squad assign; selection survives squad ensure; drill legs; F5 Parade fires Enter Relion and Enter Dragongate once; left shoulder plus a face button is the first four slots, right shoulder the last four |
| `SolidCore1.Credits.*` | Author / Cursor+Grok / Fab attribution; HUD toggle |
| `SolidCore1.MainMenu.*` | F10 menu entries (Use portal, Journal, Build squad, Character editor, Credits, Exit Game) and selection; Credits then Exit Game are the last two rows; Build squad stays dim below two companions; Menu opens and closes the menu |
| `SolidCore1.CharacterEditor.*` | Character editor row, F10 returns to the world, height build and gender sliders, Sam's red shirt and Alex's blue shirt, attribute numbers |
| `SolidCore1.SquadBuilder.*` | Build squad stays locked below two companions; companions and battle plans are dragged onto boxes; Confirm closes the page; a stronghold leave opens it when there is something to arrange |
| `SolidCore1.MenuPointer.*` | Pointer hit-tests a menu or portal row; a miss leaves the keys' selection |
| `SolidCore1.Exit.*` | Exit Game is the last row, after Credits; the page asks Exit the game?; Yes asks to quit, and a PIE yes also asks to close the editor; No and Esc leave the game running |
| `SolidCore1.Portal.*` | Town list, including Dragongate and Rathen, then the grass north of the Iglin tunnels; keys 1–8; a town landing stays on one cell of that town; the tunnel landing records Driven from the village; portal page |
| `SolidCore1.WorldTime.*` | Session 1 is 200/45 AtF; session 9 at 84 minutes is 01:24, 200/53 AtF; day 100 rolls the year |
| `SolidCore1.Journal.*` | Empty journal, then events in the order they fired, with Time and Summary columns; each line is an Event, a Stronghold, or a Lair, including the captain's title and repeated Enter Stronghold / Exit Stronghold lines |
| `SolidCore1.Crafting.*` | Torch is a known recipe; it makes one torch from one stick and one pitch |
| `SolidCore1.Inventory.*` | Item catalog: Rags (armor), a dented helmet (helmet), a crude spear (weapon), and a torch (tool); the torch and pitch are Common and the other items are Inferior, except coins which have no grade and stack to 999; a stick stacks to 10; each of the first items stacks to 1; the cave chest is two columns by three rows and starts with the rusty sword, old boots, 20 coins, five sticks, and three pitch; slots store those ids and the page shows the name; armor fits only the body, a helmet only the head, boots only the boots, and a belt only the belt; any item can go in the pack or the chest; a drag moves an item between worn boxes, pack cells, and chest cells, and an occupied destination swaps when both items fit; a closed chest refuses the drop; starting kit is the helmet, Rags, and the spear; boots, the off hand, and the belt start empty; Clear empties worn boxes, the pack, and the crafting slots, and leaves Rags on the body; person-shaped worn boxes with those names above them, in the same letters as Pack and Chest; pack is two columns by three rows; four crafting slots and a small bronze Craft button sit on the bottom right; the button lights when a known recipe fits those slots and the pack has an empty cell; pressing it spends the ingredients and puts one result in the first empty pack cell, and it stays lit only while another craft still fits; a drag can move an item into a crafting slot and back out; `/`, View, and Create open the page, and Esc, F10, or Menu closes it |
| `SolidCore1.HudTheme.*` | Oak-and-bronze frame thickness, corner bosses, opaque board; translucent oval sight marker |
| `SolidCore1.HUD.DebugToggle` | F12 shows and hides the tech readout |
| `SolidCore1.Pointer.*` | Bronze arrow tip, two head triangles and no tail, outline, and hide-while-looking |
| `SolidCore1.ClickMove.*` | Single click, raven-only; WASD is true sight; ground ray |
| `SolidCore1.Sight.*` | True sight at start; F9 or a right stick click switches to raven sight and back; entering true sight levels the camera and keeps the yaw; eye and eagle markers |
| `SolidCore1.Events.*` | Enter-town events, Reach broken bridge, Killed by monsters, and Driven from the village: each has an eight-digit id and an act (Iglin, the bridge, and the monster message are ACT_0, the return to Iglin is ACT_1, Relion ACT_2, Kanfold ACT_3, Dragongate ACT_4); messages have their own ids; the ramp shows "You are killed by monsters"; closing it opens the cave, sends the captain inside, and clears his inventory to Rags; Enter Iglin titles the captain Orphan; the next entry to Iglin shows "You are driven from the village", titles Outcast, and an EnableLair change carries the Iglin tunnels id; Enter Iglin and Killed by monsters leave the tunnels shut; the journal names that lair as enabled with the message and the title; Relion and Reach broken bridge wait until Iglin has fired; nothing before Relion enables raven sight; Relion, Kanfold, and Dragongate enable raven sight; max squad size through the whole company; captain title through Captain; Relion repairs the Iglin bridge; a named town cell is that town; an EnableStrongholdSize change enables one stronghold size; Killed by monsters enables Meagre, Enter Relion enables Humble, and Enter Kanfold enables Average; Enter Dragongate still lists one for every size, including those three, until each size has only one event; Enter Iglin does not enable a size |
| `SolidCore1.Build.*` | `SOLID_BUILD_ID` / note present |
| `SolidCore1.Content.*` | Required Content + Engine assets the code loads; Assassin and Blacksmith folders; recorded Fab paths |

**Content dependency tests** assert meshes/materials/anims/BPs the C++ loaders expect (Engine BasicShapes, FlatCol for solid colors, Viking mesh+locomotion, Fab grass, pawn/GameMode BP-or-C++). Viking and Fab grass are required; Epic mannequin assets are not used.

## Fab assets (Epic Games Launcher)

Grass, Viking, the Assassin, and the Blacksmith come from Fab. There is **no official download API**. Native UE `.uasset` packs cannot be batch-exported — each listing is **Add to Project** on its own. `resolvedPath`, `resolvedFolder`, and `resolvedPaths` in `tools/fab-assets.json` are tried first. If that file or folder is missing, Viking and grass are found by **asset name** under `/Game/Viking` or `/Game/Fab`, and the Assassin and Blacksmith packs are the folders `Medieval_Assassin_Girl` and `Elf_Blacksmith` anywhere under Content. A find is written back into the manifest. Do **not** move folders after Add to Project.

| Asset | Listing | Required | Resolved as |
|-------|---------|----------|-------------|
| Viking (Art.Hiraeth) | [fab.com/listings/ca4ba583-…](https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f) | Yes | `SK_Viking` + `Anim_Viking_*` (captain) |
| 025 Grass | [fab.com/listings/94bfee39-…](https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a) | Yes | `Mat_025_grass` |
| Free Medieval Assassin Girl (Dimentrius3dArtist) | [fab.com/listings/2986b290-…](https://www.fab.com/listings/2986b290-90ed-4a0e-a891-cacf442033f9) | Yes | `Meshes/base_mesh/SKM_base_mesh_ue` in `Medieval_Assassin_Girl` |
| Elf Blacksmith (Art.Hiraeth) | [fab.com/listings/d01abf8c-…](https://www.fab.com/listings/d01abf8c-9798-4bcb-8617-2d1d560d318a) | Yes | `Mesh/SK_Elf_Blacksmith` in `Elf_Blacksmith` |

**Restore (Launcher + UE 5.8):**

1. Epic Games Launcher → **Unreal Engine → Fab**. Sign in.
2. Open each listing. **Add to My Library** if needed (Ctrl+click can batch this).
3. **Add to Project** → this `SolidCore1.uproject`. Done.

```bat
tools\fab_doctor.bat
```

tries each recorded path in `tools/fab-assets.json` first, then searches. Viking and grass fall back to `Content/Viking` or `Content/Fab`. The Assassin body is `Meshes/base_mesh/SKM_base_mesh_ue.uasset` (`SK_Mannequin.uasset` beside it is the skeleton). The Blacksmith body is `Mesh/SK_Elf_Blacksmith.uasset` (`SK_Elf_Blacksmith_Skeleton.uasset` is the skeleton). A different recorded file is replaced when that body is found. Official notes: [Exporting Assets from Fab in Launcher](https://dev.epicgames.com/documentation/en-us/fab/exporting-assets-from-fab-in-launcher).

```bat
tools\fab_mesh_info.bat Medieval_Assassin_Girl
tools\fab_mesh_info.bat Elf_Blacksmith
```

lists that pack's folders, mesh files (size and SKM / SK / PA), and animation names. It does not change `fab-assets.json`.

We still commit these folders today so a clone runs without the Launcher.

Not yet covered (need PIE): character movement, companion steering on terrain, streamer chunk load/unload, HUD drawing.

**Session Frontend:** rebuild editor → **Tools → Session Frontend → Automation** → filter `SolidCore1` → Start.

**Command line** (prints ran / passed / failed from the automation log):

```bat
tools\run_automation_tests.bat
tools\run_automation_tests.bat SolidCore1.Fog
```

The summary prints ran/passed/failed counts and, on failure, lists each failed test path (parsed from `Result={Fail}` lines in the automation log).

## Build ID (HUD)

PIE shows a top-left debug HUD (`Build SC1-NNNN`, a one-line `Change:` note, pawn/terrain Z, chunk load, material, camera pitch). Both strings live in `Source/SolidCore1/Game/SolidBuildId.h` (`SOLID_BUILD_ID` / `SOLID_BUILD_NOTE`) and are bumped when the product changes, so screenshots identify which binary you ran. A documentation change or a test change does not bump them.

## Character visuals (clip locomotion)

Captain and Companions use single-node clip locomotion via `SolidClipLocomotion` (not Epic AnimBP). The captain stays on Fab Viking (`/Game/Viking/Mesh/SK_Viking` + idle/walk/run/jump). Companions wear a generated low-poly body on that skeleton, with a matte shirt, matte trousers, light pink head and hands, and a drawn face, and play the same Viking clips. No Epic mannequin fallback.

### Reliable setup: Blueprint pawn (recommended)

1. Content Browser → right-click `Content/Characters` → **Blueprint Class**.
2. Pick **SolidCharacter** as the parent → name it `BP_SolidCharacter` (path `/Game/Characters/BP_SolidCharacter`).
3. Compile & Save (runtime forces the Viking mesh/clips even if the BP mesh slot is empty).
4. Close the editor, rebuild/reopen so GameMode picks up the Blueprint (it prefers this BP over the bare C++ class).
5. PIE — you should see the Viking player (and Viking companion).

**Naming:** Content Blueprints are `BP_SolidCharacter` / `BP_SolidGameMode` (parents `SolidCharacter` / `SolidGameMode`). Load paths are centralized in `SolidContentPaths.h`.

While PIE is running, **Output Log** filtered to `LogSolid` shows whether a mesh was applied.

## Open and build

### Option A — Editor

1. Double-click `SolidCore1.uproject` (associate with UE 5.8 if prompted).
2. Allow it to generate Visual Studio project files and compile the `SolidCore1` module.
3. When the editor opens, create the World Partition map (next section).

### Option B — Generate + command-line compile

From the repo root:

```bat
tools\build.bat
```

`UE_ROOT` defaults to `C:\Program Files\Epic Games\UE_5.8`. `PROJECT` defaults to `SolidCore1.uproject` next to `tools\`. Set either env var to point somewhere else.

Or right-click `SolidCore1.uproject` → **Generate Visual Studio project files**, open the `.sln`, then build **Development Editor | Win64**.

## Runtime procedural terrain chunks

C++ generates walkable terrain around the player at runtime:

- `FSolidTerrainPoint` / `ESolidBiome` — simulation cell (X, Y, Height, Biome, Threat, Fog)
- `USolidWorldMap` — 128×64 ASCII biome overlay (`Content/WorldMap.txt`) scaled across the TerrainMap world bounds. Cells are 8 m east–west and 16 m north–south.
- `USolidTerrainMap` — 2D TerrainPoint grid built once at streamer startup (default 513×513 @ 200 cm); biomes from WorldMap when loaded. The ASCII map is stretched across that rectangle, so doubling the point count doubles each WorldMap cell.
- `ASolidTerrainChunk` — runtime `UStaticMesh`; vertex heights from the TerrainMap; per-triangle biome materials, and a border uses the higher shelf
- `ASolidTerrainStreamer` — builds the map, relocates the pawn to location 0, then loads/unloads chunks
- Debug HUD shows biome / threat / fog at the pawn plus map size

Defaults: 64 m chunks (`ChunkWorldSize=6400`), 32 quads/side, radius 2 (5×5 chunks), `Amplitude=3000`. Grassland uses Fab `Mat_025_grass`; other biomes use FlatCol tinted by the WorldMap key colors.

Default map is `/Game/ThirdPerson/Lvl_ThirdPerson` (SC1-0022) so Open World Landscape/HLOD outer hills are not in the scene — that cleared the horizon slivers. `L_OpenWorld` remains for comparison. SC1-0023/0082 force `BP_SolidGameMode` on PIE/game worlds so template maps keep Solid HUD + Viking pawn instead of the template default. On first stream, the pawn is snapped onto the procedural height. If a Landscape is present, actors are hidden/collision-disabled once. Chunks block the Camera channel and use complex-as-simple collision on the runtime static mesh.

SC1-0007 drops `UProceduralMeshComponent` after persistent ribbon/culling failures with that path.

## Exploration fog (fog-of-war)

See [`Source/SolidCore1/Fog/README.md`](Source/SolidCore1/Fog/README.md) — trail-cleared bands, marching-squares curtains, and rejected approaches.

## WorldMap biomes (SC1-0105)

`Content/WorldMap.txt` is a 128×64 grid of markers plus a color key (`~` Sea, `.` Grassland, `o` Town, `M` Mountain, `#` Forest, `^` Desert, `*` River). A `!` is one cell. `! 8,21 = Town Iglin` names row 8, column 21, counting from 1 at the top-left of the grid. The identifier may be wrapped in brackets, as in `Town (Iglin)`. `! 9,12 = Stronghold MeagreDungeon` is that size and type (`Meagre Dungeon` is the same token). `! 12,12 = StrongholdSite (Humble)` names a size and no type. A site is not a town and keeps the marker's biome. `! 10,11 = LairEntrance Iglin tunnels` and `! 11,40 = LairExit Iglin tunnels` are the two mouths of that lair. Anything after `#` is a comment. A town `!` is that town. A lair `!` uses the mountain shelf and is not a town. A stronghold `!` is not a town. Its shelf comes from that stronghold: Meagre Dungeon is Grassland, and its exit faces north. Imperious Tower is Grassland. A stronghold with no shelf listed keeps the marker's biome. A `!` with no line is not a location. A coordinate outside the grid, or on a cell that is not `!`, is ignored, and the map still loads. A number on the right-hand side is ignored. Town names live in `Towns/SolidTowns.h`. `LairEntrance` and `LairExit` are the Iglin tunnel mouths. `LairEntrance` is the interior `X` end and `LairExit` is the interior `Y` end. Those `!` cells use the mountain shelf, so the cliff stays closed, and they are not towns, locations, or the cave. A digit, or the letters `C`, `Z`, `X`, and `Y`, is grassland and is not a location, a mouth, or the start. A key line for one of those does not recolor a biome. The grid scales across the TerrainMap world rectangle (file row 0 = north). The default rectangle is 1024 m on a side. A marker is 8 m east–west and 16 m north–south. A `*` marks a river through that square, not a square of water. The channel is 4 m wide. An isolated `*` is a 4 m pond. A `*` beside another `*`, or beside `~`, joins that neighbor edge to edge and stays 4 m wide however long the square is. A diagonal neighbor does not join. The dry part of the square uses the adjacent cell, so the two banks can differ (grass on one side, mountain on the other, or sea where the river meets it). A `Road:` line under the key is `start -> end, type`. An end is a town name or a stronghold name such as `MeagreDungeon`. Brackets around a name are optional. A trail is 1 m wide, a track 2 m, a road 3 m, and a highway 4 m. The route is solved on the grid and drawn in a brown close to the town colour. Houses are placed only on cells marked `o` or a `!` named as a town. Every route stays off mountains and the sea. Roads and highways also stay off forest and desert, and they pay extra for each river cell so a crossing stays short. Where the strip meets the water, the channel keeps the river texture. Location **0** (Iglin) is the starting town: the streamer relocates the player there. Fog bands are centered on the player after that placement.

## Forest trees (SC1-0076 / SC1-0110)

- `ASolidTree` — cylinder trunk + cone canopy; scattered randomly across **Forest** TerrainPoints (`ForestTreeDensity`, capped by `MaxForestTrees`).
- Logic in `Vegetation/SolidForestTrees.*`. Toggle with `bAutoSpawnVegetation`.
- Fog curtains: see [`Fog/README.md`](Source/SolidCore1/Fog/README.md) (clear|fogged ~25 m plus a **white** half→full curtain ~50 m).

## Name labels (SC1-0091 / SC1-0093)

Floating nameplates (`SolidNameLabel`) sit above each Squad member and face the view camera: TextRender plus thin cube **border** and contrasting **background** plates. Defaults: Captain **"Outcast"** (larger warm amber on dark plate), Companions muted slate plates. The captain's plate shows in raven sight only, and only when at least one companion is with him outside a stronghold. Enter Iglin renames the captain to Orphan. Driven from the village sets Outcast. Relion renames the captain to Vagrant, Kanfold to Ranger, and Dragongate to Captain. Override via `CharacterDisplayName` / `SetCharacterDisplayName`.

## Battle plans (SC1-0095 / SC1-0101)

- **Company** (`USolidCompany`) owns the catalog of all battle plans. Starter assigned set:
  - **F1 Line** (Standard) — companions on the flanks
  - **F2 Column** (Standard) — file behind the Captain
  - **F3 Tight mob** (Narrow) — close triangle behind
  - **F4 Loose mob** (Wide) — spread triangle behind
  - **F5 Parade** (Standard) — line in front of the Captain, facing him, far enough that the rank fits in his view. Selecting it fires Enter Iglin if needed, then Enter Relion and Enter Dragongate, so the rank, raven sight, and the repaired Iglin bridge are ready even at the start of a session. Those events still fire only once.
- **Squad** (`USolidSquad`) holds up to 8 **assigned** plans and one **active** plan (default **F1 Line**).
- Each plan has a **formation** plus **spacing** (`Narrow` / `Standard` / `Wide`) that scales follow distances.
- Captain switches assigned slots with **F1–F8**, or a shoulder button plus a face button (left shoulder for the first four, right shoulder for the last four). Only filled slots work. The choice stays put when town events refresh the squad.
- **Test Drill** (main menu) runs a formation drill: F1 walk 1.5s → F2 / F3 / F4 each blend +90° yaw over 250 ms then walk 1.5s (square path; player move/look suppressed while active).
- HUD shows a **Battle Plans** panel under the tech block: all 8 slots, F-key + name when assigned, active slot highlighted in amber (aligned marker column).
- Engine viewmode debug binds (wireframe/unlit/lit/…) are moved to **Ctrl+F1–F5** in `Config/DefaultInput.ini` so bare F-keys stay free for battle plans. Restart the editor after pulling.

## Companion (SC1-0024 / SC1-0093)

`ASolidCompanionCharacter` follows with simple steering (no NavMesh — works on procedural terrain). The captain stays on `SK_Viking`. A companion wears a low-poly body skinned to the Viking skeleton: a chest, a tapered cylinder over each shoulder, arms, and legs down to the feet at body thickness. The hip cylinder's bottom meets the outside of the thighs. The shirt, the trousers, and the skin on the head and hands are separate matte colors. Those colors live on the companion record and are applied on the actor, so a shared body is not recolored while it is on screen. Sam's shirt is red and his trousers are blue. Alex wears the swap. Both use light pink skin. The F10 character editor shows one companion on a plain background so those colors can be checked without the open world. Space swaps that body for the Viking skeleton, drawn as bone lines. Bones that share a joint use different colours. Home turns the figure clockwise and End turns it the other way. Resting the pointer on a bone shows that bone's name in black. Clothing and weapon bones are left off that view: axe_, sheath_, cloack_, beard_, shield_, and skirt_. Thigh and calf twist bones are left off too. calf and foot stay, so the hip-to-knee line and the knee-to-ankle line are both drawn. Page Up and Page Down cycle the company. Up and Down select height, build, or gender, and Left and Right change that value. The pointer can drag the same sliders. F10 returns to the world. The face is chosen from the same unicode points, including U+1F611. The engine font has no emoji, so the expression is drawn on the front of the head. The body stays lighter than the Viking sculpt. The three sliders run from 0 to 1 and set every proportion. Build 0 is short and wide, and build 1 is tall and thin. Gender 0 is feminine and gender 1 is masculine. Height 0.5 leaves that size; 0 shortens it and 1 lengthens it. At 0.5 on every slider every part stays at its catalogue size and every bone keeps the Viking length. Moving a slider changes those part diameters and lengths, and the bones inside the part take that size. The head and the hands are not parts, so they stay the Viking size. The body is being built one piece at a time. The pieces so far are the two thighs and the two calves. Each thigh is a truncated cone the length of calf_l or calf_r, from the hip to the knee, 20 cm across at the hip and 15 cm across at the knee. Each calf is one cone the length of foot_l or foot_r, from the knee to the ankle, 15 cm across at the knee and 12 cm at the ankle. Each foot is a cylinder the length of ball_l or ball_r, from the ankle to the toe, 10 cm across. Each toe is a truncated cone past the foot, 10 cm across at the end towards the ankle and 5 cm at the toe. The hips are one shirt piece around thigh_l, thigh_r, and spine_01. It is the length of spine_01. The bottom is 50 cm side to side and 20 cm front to back, and the top is 30 cm side to side and 20 cm front to back. Each spine bone above spine_01 is its own shirt piece, still 20 cm front to back, widening from that 30 cm waist to 40 cm side to side at the top of the spine. The shoulders are one shirt piece around clavicle_l, clavicle_r, upperarm_l, upperarm_r, and neck_01. It is the length of neck_01. Its base is the same oval as the top of the spine_03 piece. It widens past the upperarm bones, then narrows to a circle of 20 cm diameter at neck_01. The drawn body is one list of named parts. Each part draws its own triangles, and the list draws the whole character. A part's name is the region it shows, which is often different from the skeleton bone it wraps. Each part stores that bone list, a shirt, trouser, or skin colour, and the side-to-side and front-to-back diameters at the start and the end of its cone. The shoulders also store a mid section, fitted when the piece is drawn. The upper arm is a shirt cone the length of lowerarm_l or lowerarm_r, 8.4 cm at the shoulder and 6.8 cm at the elbow. The lower arm is a shirt cone the length of hand_l or hand_r, 5 cm at the elbow and 6.8 cm at the wrist. The shoulder_jnt, finger, head, and eye bones are still bare, and so are the twist bones. Hovering a piece names the bones in it. The head is opaque. A companion's face still comes from the display name, and that body is built once and reused. They play Viking clips. The company catalog still lists the Assassin mesh for Sam, Morgan, and Quinn (`anim_assassin_idle_2`, `anim_assassin_Walk`, `anim_assassin_run`; `anim_assassin_idle_1` is a crouch) and the Blacksmith mesh for Alex and Max. Those catalog meshes are replaced when the actor applies its visuals. The event roster is Sam, Alex, Max, Morgan, and Quinn. The selection starts as Sam and Alex, and only that selection walks, up to the current max squad size. That cap starts at 0 and is raised by town events; Dragongate raises it to five. Offsets come from the active Battle Plan. HUD lists each companion name and distance.

**Squad camera** (SC1-0025/0026/0084): the Captain spring-arm shifts `TargetOffset` toward the Squad center and may lengthen so Companions stay in frame (`bFrameCompanions`; implementation in `SolidSquadCamera.cpp`). SC1-0026 uses screen-space fit, disables boom collision while Companions are present, and zooms in much slower than out. SC1-0027 lifts the camera via `SocketOffset` when the predicted camera point would sink below procedural terrain.

## Create the open-world map

Binary `.umap` assets are created in the Editor (not checked in as source):

1. **File → New Level → Open World** (World Partition).
2. Save as `Content/Maps/L_OpenWorld`.
3. Place a **Player Start** near the origin (or on the landscape).
4. Confirm **Project Settings → Maps & Modes** (or edit `Config/DefaultEngine.ini`):
   - Editor Startup Map / Game Default Map → `/Game/ThirdPerson/Lvl_ThirdPerson` (horizon test; Open World map still available)
   - Default GameMode → `/Game/Characters/BP_SolidGameMode`
5. **Play** (PIE).

`Config/DefaultEngine.ini` currently uses `Lvl_ThirdPerson` and `BP_SolidGameMode`. After you create `Content/Maps/L_OpenWorld`, you can point the startup maps there; until then the Third Person map is the intentional default.

World Partition and Large Worlds are enabled in project config for open-world scale.

## Project layout

```
SolidCore1.uproject
Config/
Content/
  Characters/          # BP_SolidCharacter, BP_SolidGameMode
  Maps/                # optional L_OpenWorld (+ World Partition externals)
  Viking/              # Fab Viking mesh + locomotion clips (see Fab assets)
  WorldMap.txt         # ASCII 128×64 biome overlay + color key
tools/
  fab-assets.json      # Fab listing IDs, plus recorded paths once a search finds them
  fab_manifest.ps1     # read or update one recorded path
  fab_doctor.bat       # recorded path first, then search; write back a find
  fab_mesh_info.bat    # list meshes and animations in a downloaded pack
  build.bat            # SolidCore1Editor Win64 Development
  run_automation_tests.bat
Source/
  SolidCore1.Target.cs
  SolidCore1Editor.Target.cs
  SolidCore1/
    SolidCore1.Build.cs
    SolidCore1.h / .cpp                # Module startup and the log
    Player/
      SolidCharacter.h / .cpp            # Captain core (move/look/zoom/sprint)
      SolidCharacterInput.cpp            # Runtime Enhanced Input factory
      SolidCharacterVisuals.cpp          # Captain mesh + clip selection
      SolidCharacterFormationDrill.cpp   # Square-walk formation drill
      SolidPlayerController.*
      SolidSquadCamera.cpp               # Squad framing + terrain boom lift
      SolidClickMove.h                   # Raven-sight click-to-move ground ray
      SolidSight.h                       # True sight / raven sight toggle
      SolidCameraFog.h                   # Boom scale that keeps the camera over Fog == 0
    Character/
      SolidNameLabel.*                   # Floating nameplates
      SolidClipLocomotion.*              # Shared single-node clip apply/play
      SolidCharacterStage.*              # Plain room for the character editor
    Game/
      SolidGameMode.*
      SolidGameState.*                   # FGameState: squad, sight, journal, and inventory
      SolidGameInstance.*                # Captain, squad, events, and the world-time clock across the cave trip
      SolidWorldTime.h                   # Journal date and clock
      SolidSessionSave.h                 # How many play sessions have started
      SolidBuildId.h                     # SC1-NNNN shown on the debug HUD
    Content/
      SolidMaterials.*                   # Shared FlatCol solid-color MID helper
      SolidContentLookup.*               # Recorded Fab path, then Viking/grass by name and companion packs by folder
      SolidFabManifest.*                 # tools/fab-assets.json recorded paths
      SolidContentPaths.h                # Canonical BP soft-class paths
    Companion/
      SolidCompanionCharacter.*          # Companion NPC follower
    Fog/
      README.md               # FoW history, rejected approaches, invariants
      SolidTerrainFog.*       # FoW bands, boundary mesh, materials, height-fog helpers
    Terrain/
      SolidTerrainTypes.h
      SolidWorldMap.*         # Parse/scale Content/WorldMap.txt; location 0 centroid
      SolidRiver.h            # 4 m river channel and pond
      SolidRoad.*             # Trail, track, road, and highway routes
      SolidBridge.*           # Ramp, flat center, and ramp where a road crosses a river
      SolidTerrainMap.*
      SolidTerrainNoise.h
      SolidTerrainChunk.*
      SolidTerrainStreamer.*
      SolidTerrainMaterials.*   # Grass + per-biome FlatCol resolve
      SolidTerrainWorldSubsystem.*
    Vegetation/
      SolidTree.*             # Placeholder cylinder+cone tree
      SolidForestTrees.*      # Random Forest-biome tree placement
    Towns/
      SolidBuilding.*         # Grey cuboid + red gable roof
      SolidTownBuildings.*    # Non-overlapping town building pack
      SolidTownSign.*         # Pole, board, "Welcome to" over the town name
      SolidTownSigns.*        # One sign per named town, at the middle of its cells
      SolidTowns.h            # Town names, type, and welcome-sign position, keyed by the WorldMap index
      SolidWallgate.*       # Stone wall and two towers for a WallGate town
    Strongholds/
      SolidStronghold.h       # Type (Dungeon, Fortress, Tower), size (Meagre … Imperious), and the shelf each defined stronghold sits on
      SolidStrongholds.*      # Grass face of a map cell, and the sizes of a cliff frame
      SolidDungeonPassage.h  # Click on the dark opening to enter or leave
      SolidDungeonLevel.h        # Travel options; captain only; room bounds
      SolidStrongholdGrid.h         # 1.2 m pointy-top hexes shared by every stronghold floor
      SolidTowerStronghold.h        # Tower plans. Average through Imperious, with Impressive, Grand, and Stately on the steps between. Each floor may shift its hex grid by at most 30 cm
      SolidDungeonExterior.*        # Dungeon exterior: a rock with one dark mouth, centered on the stronghold cell
      SolidTowerExterior.*          # Tower exterior: smooth stone courses, a flat cap, and a dark door, centered on the tower cell
      SolidTowerInterior.*         # Tower visit: discs, arrival holes, flat landings, floor numbers, rails, treads, yellow grid, and the door that leaves
      SolidStrongholdFurniture.h    # Furnishing catalog, and the pieces each stronghold uses
      SolidStrongholdSite.*         # A stronghold site, drawn as a stone block. A click asks for a type when that size is unlocked and larger
      SolidStrongholdTransition.h   # The move: which types are offered, where the furniture goes, and which old entrance stays shut
      SolidFurnishing.*             # One placed piece: hearth, bed, or chest
      SolidDungeonInterior.*     # Irregular fourteen-hex Meagre floor
    Lairs/
      SolidLairEntrance.*     # Cliff frame and solid dark opening for an Iglin tunnel end
    HUD/
      SolidHUD.*              # Debug HUD, battle-plan panel, journal and inventory boards, menu and credits drawing
      SolidHudTheme.h         # Oak-and-bronze frame
      SolidPointer.h          # In-game mouse pointer: two head triangles, tip on the hotspot, no tail
    Events/
      SolidEvent.h            # Event type, change helpers, and the builders for a town, a bridge, and a closed message
      SolidEvents.h           # The shipped event list
      SolidMessages.h         # Message ids and text
    Journal/
      SolidJournalEntry.h     # One line: Event or Stronghold
      SolidJournal.*          # Journal page: time column, entry column, larger board
    Inventory/
      SolidItemType.h         # Item catalog: id, name, kind, quality, and max stack
      SolidStorage.h          # Cave chest slots; rusty sword, old boots, 20 coins, five sticks, and three pitch
      SolidInventory.*        # Worn boxes and pack store those ids; slot names above the boxes; four crafting slots; Craft spends a known recipe into the first empty pack cell; drag between slots and the chest; starting kit; clear to Rags; / to open
    Crafting/
      SolidRecipe.h           # Recipe: name, output item, inputs with counts, known flag; Torch takes one stick and one pitch
    Menus/
      SolidCharacterEditor.*  # F10 character editor: page cycle, height build and gender sliders, Sam and Alex colors
      SolidMainMenu.*         # F10 menu entries (Use portal, Test Drill, Journal, Credits, Build squad, Enter cave, Character editor, Inventory)
      SolidSquadBuilder.*    # company and plan lists, drag onto squad boxes and F1–F8, Confirm closes the page, a stronghold leave opens it when there is something to arrange, locked until two companions are allowed
      SolidPortal.h           # Town list, Iglin tunnels row, and squad landing
      SolidMenuPointer.h      # Pointer hit targets for the menu and portal list
      SolidCredits.*          # Credits page copy
    Tests/
      SolidTerrainTestHelpers.h   # Shared MakeSmallMap fixture
      SolidWorldMapTests.cpp
      SolidRiverTests.cpp
      SolidRoadTests.cpp
      SolidTerrainFogTests.cpp
      SolidTerrainFogMeshTests.cpp
      SolidTerrainMapTests.cpp
      SolidTerrainMapMoreTests.cpp
      SolidTerrainNoiseTests.cpp
      SolidTerrainTypesTests.cpp
      SolidVegetationTests.cpp
      SolidTownSignTests.cpp
      SolidTownsTests.cpp
      SolidWallgateTests.cpp
      SolidStrongholdTests.cpp
      SolidDungeonLevelTests.cpp
      SolidStrongholdGridTests.cpp
      SolidStrongholdFurnitureTests.cpp
      SolidStrongholdTransitionTests.cpp
      SolidGameModeTests.cpp
      SolidCompanionTests.cpp
      SolidTerrainStreamerTests.cpp
      SolidClipLocomotionTests.cpp
      SolidBuildIdTests.cpp
      SolidPointerTests.cpp
      SolidClickMoveTests.cpp
      SolidCharacterEditorTests.cpp
      SolidMainMenuTests.cpp
      SolidInventoryTests.cpp
      SolidItemTypeTests.cpp
      SolidStorageTests.cpp
      SolidRecipeTests.cpp
      SolidPortalTests.cpp
      SolidMenuPointerTests.cpp
      SolidSightTests.cpp
      SolidContentDependencyTests.cpp
tools/
  run_automation_tests.bat
```

## Requirements

- Unreal Engine **5.8** at `C:\Program Files\Epic Games\UE_5.8`
- Visual Studio 2022 with **Game development with C++** (MSVC, Windows SDK)

## Notes

- This repo is source + config only. `Binaries/`, `Intermediate/`, `Saved/`, and `.sln` are gitignored and generated locally.
- Optional: create Blueprint subclasses of `SolidCharacter` / `SolidGameMode` for content-driven tuning without changing C++. Keep names `BP_Solid*`.
