# SolidCore1

UE **5.8** C++ open-world starter with World Partition–oriented config, a Viking captain, and Enhanced Input (move / look / jump / sprint).

## To-do

- Better mouse pointer
- Use white material for 100% fog terrain
- Don't render trees or buildings on 100% fog terrain, generate it when fog is removed
- Improve building and tree models
- HUD / Menu panel borders & themes, from FAB
- Only move companions when captain is moving
- Test Drill should end with party in Parade formation, and add both companions if not already in party 
- Extend SolidSight.h with Falcon Sight and Eagle sight, both 3rd person but with increased maximum height (not horizontal distance)
- More terrain textures from FAB
- Only show sight indicator after Raven Sight unlocked
- Hide battle plan menu if no companions in party
- Scrollbar on the journal
- Replace FAB_Doctor with an automated test
- "Hold" battle plan that has the companions not move when the captain does
- Events auto-triggering other events
- Reorder game menu items
- Stamina: moving depletes stamina, better roads deplete it slower
- Basic monsters (needs FAB asset download); no combat yet
- Basic NPCs
- Pathfinding to use bridges

## Done

- **Captain + Party** — Viking pawn (Enhanced Input: move / look / zoom / jump / sprint), clip locomotion (idle / walk / run / jump), floating nameplates. The party starts empty. Entering Relion adds **Sam**; entering Kanfold adds **Alex** as well. Companions follow in battle-plan slots; the party camera frames the group and lifts off terrain. The captain's nameplate stays hidden until a companion is with him.
- **Company roster** — the company lists Sam, Alex, Max, Morgan, and Quinn. The catalog meshes are Viking (the captain), Assassin, and Blacksmith. Sam, Morgan, and Quinn use Assassin. Alex and Max use Blacksmith. On spawn, a companion replaces that catalog mesh with a low-poly body on the Viking skeleton. The captain stays on the Viking sculpt. Each companion stores a matte shirt color, a matte trouser color, and a light pink skin color for the head and hands. Sam's shirt is red and his trousers are blue. Alex's shirt is blue and his trousers are red. A face is drawn on the front of the head. Each companion also has a vocation: Warrior, Rogue, Mage, or Cleric. Sam is a Rogue and Morgan is a Mage. The nameplate adds that initial in brackets, such as Sam (R). The party remembers who is selected from that list (Sam and Alex to start). Only as many as the current party limit walk with the captain. Dragongate raises that limit to five. Build party can add the rest; the selection still starts as Sam and Alex.
- **Battle plans** — Company catalog and Party assigned slots. F1 Line, F2 Column, F3 Tight mob, F4 Loose mob, F5 Parade (companions in front, facing the captain, far enough to stay in his view). F5 first fires **Enter Iglin** if it has not already, then **Enter Relion** and **Enter Dragongate**, so a fresh launch still has companions, raven sight, and a repaired Iglin bridge. The captain stays where he is. F1–F8 select; HUD battle-plan panel. The square-walk formation drill is the main menu's Test Drill entry and still cycles F1–F4.
- **World + biomes** — 128×64 `WorldMap.txt` overlay (Sea, Grassland, Town, Mountain, Forest, Desert, River, Cave) scaled onto a 513×513 TerrainPoint map (1024 m side, 200 cm spacing). The world stays square. Each cell is 8 m east–west and 16 m north–south. Numbered locations are towns: `0` Iglin (start), `1` Relion, `2` Kanfold, `3` Visolar, `4` Jethan, `5` Dragongate (a stone wall across its cells, following the long axis of that town, with a 4 m gate and a tower each side; the wall is 4 m high and the towers stand above it), `6` Rathen (on the north bank of the southern river). Two digits side by side on a row are one location number and still two cells of ground. The captain starts at location 0. Per-biome height shelf (Sea/River 0, Mountain +10 m, else +1 m). Darker mountain grey. `C` is the cave stronghold and uses the Mountain shelf. A `*` is a 4 m channel through that square, or a 4 m pond when it stands alone. It joins an orthogonal river or the sea and stays 4 m wide. The rest of the square takes the biome beside it, so the banks can differ, and those banks take that biome's shelf. Where two biomes meet, the slope uses the texture of the higher side, so a mountain cliff is rock rather than the river below it.
- **Lairs** — closed visits, apart from the open world, for fights that are still to come. The first is the **Iglin tunnels**, a stone maze from the mouth south of Iglin (`X`) to the mouth east of Relion (`Y`), so the broken bridge can be left alone. For early testing the two mouths are also joined by a straight run, and the side passages stay. Each mouth is the cave's stone frame on its own cliff, facing the grass beside that cell. A click on the dark opening enters, and a click on an opening inside leaves by that end. The captain goes alone. Companions stand on the grass in front of that mouth when he leaves. The mouths stay shut until Driven from the village has fired. Until then a click does nothing. F10 Enter cave does not make this trip. The portal list can stand the party on the grass just north of the X mouth, and that records Driven from the village. Design notes for later lairs are in `Content/Lairs.md`.
- **Cave stronghold** — the company's first base, a Meagre Dungeon. A stronghold has a type (Dungeon, Fortress, Tower) and a size (Meagre, Humble, Average, Impressive, Grand, Stately, Imperious). A stone mouth (dark opening, rock jambs and lintel) stands against the cliff between the `C` cell and the grass next to it, facing that grass, with the captain's standing spot on the grass in front of the stone. The cave starts closed. **Enter cave** and a click on the mouth do nothing until the captain closes the message from Reach broken bridge. Closing it opens the cave and sends him inside. After that, **F10, 6 Enter cave** and the mouth both load the interior: an irregular den of fourteen hexes, captain only. The entrance is wider than the captain. The same dungeon code will serve every size; Meagre is the cave, and a Meagre stronghold has a single floor. A yellow hex grid covers that floor, 1.2 m from flat side to flat side, and the same grid will serve every stronghold type. Furnishings come from one shared list. The cave uses three of them, on level 0: a grey fireplace with a lower red hearth (one hex), a beige bed with a white pillow narrowed so it sits inside its two hexes, and a beige chest with a rounded lid (one hex). The chest is a storage container. It holds a rusty sword, old boots, 20 coins, five sticks, and three pitch. Inside the cave, the inventory page shows that chest. A drag moves an item between a worn box, a pack cell, and a chest cell, and an occupied destination swaps. Outside the cave the chest stays off the page and its contents remain. The captain's nameplate stays hidden in the stronghold. Companions are removed with the open world and spawn again beside him at the mouth on **Leave cave**. The third-person playground stays on that map, and its colorway blocks (the green shapes, and the other colors) are hidden for the visit. The party builder stays closed. A left click on the dark opening enters from the cliff and leaves from inside. While the pointer rests on that opening it reads **Enter cave** or **Leave cave**. **F10, 6** still makes the same trip. The Meagre Dungeon's visit view aims the camera level: into the den on entry, and out over the grass on leave.
- **No starter monolith** — the grey slab that marked the start-town centroid is gone. Location 0 still sets the captain spawn and the building clear radius.
- **Fog follows the player** — the map starts fully fogged. After the captain is placed at location 0, exploration fog is centered on the pawn, not on that cell. The two match only because that is where the pawn stands. Curtains are 110 m (half) and 160 m (full), above the 50 m max zoom. The camera boom shortens so it cannot sit above terrain with fog greater than 0.
- **Procedural terrain** — streamed chunks around the pawn, Fab grassland material, FlatCol tints for other biomes. Exploration fog-of-war (25 m / 50 m bands, marching-squares curtains) clears from the trail, not the camera.
- **Town + forest** — random non-overlapping town buildings (grey cuboid + red gable prism roof); forest trees scattered on Forest cells. Dragongate is not that pack: a 4 m stone wall runs across its cells, with a 4 m gap and a taller tower on each side of the gap. A row of cells runs east–west and a column runs north–south. Buildings stay on cells marked `o`, Z, or a location digit, clear of the start-town centroid and of each welcome sign.
- **Roads** — `WorldMap.txt` lists each road as a start, an end, and a type. A trail is 1 m wide, a track 2 m, a road 3 m, and a highway 4 m. The route is solved across the grid and painted in a brown close to the town colour. Houses stay in the marked towns, not on that strip. Every route stays off mountains and the sea. Roads and highways also stay off forest and desert, and they cross a river only when going around costs more. The water in a crossing keeps the river texture. The road runs straight across that water and along both banks, and it does not leave a spur past the bend on the far bank. A trail or a track may cross forest or desert; when two routes are the same length, open ground is preferred.
- **Bridges** — wherever a route crosses a river, a bridge stands on that water: an up ramp, a flat center, and a down ramp. The center is as wide as that road and as long as the water, with a short rest on each bank, and it is its own mesh. Each bridge has a broken flag, false unless that route joins Iglin and Relion. That crossing starts with the center undrawn, and each ramp ends in a solid face. The gap in front of that face cannot be walked. Entering Relion repairs it: the center is drawn, and the crossing can be used. Its ramps still meet each bank. Every other crossing keeps its center. Each ramp runs down to its own bank and continues into the ground, so a high bank and a low bank get different lengths. The walking surface uses the road brown, and it is the triangle surface, so a character walks up a ramp, across an intact center, and down the other side. The bridge follows the painted strip, which lies half a terrain step southwest of the cell centres. The water underneath keeps the river texture. Characters stop at the sea, at a mountain, and at the river channel. The channel is the 4 m width, so the rest of a river cell can still be walked. An intact bridge is the only way across that channel.
- **Town signs** — a named town gets one welcome sign at the middle of its cells: one thin dark-brown pole, a flat light-brown board, white letters with a black border. "Welcome to" is the first line and the town name is the second. The board faces south, on that middle, unless that town sets another spot. A single cell's middle is the cell center. Dragongate's sign stands on the north edge of the town, east of the gate, and faces north, so it can be read while facing south. The gate stays clear. Kanfold's sign stands between its two cells.
- **True sight / raven sight** — the game starts in true sight (first person). Raven sight (third person) stays locked through Iglin, the broken bridge, the monster close, and the return to the village. It unlocks when the captain enters Relion, and also at Kanfold or Dragongate. F9 then toggles. Switching from raven sight to true sight levels the camera, so a downward raven look does not carry into first person. True sight hides the captain's body and nameplate, turns the body with the look direction, and ignores mouse-wheel zoom until you switch back. The menu and the formation drill ignore F9. A translucent oval in the top-right shows an eye in true sight and an eagle in raven sight.
- **Events** — a list of one-shot triggers. Enter town fires when the captain steps onto a numbered WorldMap cell; the parameter is that index. Each event has an eight-digit id and an act. The act numbers are the constants ACT_0, ACT_1, ACT_2, ACT_3, and ACT_4. Enter Iglin, Reach broken bridge, and Killed by monsters are ACT_0. Driven from the village is ACT_1. Enter Relion is ACT_2, Enter Kanfold is ACT_3, and Enter Dragongate is ACT_4. Messages are a separate list, each with an eight-digit id. ShowMessage names one. MessageClosed fires when that message is closed. An event can require other ids, and it does not fire until those have. Entering town 0 (Iglin) is recorded and titles the captain Orphan. Reach broken bridge requires Enter Iglin. It fires when the captain stands on either ramp of the Iglin bridge, including after that bridge is repaired. It shows the message "You are killed by monsters". The deck and the gap past the lip do not count. Standing on a ramp before Iglin changes nothing and is not recorded. The next time he stands there after Iglin records it and shows the message. A click, Enter, Esc, or F10 closes that message. Killed by monsters then fires, opens the starting cave, sends the captain inside, and clears his inventory, including the pack, leaving Rags on the body. Driven from the village requires that close. It fires the next time the captain enters Iglin, shows "You are driven from the village", titles the captain Outcast, and opens the Iglin tunnel mouths. It does not send him inside. The portal to the grass north of the X mouth records that same event without waiting for the close. A return to Iglin before that close changes nothing extra and is not recorded. Entering town 1 (Relion) requires Enter Iglin. It then enables raven sight, sets max party size to 1 (Sam), titles the captain Vagrant, and repairs the Iglin bridge. A visit to Relion before Iglin changes nothing and is not recorded. The next visit after Iglin applies it. Entering town 2 (Kanfold) enables raven sight, sets max party size to 2 (Sam and Alex), and titles the captain Ranger. Entering town 5 (Dragongate) enables raven sight, sets max party size to 5 (the whole company), and titles the captain Captain. The nameplate starts as Outcast. F5 does not fire Reach broken bridge. F5 does visit Iglin, so after the close it shows the village message too.
- **Journal** — F10 menu, **3 Journal**. The page is a large board. The name sits in the top border. The columns are headed **Time** and **Summary**, such as `01:24, 200/53 AtF` beside `Enter Iglin`. A line is an Event, a Stronghold, or a Lair. Events are listed in the order they fired, with what each one changed under the entry. Iglin is recorded as soon as the captain is placed there, with Title Orphan under the name. Reach broken bridge is recorded once the captain stands on an Iglin ramp after Iglin, with the message under the name. Killed by monsters is recorded when that message closes, with Cave enabled and Inventory cleared under the name. That trip also records Enter Stronghold. Driven from the village is recorded the next time the captain enters Iglin after that close, and when the portal stands him north of the X mouth, with the message and Title Outcast under the name. Entering or leaving a stronghold, including the cave, is `Enter Stronghold` or `Exit Stronghold`. The line does not name the size or type; an upgrade will be its own entry. Entering or leaving the Iglin tunnels is `Enter Iglin tunnels` or `Exit Iglin tunnels`. The same trip can be recorded again. A new play session advances the date by one day, from **200/45 AtF**. Days run 0–99, then the year increases. Hours and minutes match real time since that session started and do not roll the date. The cave trip keeps the same day and the same clock. Esc or F10 closes the page.
- **Inventory** — F10 menu, **8 Inventory**, or `/`. The page is the same oak-and-bronze board as the journal. The name sits in the top border. On the left, boxes follow a person: head, body, and boots in a column, the hands beside the body, and a narrower belt between the body and the boots. Head, Body, Off hand, Main hand, Belt, and Boots are headings above those boxes, in the same letters as Pack and Chest. On the right, the captain's pack is six boxes, two columns by three rows, under the heading Pack. The items are Rags (armor, Inferior), a dented helmet (helmet, Inferior), a crude spear (weapon, Inferior), and a torch (tool, Common). Each of them stacks to 1. A worn box or a pack cell stores one of those ids, and the page shows its name. Armor can be dragged only onto the body. A helmet can be dragged only onto the head, boots only onto the boots, and a belt only onto the belt. Any item can be dragged into the pack or a chest. A rusty sword is an Inferior weapon. It stacks to 1. Old boots are Inferior and stack to 1. Coins are a material with no grade and stack to 999. A stick is an Inferior material and stacks to 10. Pitch is a Common material and stacks to 10. The cave chest is two columns by three rows. It starts with the rusty sword, the old boots, 20 coins, five sticks, and three pitch. The last cell is empty. A stack above one shows that count after the name. Press a worn box, a pack cell, or a chest cell that holds an item, and release on another. An occupied destination swaps when both items fit. The chest is on the page only inside the cave, so a drag back into it happens there. A new game wears the dented helmet, Rags, and the crude spear in the main hand. The off hand, the boots, the belt, and the pack start empty. Closing the monster message empties every worn box, the pack, and the crafting slots, then puts Rags back on the body. On the bottom right, four crafting slots sit in a row, with a small bronze Craft button beside them. An item can be dragged into a crafting slot, and back out. Each such drop checks the known recipes. The button lights when one of them is satisfied by those slots and the pack has an empty cell. Otherwise it stays dim. Pressing it spends those ingredients, clearing a stack that reaches zero, and puts one crafted item in the first empty pack cell. The button stays lit when the slots can still make that recipe and the pack still has room. `/` again, Esc, or F10 closes the page. Move, look, and battle-plan keys wait while it is open.
- **Crafting** — a recipe has a name, the item type it makes, the inputs and how many of each, and whether the player knows it yet. The list starts with Torch. It is known. It takes one stick and one pitch, and it makes one torch.
- **Oak and bronze** — the journal, inventory, credits, main menu, and battle-plan list are a dark oak board in a bronze binding, with brighter corner bosses. The debug readout in the corner stays a plain block.
- **Debug HUD** — F12 hides and shows the tech readout (build, FPS, pawn, terrain). The battle-plan list stays up. It starts visible.
- **Mouse pointer** — a bronze arrow drawn in the game, tip on the hotspot. The Windows cursor stays hidden. Hold the right mouse button to look; the pointer hides until you let go, then it is back where you pressed. The right stick still looks.
- **Click to move** — raven sight only. One left click on the ground walks the captain there in a straight line, then he stops. WASD and the left stick do nothing in raven sight. In true sight a click does nothing: he moves only with WASD or the left stick. Switching back to true sight cancels a walk that was in progress. A walk stops at the sea, a mountain, or the river channel.
- **Main menu** — F10 opens it. **1 Use portal** lists the towns and then the Iglin tunnels. A town moves the party onto that town and fires its enter-town event. Iglin tunnels stands the party on the grass just north of the X mouth and records Driven from the village. **2 Test Drill** runs the formation drill. **3 Journal** opens the journal. **4 Credits** opens the credits page (Silvatek, Cursor + Grok, Fab Viking, grass, Assassin, and Blacksmith). **5 Build party** lists the company. It stays dim until events allow at least two companions. Each row toggles that companion in or out, up to the current party limit. **6 Enter cave** stays dim until Reach broken bridge opens the cave. It then loads the Meagre Dungeon interior for the captain alone, and reads **Leave cave** inside. **7 Character editor** shows one companion on a plain full-screen background. Page Up and Page Down cycle Sam, Alex, Max, Morgan, and Quinn. Up and Down select height, build, or gender. Left and Right change the selected value, and the pointer can drag that slider. A panel in the corner lists that companion's proportions and shirt, trouser, and skin colors as numbers. F10 or Esc returns to the open world. **8 Inventory** opens the inventory. Up/Down, Enter, and the pointer also work on the menu: resting the pointer on a row highlights it, and a left click chooses it. The same pointer works on the portal list and the party builder. Esc or F10 closes. Move, look, and battle-plan keys are ignored while the menu, the portal list, the party builder, the journal, the inventory, the credits page, or the character editor is open.
- **Content pipeline** — Fab listings restored via Launcher Add to Project. Recorded paths in `tools/fab-assets.json` are tried first. A miss falls back to a search: Viking and grass by name under `/Game/Viking` or `/Game/Fab`, companion bodies `Meshes/base_mesh/SKM_base_mesh_ue` inside `Medieval_Assassin_Girl` and `Mesh/SK_Elf_Blacksmith` inside `Elf_Blacksmith`, anywhere under Content. A find is written back into the manifest. `tools/fab_doctor.bat`. Packs are gitignored (attribution README kept).
- **Automation** — `SolidCore1.*` editor tests; `tools/run_automation_tests.bat` prints failed test paths. Build ID `SC1-NNNN` on the debug HUD.
- **Editor build** — `tools\build.bat` compiles `SolidCore1Editor` Win64 Development (`-WaitMutex`). `UE_ROOT` overrides the engine install; `PROJECT` overrides the uproject. Both default from the script location.

