# Basic monsters

Status: Ready

## Story

The player walks from Iglin toward the broken bridge and meets three reptiles on the grass beside that bridge. Each one is the same green Velociraptor from a Fab download. They stand and idle. The player can walk into one and has to stop. A click does nothing to them. The ramp still shows "You are killed by monsters" when the captain stands on it. These three do not fire that message.

## Acceptance

- Three monsters stand on the open world, on the Iglin side of the Iglin–Relion bridge.
- Each one uses the green Velociraptor skeletal mesh from `/Game/Fab/PBR_Velociraptor__Animated_`. The same mesh is used three times. The other four skins stay unused.
- The standing height is 220 cm. The feet sit on the ground.
- Each one loops that mesh's in-place idle clip. They stay in that pose.
- They face the outer end of the Iglin ramp. Yaw only.
- A monster's terrain point is Grassland, and it is walkable.
- The point is between 8 m and 40 m from the outer end of the Iglin ramp. The outer end is the ramp sample farthest from the deck. The Iglin ramp is the ramp whose outer end is closer to the Iglin town cell.
- The three points are the nearest eligible points to that outer end. A later point is at least 4 m from one already chosen. Fewer than three eligible points places as many as there are.
- A point that has a tree has none. A point already chosen for a foraging pile has none.
- A point whose `Fog` is `1.0f` is not seeded. When that `Fog` falls below `1.0f`, a chosen monster on it is seeded then. A point that stays at `1.0f` never receives one.
- The dungeon, the tower, and the Iglin tunnels have none. Leaving one of those and returning to the open world shows the same three places again.
- The capsule blocks the captain and a companion. It ignores the visibility channel, so a ground click still hits the ground.
- Walking into the body stops the captain. The captain, a companion, and the monster all stay unhurt.
- A click on a monster leaves the fight unstarted, leaves every item where it is, and fires no event.
- The pointer shows no name for a monster. A monster has no nameplate.
- The squad camera stays on the captain and the companions.
- Standing beside a monster leaves Reach broken bridge unfired. The ramp still fires it, including after the bridge is repaired. Repairing the bridge leaves the three where they are.
- Seeing them writes no journal line.
- Any Blueprint or AnimBP in the download stays unused. The captain stays on the Viking mesh.
- Insects stay a later body. `Content/Lairs.md` still leaves the tunnels empty of monsters.
- The 220 cm height is a uniform scale of the downloaded mesh. A later body builds a Velociraptor the way a human mesh is built, and the way the Character Editor changes one. [`Content/Fauna.md`](../Content/Fauna.md) records that. This story places the downloaded mesh.

## Implementation notes

- The listing is PBR Velociraptor (Animated) by Ferocious Industries, [https://www.fab.com/listings/5d5fa3d3-4459-4677-be46-c686f733908c](https://www.fab.com/listings/5d5fa3d3-4459-4677-be46-c686f733908c). It is the green skin, about 15144 triangles, with albedo, normal, ambient occlusion, and a secondary 2K texture. Restore it with Launcher **Add to Project**. There is no download API. Add to Project puts the pack at `/Game/Fab/PBR_Velociraptor__Animated_`, which is `Content/Fab/PBR_Velociraptor__Animated_`. `tools/fab-assets.json` gains a `velociraptor` entry with `required` true and that path as `resolvedFolder`. The body and the idle are recorded in `resolvedPaths` the way Viking records `SK_Viking` and `Anim_Viking_idle1`. Look in that folder first. A miss searches under Content for a skeletal mesh whose name contains `Velociraptor` and contains none of `Skeleton`, `Physics`, and `Morph`. A name that starts with `SK_` wins, and otherwise the shortest such name wins. The green material is the one whose name contains `Green`. The idle is the AnimSequence in that mesh's folder whose name contains `Idle` and contains none of `Root`, `Attack`, `Death`, `Hit`, `Roar`, `Walk`, and `Run`. The shortest such name wins. A find is written back. Do not move the folder.
- Credits gain a Velociraptor block after the grass listing: creator Ferocious Industries, the Fab Standard line, and that listing URL. The `SolidCore1.Credits.*` line in `README.md` gains it in the same commit.
- Scale the mesh so its bounds height is 220 cm. The scale is uniform. The capsule half-height is half of that, and the radius is 60 cm. The actor origin is the feet. That scale is not the human-body deform. `SolidBodyMesh` and the Character Editor stay on the Viking skeleton.
- Play the idle with `SolidClipLocomotion` on an `AnimSingleNodeInstance`, looping. Leave any AnimBP in the download unassigned.
- Choose the points in a function tests can call without a world. The Iglin town center comes from the world map. The ramp polylines come from the `SolidBridge::FBridge` whose `bIglinRelion` is set. Distance is horizontal. Sort by distance to the outer end, then by point index. Separation is 400 cm. The near limit is 800 cm. The far limit is 4000 cm. The count is 3.
- Run that choice after the tree choice and the foraging choice, and skip a point index either of those already took. Seed when `Fog` falls below `1.0f`, the same moment as a foraging pile. Entering a stronghold or the tunnels stores no taken index for a monster. The next open-world visit chooses the same points and seeds the ones whose fog is already clear.
- Collision blocks Pawn and ignores Visibility. There is no damage function and no health. Click-to-move stays as it is, because the trace still hits the ground.
- Tests live in `SolidCore1.Monster.*`. A fake point set with one ramp returns the three nearest grassland points inside the band. It skips a road point, a river point, a tree point, a foraging point, a point inside 8 m, and a point past 40 m, and it keeps 4 m between the three. A map with no Iglin bridge returns none. The README suite table gains that line in the product commit.
- The content test that loads Viking also loads this body and this idle once the pack is added. Until Add to Project, that load fails the same way a missing Viking mesh fails.
