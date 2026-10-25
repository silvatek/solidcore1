# Lairs

Design notes for later lairs, then the tunnels that already ship. Read this before changing lair code. Human play notes stay in `README.md`.

The bullets below are the direction. They are not the rules of the Iglin tunnels. Applying them to that lair now would undo a playthrough that already worked.

## Later lairs

- A lair will generally have an entrance and an exit.
- An exit is locked from the outside. The captain can leave through it. He cannot enter there.
- Most lairs will be generated, with a different layout each playthrough.
- A lair may have more than one level.
- A level is rooms and corridors.
- Each level has a size, the x and y dimensions, and a complexity, the number of dead ends.
- Lairs will hold monsters. Deeper levels are tougher.

Do not build that generator, those locked exits, extra floors, or monsters until asked. The shipped lair is still the fixed plan below.

## Shipped: Iglin tunnels

One closed visit, apart from the open world. The captain goes alone. Companions wait on the grass in front of the mouth he used. His nameplate stays hidden inside. The same level as the open world loads again, with `SolidLevel=Lair` and `LairEnd` set to the Iglin Tunnels Entrance or the Iglin Tunnels Exit. There is no separate map.

The Iglin Tunnels Entrance is the `LairEntrance` cell south of Iglin. The Iglin Tunnels Exit is the `LairExit` cell east of Relion. Both are mountain-shelf cells, not towns, locations, or the cave. They let him pass the broken bridge. Do not move those cells when editing the interior grid.

Each mouth is a stone frame (`ASolidLairEntrance`) on its own cliff, with `SetLairEnd`. It is not the cave mouth. The entrance faces the first grassland neighbor, tried north, east, south, then west. The exit tries west first, then north, east, and south, so it faces west when that neighbor is grassland. The standing spot is `SolidStrongholds::FaceInsetCm` onto that grass. A click on the dark opening enters. A click on an opening inside leaves by that end. Enter does nothing until the lair is open. Leave stays available.

The lair opens from an `EnableLair` change that carries the Iglin tunnels id (`SolidLair::IdIglinTunnels`). The display name stays on the map line. Driven from the village carries that change. Opening the mouths is not a side effect of the event id having fired. That event is the normal return to Iglin after the monster message closes. It shows "You are driven from the village", titles him Outcast, and does not send him inside. F10 Enter cave does not make this trip, and it does nothing while he is already in a lair.

F10, Use portal, then **Iglin tunnels** (key 8, after the towns) is an early-test shortcut. It stands the squad 4 m north of the Iglin Tunnels Entrance (`SolidPortal::NorthOfEntranceCm`), facing the stone, and records Driven from the village even when Killed by monsters has not fired. That recording runs `EnableLair`, so the tunnels open. It does not enter the tunnels. It does not open the cave. A second use does not record the event again.

The interior is one floor, 11 by 9 cells, in `SolidLair::Maze`. `#` is rock. `.` is floor. The Iglin Tunnels Entrance and the Iglin Tunnels Exit are the mouths. Cell 200 cm, room height 320, shell 48, doorway 160, mouth height 240. The west side of the Iglin Tunnels Entrance cell is the door. The east side of the Iglin Tunnels Exit cell is the door. A rock lintel sits above each door. For early testing the first open row is a straight run from the Iglin Tunnels Entrance to the Iglin Tunnels Exit. The side passages stay. Leave that run in place until asked. Both mouths can be entered from the grass once the lair is open. The owner played the chain on this build: bridge, cave, driven from the village, into the tunnels, out and on to Relion, companion and raven sight, repaired bridge. The portal worked as well. The tunnels are dark. Each mouth has a dim light, and the middle has none. A torch in either of the captain's hands burns at him and lights the stone around him. A torch in the pack does not. His body does not cast a shadow in the tunnels, so that flame is not sealed inside him. Outside, the hidden body still casts a shadow.

Journal lines are `Enter Iglin tunnels` and `Exit Iglin tunnels`.

## Do not break these while extending lairs

- Tests in `SolidCore1.Lair.*` read the plan through `SolidLair` (`CellOf`, `ShortestPathSteps`, `EndSeparation`, `DeadEndCount`, `OpenCellCount`). They do not pin cell numbers, and they allow a path that is only as long as a straight walk. A new layout that still has both mouths, a joining path, dead ends, and floor off that path can ship without rewriting those tests. If a change would break an assumption, loosen the tests in their own push first, using a product function rather than a copied number. The layout change comes after and does not edit those tests.
- `USolidWorldChart::WorldXYToCell` is protected. Project a cell with `CellToWorldXY`.
- Never pass a `TObjectPtr` to `TestNotNull` / `TestNull`. Bind a raw `T*` or call `.Get()`. `GetStaticMesh()` returns a `TObjectPtr`.
- Unity builds merge anonymous namespaces across cpp files. Lair helpers stay in named namespaces: `SolidLairMouth`, `SolidLairInteriorPrivate`, `SolidLairTestPrivate`. Do not add `NeighborSteps`, `InBounds`, `YawFacingGrass`, `MountainGrid`, `SetCell`, `PlaceMouth`, or `AddRock` in an anonymous namespace.
- Comments in `SolidLair.h` describe a lair visit, not only this first maze.