## Controls (runtime defaults)

| Action | Keyboard | Gamepad |
|--------|----------|---------|
| Move | WASD (true sight) | Left stick (true sight) |
| Look | Right mouse button (hold) | Right stick |
| Pointer | Mouse | — |
| Click to move | Left click (raven sight) | — |
| Zoom | Mouse wheel | — |
| Jump | Space | A / Cross |
| Sprint | Left Shift | Left stick click |
| True sight / raven sight | F9 (after raven sight is enabled) | — |
| Main menu | F10 | — |
| Inventory | / or F10 then 8 | — |
| Debug readout | F12 | — |
| Menu up / down | Up / Down, or point at a row | — |
| Menu confirm | Enter, 1–8, or left click | — |
| Character editor | F10, then 7 | — |
| Previous / next companion | Page Up / Page Down, while the character editor is open | — |
| Select height, build, or gender | Up / Down, while the character editor is open | — |
| Change the selected attribute | Left / Right, or drag the slider, while the character editor is open | — |
| Portal | Enter, 1–8, or left click, while the portal list is open | — |
| Close menu / portal / party / journal / inventory / credits / character editor | Esc or F10 | — |

Input Actions / Mapping Context can be replaced with Content assets on the character later; if unset, C++ creates transient defaults so PIE works immediately.

