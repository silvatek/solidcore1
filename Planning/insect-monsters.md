# Insect monsters

Status: Ready

## Story

The player meets two Tiny ants where the chart marks the Giant ants encounter. An ant is an insect monster. The body is the Stylized Ant from Fab, one downloaded mesh used for both. They stand and idle. Walking into one stops the captain. Coming into range starts a fight, and that fight is the battle a monster group already fights.

This story introduces the ant. Ant is `FSolidAnt`. The actor that shows one is `ASolidAnt`.

The catalog holds Giant ants. The World chart editor places the mark. Until that mark is saved on the chart, the world has no ants.

This story ships before or after [Encounter enhancements](encounter-enhancements.md). Shipped first, Giant ants is spawned at startup, with the other encounters. Shipped second, Giant ants depends on Driven from the village and is spawned when that event fires.

## Acceptance

- Ant is a monster kind. `EMonsterKind` includes Ant. A group member of that kind is an ant, and `Kind` is `Ant`.
- The catalog gains one encounter named Giant ants. It is one group of two Tiny ants. The plan is Attack. The near limit, the far limit, and the separation stay the definition defaults, 8 m, 40 m, and 4 m.
- Iglin Bridge stays three Small raptors on Attack. Its mark still places those raptors.
- A mark named Giant ants is the focus. The points are the ones an encounter mark already uses: grassland, walkable, clear of a mountain quad, inside the band, 4 m apart, clear of a tree and of a foraging pile, facing the mark, yaw only. Fewer than two eligible points places as many as there are. A missing mark places none.
- Encounter enhancements has not shipped. Giant ants names no event. The two ants are spawned at startup, when the open world chooses its monsters, with the other encounters. Saving the chart is what puts them in the world.
- Encounter enhancements has shipped. The row is an `FStaticEncounter` in `SolidEncounter::StaticEncounters` and depends on `SolidEvents::IdDrivenFromVillage` (`8f3c2a17`). Startup before that event places none. The ants are spawned when that event fires, the way that story places an encounter that depends on an event. A later visit, after the event has fired, spawns them at startup.
- The shipped chart stays as it is. The World chart editor lists Giant ants with the other encounter names, and a placed mark stores that name. This story does not write the mark.
- Each ant uses the Stylized Ant skeletal mesh. The same mesh is used twice. The listing's other two colour variations stay unused. The variation this encounter uses is recorded once the pack is added.
- A Tiny ant is about 40 cm long. The feet sit on the ground. The scale is uniform. Power is the Tiny size roll, the first step. The factor that makes the downloaded mesh that long is chosen in play, the way the raptor's factor was chosen beside the captain and the 1 m post. `FSolidRaptor::SmallDrawnBodyFactor` stays 6.
- Each one loops one in-place idle clip from that mesh. The other clips in the pack stay unused.
- The capsule fits the scaled body, and its radius and half-height are set with that scale. The click, the fog seeding, the empty nameplate, and the camera stay as they are for a raptor. Detection range starts at 5 m.
- The ant's weapon is Mandibles. The ant's armour is Carapace. Speed is the monster weapon's starting figure. Carapace Strength starts at the Hide figure, 20, until play-testing changes it.
- Coming into DetectionRange starts the battle. The log uses Ant and Mandibles, so a blow reads as an ant attacking with Mandibles. Walking into the body still stops the captain.
- Any Blueprint or AnimBP in the download stays unused. The captain stays on the Viking mesh.
- The 40 cm length is a uniform scale of the downloaded mesh. A later body builds an insect the way a human mesh is built. [`Content/Fauna.md`](../Content/Fauna.md) records this pack in the insect section, and it records that a Tiny ant is about 40 cm long and that the downloaded mesh is the body for now. Segments, wings, and the other colours stay that later body.
- The Credits page credits Anna Vorobeva for the Stylized Ant. The Fab listing is the Standard license, with a Personal tier and a Professional tier. The page names that creator, the line Fab Standard (Personal / Professional), and the listing URL, after the Velociraptor listing.

## Implementation notes

This story is one phase. The ant, the pack, and the catalog row land together. The chart mark is placed in the World chart editor after that. Shipped before Encounter enhancements, the ants spawn from the saved mark at startup. Shipped after it, the saved mark waits for Driven from the village. Spiders, centipedes, and the segmented body are later stories. A drawn length for a size other than Tiny is a later story.

