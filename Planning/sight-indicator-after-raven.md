# Sight indicator after Raven Sight

Status: In Progress

## Story

The player looks for the sight indicator before raven sight is unlocked, and it is hidden. After raven sight unlocks, the player can see it. True sight still shows the eye, and raven sight still shows the eagle.

## Acceptance

- While raven sight is locked, the top-right indicator is hidden. A new game hides it. Enter Iglin, Reach broken bridge, Killed by monsters, and Driven from the village leave it hidden.
- Enter Relion shows it. Enter Kanfold shows it. Enter Dragongate shows it.
- Once raven sight is unlocked, the indicator stays visible in true sight and in raven sight. True sight shows the eye. Raven sight shows the eagle.
- The oval, its size, and its place stay as they are now.
- The character editor hides the indicator. Leaving the editor shows it when raven sight is unlocked, and leaves it hidden when raven sight is locked.
- F9 and the right-stick click still refuse raven sight while it is locked.
- A stronghold trip and a lair trip leave the indicator hidden when raven sight is still locked, and leave it visible when raven sight is unlocked.

## Implementation notes

- `DrawHUD` calls `EnsureSightIndicator` unless the character editor is open, so the oval is on screen from the start. Show it only when `ASolidGameMode::IsSightEnabled` is true for raven sight. That reads `FGameState::bRavenSightEnabled`.
- Add a pure helper, `SolidSight::ShowsIndicator`, that takes the raven-sight flag and whether the character editor is open. The editor hides it even when raven sight is unlocked. The product tests call that helper. They do not add a viewport widget and do not spawn PIE.
- The widget, the glyphs, and `SolidHudTheme::SightMarkerChip` stay. `SolidCore1.Sight.Marker` and `SolidCore1.HudTheme.SightMarker` stay. No current test asserts that the widget is added, so this story needs no prior test commit.
- The unlock events stay as they are. Nothing before Relion enables raven sight. Enter Relion, Enter Kanfold, and Enter Dragongate do. This story does not add an event.
- `FSolidTravelSession` copies `FGameState`, so `bRavenSightEnabled` survives a stronghold trip and a lair trip. The indicator follows that flag after the load.
- [Falcon Sight and Eagle Sight](falcon-and-eagle-sight.md) adds the marks for those two sights. This story does not.