## Automated tests

UE Automation tests live under `Source/SolidCore1/Tests/` (editor builds, `WITH_DEV_AUTOMATION_TESTS`).

### Policy

1. **All existing tests must keep passing** when you change product code — **without modifying those tests**.
2. If a change **must** update an existing test (API rename, intentional behavior change), say so explicitly in the commit / change description and why.
3. **New product code ships with tests in the same commit** (same PR/change set). Prefer pure logic / `NewObject` unit tests; add heavier PIE tests only when needed.
4. **Never pass `TObjectPtr<T>` to `TestNotNull` / `TestNull`.** UE 5.8 cannot deduce `const ValueType*` from it (`GetStaticMesh()` is the usual trap). Bind a raw `T*` first, or call `.Get()`.
5. When a product change would invalidate an assumption in an existing test, remove that assumption **before** the product change. The test has to pass on the current code and on the code that is about to be written. Read a constant the product already defines, such as `USolidWorldMap::MapWidth`, instead of copying a number.
   1. Name the tests that assume the old behavior.
   2. Update those tests only. Do not change product code in that commit.
   3. That commit is pushed on its own, so the tests can be run against the current build.
   4. The product change comes next and does not edit the tests.
   5. The same tests are run again against the new build.

### Suite (filter `SolidCore1`)