- The listing is Stylized Ant by Anna Vorobeva, [https://www.fab.com/listings/da2e1374-ca5d-4f13-9245-72cebfb3bac6](https://www.fab.com/listings/da2e1374-ca5d-4f13-9245-72cebfb3bac6). The seller page is [https://www.fab.com/sellers/Anna%20Vorobeva](https://www.fab.com/sellers/Anna%20Vorobeva). It is a game-ready stylized insect, three colour variations, animated. The listing lists Attack, Death, Evasion, Idle, Jump, Run, Walk, Step aside, Stun, Damaged, and Enemy spotted, and calls the set 21 clips. Restore it with Launcher **Add to Project**. There is no download API. `tools/fab-assets.json` gains an `ant` entry with `required` true. `resolvedFolder` and `resolvedPaths` are written when the mesh and the idle are found. Look in the recorded folder first. A miss searches under Content for a skeletal mesh whose name contains `Ant` as its own word, and contains none of `Skeleton`, `Physics`, and `Morph`. `Plant` and `Phantom` do not match. A name that starts with `SK_` wins, and otherwise the shortest such name wins. The idle is the AnimSequence in that mesh's folder whose name contains `Idle` and contains none of `Attack`, `Death`, `Evasion`, `Jump`, `Run`, `Walk`, `Step`, `Stun`, `Damaged`, and `Enemy`. The shortest such name wins. One colour variation is applied to every slot. A find is written back. Do not move the folder.
- The Credits page names Anna Vorobeva after the Velociraptor listing, with the line Fab Standard (Personal / Professional) and the listing URL. The listing's license group is standard. Personal and Professional are both offered, each at no charge. The credit line is `Stylized Ant by Anna Vorobeva, used from Fab as an element of this project.` The `SolidCore1.Credits.*` line in the tests README names the ant.
- The README Fab table gains this listing, required, after the Velociraptor row.
- `FSolidAnt` is the kind, in its own header and cpp, the way `FSolidRaptor` is. `ASolidAnt` is the engine shell, the way `ASolidRaptor` is. `MakeMember` returns an ant for `EMonsterKind::Ant`, armed with Mandibles and Carapace. Append Ant on the enum. Raptor stays the first value. `DrawnBodyFactor` for a Tiny ant is the factor play settles. The raptor's Small factor stays on the raptor.
- Scale the mesh so the body is about 40 cm long. The length is the long axis of the LOD0 vertices, the same vertex measure a raptor uses for its height. The scale is uniform. The feet sit on the actor origin. Start from the uniform scale that makes that length 40 cm. Play may multiply it, the way the raptor's span scale of about 0.10 was multiplied by six before the body matched the captain and the 1 m post. The constant on the ant records the factor that looks right. The capsule fits that body. Its radius and half-height are chosen with the scale, and they stay inside the body. The raptor capsule stays 27 cm by a 50 cm half-height.
- Play the idle with `SolidClipLocomotion` on an `AnimSingleNodeInstance`, looping. Leave any AnimBP in the download unassigned.
- Encounter enhancements has not shipped. The encounter is the second `FDefinition` in `SolidEncounter::Definitions`. The name is `Giant ants`. The group is `EMonsterKind::Ant`, `ESolidMonsterSize::Tiny`, count 2, plan `Attack`. The limits stay the defaults on `FDefinition`. It names no event. The test that says the catalog has one encounter becomes two. Iglin Bridge stays the first row. Place uses the mark the way Iglin Bridge does. A test chart with a mark named Giant ants returns two Tiny ants at startup. A chart with no Giant ants mark returns none. The Iglin Bridge cases stay as they are. The shipped `/Game/World/WorldChart` asset stays as it is.
- Encounter enhancements has shipped. The row is an `FStaticEncounter` appended to `SolidEncounter::StaticEncounters` and calls `Require(SolidEvents::IdDrivenFromVillage)`. That constant is the event `8f3c2a17`. `SolidMessages::IdDrivenFromVillage` is the message `4b91d0e6`, and the dependency names the event. Iglin Bridge stays the first row and still spawns at startup. A test chart with a Giant ants mark places nobody before the event has fired, and places two Tiny ants once it has. The shipped `/Game/World/WorldChart` asset stays as it is.
- The content test that loads the Velociraptor also loads this body and this idle once the pack is added. Until Add to Project, that load fails the same way a missing Velociraptor mesh fails.
- The product commit updates the `SolidCore1.Monster.*`, `SolidCore1.Credits.*`, and `SolidCore1.Content.*` lines in `Source/SolidCore1/Tests/README.md`.
