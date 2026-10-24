# Dracorex Encounter

Status: New

## Story

The player meets one Immense raptor where the chart marks the Dracorex encounter. It is the same green Velociraptor as the three Small raptors at Iglin Bridge, drawn at the Immense size. It stands and idles. Walking into it stops the captain. Coming into range starts a fight, and that fight is the battle a monster group already fights.

The developer places that mark in the World chart mode, the mode the World Editor opens. The Place list offers Dracorex with the other encounter names. Until that mark is saved on the chart, the world has no Dracorex.

This story introduces no game concept. Raptor is already `FSolidRaptor`. Encounter is already `FSolidEncounter`. The marker is a mark on the chart.

This is a single phase. The catalog row and the Immense body land together. The chart mark is placed in the World chart mode after that.

## Acceptance

- The catalog gains one encounter named Dracorex. It is one group of one Immense raptor. The plan is Attack. The near limit, the far limit, and the separation stay the definition defaults, 8 m, 40 m, and 4 m.
- Iglin Bridge stays three Small raptors on Attack. Its mark still places those raptors. A catalog row another story has already added stays.
- A mark named Dracorex is the focus. The point is the one an encounter mark already uses: grassland, walkable, clear of a mountain quad, inside the band, clear of a tree and of a foraging pile, facing the mark, yaw only. One member needs one point. A missing mark places none. Two marks named Dracorex are two of that encounter.
- Dracorex names no event. The raptor is spawned at startup, when the open world chooses its monsters, with the other encounters that are ready then. Saving the chart is what puts it in the world.
- The shipped chart stays as it is. The World chart mode lists Dracorex with the other encounter names. A placed mark stores that name. This story does not write the mark.
- The Immense raptor uses the same green Velociraptor mesh and the same idle as a Small raptor. The feet sit on the ground. The scale is uniform.
- Small stays 100 cm, at `FSolidRaptor::SmallDrawnBodyFactor` of 6. An Immense raptor stands about 8 m, eight times that body. The factor that makes the downloaded mesh that tall is chosen in play, the way the Small factor was chosen beside the captain and the 1 m post. The sizes between Small and Immense still draw as Small.
- The capsule fits the scaled body. Its radius and half-height keep the Small proportions, multiplied by that same eight. The Small capsule stays 27 cm by a 50 cm half-height. The click, the fog seeding, the empty nameplate, and the camera stay as they are for a Small raptor. Detection range starts at 5 m.
- The weapon is Claws. The armour is Hide. Power is the Immense size roll. Hide Strength stays 20. Speed is the monster weapon's starting figure.
- Coming into DetectionRange starts the battle. The log uses Raptor and Claws. Walking into the body still stops the captain.
- The product commit bumps `SOLID_BUILD_ID` and `SOLID_BUILD_NOTE`.

## Implementation notes

This story is one phase. The catalog row and the Immense drawn body land together. The developer places the mark afterwards in the World chart mode. `tools\editor.bat` opens that mode. [Encounter enhancements](encounter-enhancements.md) may ship before or after this story. [Insect monsters](insect-monsters.md) may too. Neither is a dependency.

- The encounter name is `Dracorex`. `FindByName` ignores case, so `dracorex` is that same row. Append it. Iglin Bridge stays the first row.
- Encounter enhancements has not shipped. The row is an `FDefinition` in `SolidEncounter::Definitions`. The group is `EMonsterKind::Raptor`, `ESolidMonsterSize::Immense`, count 1, plan `Attack`. The limits stay the defaults on `FDefinition`. It names no event.
- Encounter enhancements has shipped. The row is an `FStaticEncounter` appended to `SolidEncounter::StaticEncounters`. It does not call `Require`. Startup places it with Iglin Bridge. The Place list still reads the catalog, under whichever type name that story left.
- Insect monsters may already have appended Giant ants. Leave that row. Dracorex follows whatever rows are already there. Iglin Bridge stays first.
- `MakeMonster` already sets `Size` from the group and arms Claws and Hide. `PowerFor` of Immense is the power. `FSolidRaptor::Arm` still sets Hide Strength to 20. The member's kind stays Raptor. The actor stays `ASolidRaptor`.
- `FSolidRaptor::DrawnBodyFactor` returns `SmallDrawnBodyFactor` for Small and `ImmenseDrawnBodyFactor` for Immense. The other sizes stay on the Small factor, which is what the default branch does today. `ImmenseDrawnBodyFactor` starts at 48, eight times 6, so the drawn body is about 8 m. Play beside a Small raptor and the 1 m post can change that constant. `SmallDrawnBodyFactor` stays 6.
- `ASolidRaptor` builds the capsule at the Small size. `SetRaptor` resizes it when the raptor is Immense: radius and half-height times `ImmenseDrawnBodyFactor / SmallDrawnBodyFactor`, and the capsule centre stays at the half-height so the bottom is the actor origin. A Small raptor keeps 27 cm and a 50 cm half-height. The default actor, before `SetRaptor`, keeps those Small figures. The body mesh scales through `BodyScale` as it does now.
- The World chart mode already fills the Encounter combo from the catalog in `FSolidWorldChartEdMode`. A new row appears there. `PlaceEncounter` already stores a catalog name and refuses a name the catalog lacks. The shipped `/Game/World/WorldChart` asset stays as it is.
- The test that says the catalog has one encounter counts the rows present when this story ships, and expects Dracorex among them: one group, one Immense raptor, Attack, the default limits. Iglin Bridge stays three Small raptors. A test chart with a mark named Dracorex places one Immense raptor at startup. A chart with no Dracorex mark places none of that encounter. `PlaceEncounter` accepts Dracorex and still rejects an unknown name. An Immense raptor's drawn factor is the new constant, and a Small raptor's factor stays 6. The body test still sees the default capsule at 27 cm by a 50 cm half-height, and an Immense `SetRaptor` grows that capsule. The product commit updates the `SolidCore1.Monster.*` line in `Source/SolidCore1/Tests/README.md`. The tests run without a viewport. A cloud agent does not run the editor or the suite. The Windows machine places the mark in the World chart mode after the row exists.