| Filter | Covers |
|--------|--------|
| `SolidCore1.Fog.*` | Distance bands, units, mist sampling, fog mesh build guards, camera stays on clear ground, curtain above max zoom |
| `SolidCore1.Map.*` | Build smoke, trail clear, idempotent build, sampling, bounds, biome Z shelf |
| `SolidCore1.WorldMap.*` | ASCII overlay load, key colors, numbered locations, two digits on a row are one location, sea is `~`, grass is `.`, a town cell is `o`, forest is `#`, a river is `*`, desert is `^`, `X` and `Y` use the mountain shelf and are not towns, 1024 m square world, cell size follows the map width and height, fog follows the player |
| `SolidCore1.River.*` | 4 m pond, orthogonal joins to river or sea, banks take the adjacent biome; a road bank uses the road texture |
| `SolidCore1.Road.*` | Trail, track, road, and highway widths; every route stays off mountains and the sea; roads avoid forest, desert, and extra river cells; a crossing runs straight onto both banks with no spur past the far bend; the strip is a road brown and a crossing keeps the river; `o`, Z, and location digits are towns |
| `SolidCore1.Bridge.*` | A crossing gets an up ramp, a flat center, and a down ramp; the center matches that road's width and that river's span and lies on the painted strip; each ramp runs down into its own bank; the pieces collide as triangles so a character can walk them; the Iglin–Relion center starts left out and those ramps end in a solid face; repairing it draws the center and still marks that crossing; other crossings keep theirs |
| `SolidCore1.GroundMove.*` | Sea, mountain, and the river channel block a step; the rest of a river cell stays open; an intact bridge is the only crossing; a broken center does not open the channel; the gap in front of a broken ramp stays closed; a diagonal step slides along the edge; a step already on closed ground may only leave; a ramp is the ramp surface, not the deck or the gap past the lip |
| `SolidCore1.Towns.*` | Town names and type (Town or WallGate); Dragongate is the wall gate, with a 4 m gap and two equal towers; a two-cell gate follows the long axis |
| `SolidCore1.Stronghold.*` | Types and sizes; the `C` cell is a Meagre Dungeon; the mouth faces the grassland neighbor; the cave starts closed until the monster message is closed; F10 cave travel keeps companions outside; a click on the dark opening enters and leaves once the cave is open; the Meagre Dungeon visit view levels the camera into the den and out over the grass; the Meagre den is fourteen irregular hexes; a furnishing catalog places the fireplace, bed, and chest on that single floor; the chest is storage and the fireplace and bed are not |
| `SolidCore1.Lair.*` | Iglin tunnels: `X` south of Iglin and `Y` east of Relion use the cave mouth on their own cliffs; the maze joins those ends, including a straight run for early testing, and has dead ends; the mouths stay shut until Driven from the village; a click enters and leaves by the end that was hit |
| `SolidCore1.Noise.*` | Hash / value / fBm / height / grass tone |
| `SolidCore1.Types.*` | Biome names (incl. Sea/River), height offsets, border triangles use the higher shelf, `FSolidTerrainPoint` defaults |
| `SolidCore1.Vegetation.*` | Tree RNG variation, town building pack on cells marked `o`, Z, or a location digit, one welcome sign at the middle of each named town; Dragongate's sign stands east of the gate on the north edge, facing north |
| `SolidCore1.GameMode.*` | Default spawn flags, pawn BP resolution |
| `SolidCore1.Companion.*` | Defaults, SetFollowTarget |
| `SolidCore1.Body.*` | Bone roles; a seeded body on the Viking skeleton stays under the polygon budget and keeps shirt, trouser, and skin slots; a name picks a Unicode face |
| `SolidCore1.Company.*` | Viking, Assassin, and Blacksmith meshes; roster of five with vocations, torso colors, and leg colors; party selection; fielded count follows the party limit; selection survives ensure |
| `SolidCore1.Streamer.*` | FindExisting / EnsureExists (null + idempotent) |
| `SolidCore1.Clip.*` | SelectClip idle/walk/run/jump rules |
| `SolidCore1.NameLabel.*` | Style sizes/colors/plates; Outcast / Sam defaults; vocation initial on the companion plate; captain plate hidden when alone or in a stronghold |
| `SolidCore1.BattlePlan.*` | Formation slots; Company/Party assign; selection survives party ensure; drill legs; F5 Parade fires Enter Relion and Enter Dragongate once |
| `SolidCore1.Credits.*` | Author / Cursor+Grok / Fab attribution; HUD toggle |
| `SolidCore1.MainMenu.*` | F10 menu entries (Use portal, Test Drill, Journal, Credits, Build party, Enter cave, Character editor, Inventory) and selection; Enter cave stays dim until the cave is open, and Leave cave stays available |
| `SolidCore1.CharacterEditor.*` | Character editor row, F10 returns to the world, height build and gender sliders, Sam's red shirt and Alex's blue shirt, attribute numbers |
| `SolidCore1.PartyBuilder.*` | Build party stays locked below two companions; toggles the company up to the event cap |
| `SolidCore1.MenuPointer.*` | Pointer hit-tests a menu or portal row; a miss leaves the keys' selection |
| `SolidCore1.Portal.*` | Town list, including Dragongate and Rathen, then the grass north of the Iglin tunnels; keys 1–8; a town landing stays on one cell of that town; the tunnel landing records Driven from the village; portal page |
| `SolidCore1.WorldTime.*` | Session 1 is 200/45 AtF; session 9 at 84 minutes is 01:24, 200/53 AtF; day 100 rolls the year |
| `SolidCore1.Journal.*` | Empty journal, then events in the order they fired, with Time and Summary columns; each line is an Event, a Stronghold, or a Lair, including the captain's title and repeated Enter Stronghold / Exit Stronghold lines |
| `SolidCore1.Crafting.*` | Torch is a known recipe; it makes one torch from one stick and one pitch |
| `SolidCore1.Inventory.*` | Item catalog: Rags (armor), a dented helmet (helmet), a crude spear (weapon), and a torch (tool); the torch and pitch are Common and the other items are Inferior, except coins which have no grade and stack to 999; a stick stacks to 10; each of the first items stacks to 1; the cave chest is two columns by three rows and starts with the rusty sword, old boots, 20 coins, five sticks, and three pitch; slots store those ids and the page shows the name; armor fits only the body, a helmet only the head, boots only the boots, and a belt only the belt; any item can go in the pack or the chest; a drag moves an item between worn boxes, pack cells, and chest cells, and an occupied destination swaps when both items fit; a closed chest refuses the drop; starting kit is the helmet, Rags, and the spear; boots, the off hand, and the belt start empty; Clear empties worn boxes, the pack, and the crafting slots, and leaves Rags on the body; person-shaped worn boxes with those names above them, in the same letters as Pack and Chest; pack is two columns by three rows; four crafting slots and a small bronze Craft button sit on the bottom right; the button lights when a known recipe fits those slots and the pack has an empty cell; pressing it spends the ingredients and puts one result in the first empty pack cell, and it stays lit only while another craft still fits; a drag can move an item into a crafting slot and back out; F10 Inventory and `/` open the page, and Esc or F10 closes it |
| `SolidCore1.HudTheme.*` | Oak-and-bronze frame thickness, corner bosses, opaque board; translucent oval sight marker |
| `SolidCore1.HUD.DebugToggle` | F12 shows and hides the tech readout |
| `SolidCore1.Pointer.*` | Bronze arrow tip, outline, and hide-while-looking |
| `SolidCore1.ClickMove.*` | Single click, raven-only; WASD is true sight; ground ray |
| `SolidCore1.Sight.*` | True sight at start; F9 switches to raven sight and back; entering true sight levels the camera and keeps the yaw; eye and eagle markers |
| `SolidCore1.Events.*` | Enter-town events, Reach broken bridge, Killed by monsters, and Driven from the village: each has an eight-digit id and an act (Iglin, the bridge, and the monster message are ACT_0, the return to Iglin is ACT_1, Relion ACT_2, Kanfold ACT_3, Dragongate ACT_4); messages have their own ids; the ramp shows "You are killed by monsters"; closing it opens the cave, sends the captain inside, and clears his inventory to Rags; Enter Iglin titles the captain Orphan; the next entry to Iglin shows "You are driven from the village", titles Outcast, and opens the Iglin tunnel mouths; Relion and Reach broken bridge wait until Iglin has fired; nothing before Relion enables raven sight; Relion, Kanfold, and Dragongate enable raven sight; max party size through the whole company; captain title through Captain; Relion repairs the Iglin bridge; numbered cells |
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

PIE shows a top-left debug HUD (`Build SC1-NNNN`, a one-line `Change:` note, pawn/terrain Z, chunk load, material, camera pitch). Both strings live in `Source/SolidCore1/Game/SolidBuildId.h` (`SOLID_BUILD_ID` / `SOLID_BUILD_NOTE`) and are bumped on every GitHub push so screenshots identify which binary you ran.

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

`Content/WorldMap.txt` is a 128×64 grid of markers plus a color key (`~` Sea, `.` Grassland, `o` Town, `M` Mountain, `C` Cave, `#` Forest, `^` Desert, `*` River). Digits are location indexes (`0` Iglin, `1` Relion, `2` Kanfold, `3` Visolar, `4` Jethan, `5` Dragongate, `6` Rathen). A single digit is that location. Two digits next to each other on a row are one number, read from the left, and each character is still one cell: `05` is Dragongate across two cells. A longer run is taken in pairs from the left, so `55` is location 55. Town names live in `Towns/SolidTowns.h`, matched by that index. A digit with no entry there has no name. Unlisted digits default to the Town biome. `X` and `Y` are the Iglin tunnel mouths. They are kept as themselves. They use the mountain shelf, so the cliff stays closed, and they are not towns, locations, or the cave. A key line for `X` or `Y` does not change the mountain colour. `C` is not a town: it is a Meagre Dungeon and uses the Mountain biome, so that cell gets the +10 m shelf. The grid scales across the TerrainMap world rectangle (file row 0 = north). The default rectangle is 1024 m on a side. A marker is 8 m east–west and 16 m north–south. A `*` marks a river through that square, not a square of water. The channel is 4 m wide. An isolated `*` is a 4 m pond. A `*` beside another `*`, or beside `~`, joins that neighbor edge to edge and stays 4 m wide however long the square is. A diagonal neighbor does not join. The dry part of the square uses the adjacent cell, so the two banks can differ (grass on one side, mountain on the other, or sea where the river meets it). A `Road:` line under the key is `start -> end, type`. Ends are a town number or `C` for the cave. A trail is 1 m wide, a track 2 m, a road 3 m, and a highway 4 m. The route is solved on the grid and drawn in a brown close to the town colour. Houses are placed only on cells marked `o`, Z, or a location digit. Every route stays off mountains and the sea. Roads and highways also stay off forest and desert, and they pay extra for each river cell so a crossing stays short. Where the strip meets the water, the channel keeps the river texture. Location **0** is the starting town: the streamer relocates the player there. A legacy `Z` marker is the start only on maps that have no `0` cells. Fog bands are centered on the player after that placement.

## Forest trees (SC1-0076 / SC1-0110)

- `ASolidTree` — cylinder trunk + cone canopy; scattered randomly across **Forest** TerrainPoints (`ForestTreeDensity`, capped by `MaxForestTrees`).
- Logic in `Vegetation/SolidForestTrees.*`. Toggle with `bAutoSpawnVegetation`.
- Fog curtains: see [`Fog/README.md`](Source/SolidCore1/Fog/README.md) (clear|fogged ~25 m plus a **white** half→full curtain ~50 m).

## Name labels (SC1-0091 / SC1-0093)

Floating nameplates (`SolidNameLabel`) sit above each Party member and face the view camera: TextRender plus thin cube **border** and contrasting **background** plates. Defaults: Captain **"Outcast"** (larger warm amber on dark plate), Companions muted slate plates. The captain's plate shows in raven sight only, and only when at least one companion is with him outside a stronghold. Enter Iglin renames the captain to Orphan. Driven from the village sets Outcast. Relion renames the captain to Vagrant, Kanfold to Ranger, and Dragongate to Captain. Override via `CharacterDisplayName` / `SetCharacterDisplayName`.

## Battle plans (SC1-0095 / SC1-0101)

- **Company** (`USolidCompany`) owns the catalog of all battle plans. Starter assigned set:
  - **F1 Line** (Standard) — companions on the flanks
  - **F2 Column** (Standard) — file behind the Captain
  - **F3 Tight mob** (Narrow) — close triangle behind
  - **F4 Loose mob** (Wide) — spread triangle behind
  - **F5 Parade** (Standard) — line in front of the Captain, facing him, far enough that the rank fits in his view. Selecting it fires Enter Iglin if needed, then Enter Relion and Enter Dragongate, so the rank, raven sight, and the repaired Iglin bridge are ready even at the start of a session. Those events still fire only once.
- **Party** (`USolidParty`) holds up to 8 **assigned** plans and one **active** plan (default **F1 Line**).
- Each plan has a **formation** plus **spacing** (`Narrow` / `Standard` / `Wide`) that scales follow distances.
- Captain switches assigned slots with **F1–F8** (only filled slots work). The choice stays put when town events refresh the party.
- **Test Drill** (main menu) runs a formation drill: F1 walk 1.5s → F2 / F3 / F4 each blend +90° yaw over 250 ms then walk 1.5s (square path; player move/look suppressed while active).
- HUD shows a **Battle Plans** panel under the tech block: all 8 slots, F-key + name when assigned, active slot highlighted in amber (aligned marker column).
- Engine viewmode debug binds (wireframe/unlit/lit/…) are moved to **Ctrl+F1–F5** in `Config/DefaultInput.ini` so bare F-keys stay free for battle plans. Restart the editor after pulling.

## Companion (SC1-0024 / SC1-0093)

`ASolidCompanionCharacter` follows with simple steering (no NavMesh — works on procedural terrain). The captain stays on `SK_Viking`. A companion wears a low-poly body skinned to the Viking skeleton: a chest, a tapered cylinder over each shoulder, arms, and legs down to the feet at body thickness. The hip cylinder's bottom meets the outside of the thighs. The shirt, the trousers, and the skin on the head and hands are separate matte colors. Those colors live on the companion record and are applied on the actor, so a shared body is not recolored while it is on screen. Sam's shirt is red and his trousers are blue. Alex wears the swap. Both use light pink skin. The F10 character editor shows one companion on a plain background so those colors can be checked without the open world. Space swaps that body for the Viking skeleton, drawn as bone lines. Bones that share a joint use different colours. Home turns the figure clockwise and End turns it the other way. Resting the pointer on a bone shows that bone's name in black. Clothing and weapon bones are left off that view: axe_, sheath_, cloack_, beard_, shield_, and skirt_. Thigh and calf twist bones are left off too. calf and foot stay, so the hip-to-knee line and the knee-to-ankle line are both drawn. Page Up and Page Down cycle the company. Up and Down select height, build, or gender, and Left and Right change that value. The pointer can drag the same sliders. F10 returns to the world. The face is chosen from the same unicode points, including U+1F611. The engine font has no emoji, so the expression is drawn on the front of the head. The body stays lighter than the Viking sculpt. The three sliders run from 0 to 1 and set every proportion. Build 0 is short and wide, and build 1 is tall and thin. Gender 0 is feminine and gender 1 is masculine. Height 0.5 leaves that size; 0 shortens it and 1 lengthens it. At 0.5 on every slider every part stays at its catalogue size and every bone keeps the Viking length. Moving a slider changes those part diameters and lengths, and the bones inside the part take that size. The head and the hands are not parts, so they stay the Viking size. The body is being built one piece at a time. The pieces so far are the two thighs and the two calves. Each thigh is a truncated cone the length of calf_l or calf_r, from the hip to the knee, 20 cm across at the hip and 15 cm across at the knee. Each calf is one cone the length of foot_l or foot_r, from the knee to the ankle, 15 cm across at the knee and 12 cm at the ankle. Each foot is a cylinder the length of ball_l or ball_r, from the ankle to the toe, 10 cm across. Each toe is a truncated cone past the foot, 10 cm across at the end towards the ankle and 5 cm at the toe. The hips are one shirt piece around thigh_l, thigh_r, and spine_01. It is the length of spine_01. The bottom is 50 cm side to side and 20 cm front to back, and the top is 30 cm side to side and 20 cm front to back. Each spine bone above spine_01 is its own shirt piece, still 20 cm front to back, widening from that 30 cm waist to 40 cm side to side at the top of the spine. The shoulders are one shirt piece around clavicle_l, clavicle_r, upperarm_l, upperarm_r, and neck_01. It is the length of neck_01. Its base is the same oval as the top of the spine_03 piece. It widens past the upperarm bones, then narrows to a circle of 20 cm diameter at neck_01. The drawn body is one list of named parts. Each part draws its own triangles, and the list draws the whole character. A part's name is the region it shows, which is often different from the skeleton bone it wraps. Each part stores that bone list, a shirt, trouser, or skin colour, and the side-to-side and front-to-back diameters at the start and the end of its cone. The shoulders also store a mid section, fitted when the piece is drawn. The upper arm is a shirt cone the length of lowerarm_l or lowerarm_r, 8.4 cm at the shoulder and 6.8 cm at the elbow. The lower arm is a shirt cone the length of hand_l or hand_r, 5 cm at the elbow and 6.8 cm at the wrist. The shoulder_jnt, finger, head, and eye bones are still bare, and so are the twist bones. Hovering a piece names the bones in it. The head is opaque. A companion's face still comes from the display name, and that body is built once and reused. They play Viking clips. The company catalog still lists the Assassin mesh for Sam, Morgan, and Quinn (`anim_assassin_idle_2`, `anim_assassin_Walk`, `anim_assassin_run`; `anim_assassin_idle_1` is a crouch) and the Blacksmith mesh for Alex and Max. Those catalog meshes are replaced when the actor applies its visuals. The event roster is Sam, Alex, Max, Morgan, and Quinn. The selection starts as Sam and Alex, and only that selection walks, up to the current max party size. That cap starts at 0 and is raised by town events; Dragongate raises it to five. Offsets come from the active Battle Plan. HUD lists each companion name and distance.

**Party camera** (SC1-0025/0026/0084): the Captain spring-arm shifts `TargetOffset` toward the Party center and may lengthen so Companions stay in frame (`bFrameCompanions`; implementation in `SolidPartyCamera.cpp`). SC1-0026 uses screen-space fit, disables boom collision while Companions are present, and zooms in much slower than out. SC1-0027 lifts the camera via `SocketOffset` when the predicted camera point would sink below procedural terrain.

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
      SolidPartyCamera.cpp               # Party framing + terrain boom lift
      SolidClickMove.h                   # Raven-sight click-to-move ground ray
      SolidSight.h                       # True sight / raven sight toggle
      SolidCameraFog.h                   # Boom scale that keeps the camera over Fog == 0
    Character/
      SolidNameLabel.*                   # Floating nameplates
      SolidClipLocomotion.*              # Shared single-node clip apply/play
      SolidCharacterStage.*              # Plain room for the character editor
    Game/
      SolidGameMode.*
      SolidGameState.*                   # FGameState: party, sight, journal, and inventory
      SolidGameInstance.*                # Captain, party, events, and the world-time clock across the cave trip
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
      SolidStronghold.h       # Type (Dungeon, Fortress, Tower) and size (Meagre … Imperious)
      SolidStrongholds.*      # Mouth on the grassland face of a C cell
      SolidDungeonEntrance.*  # Rock frame and solid dark opening
      SolidDungeonPassage.h  # Click on the dark opening to enter or leave
      SolidDungeonLevel.h        # Travel options; captain only; room bounds
      SolidStrongholdGrid.h         # 1.2 m pointy-top hexes shared by every stronghold floor
      SolidStrongholdFurniture.h    # Furnishing catalog, and the pieces each stronghold uses
      SolidFurnishing.*             # One placed piece: hearth, bed, or chest
      SolidDungeonInterior.*     # Irregular fourteen-hex Meagre den
    HUD/
      SolidHUD.*              # Debug HUD, battle-plan panel, journal and inventory boards, menu and credits drawing
      SolidHudTheme.h         # Oak-and-bronze frame
      SolidPointer.h          # In-game mouse pointer
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
      SolidMainMenu.*         # F10 menu entries (Use portal, Test Drill, Journal, Credits, Build party, Enter cave, Character editor, Inventory)
      SolidPartyBuilder.*    # company list, locked until two companions are allowed
      SolidPortal.h           # Town list, Iglin tunnels row, and party landing
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
