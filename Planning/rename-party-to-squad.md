# Rename Party to Squad

Status: Ready

## Story

A developer and a player both meet the word Squad where the game used to say Party. The captain fields a squad from the company. The F10 row that opened Build party now opens Build squad. Types, functions, folders, comments, tests, and the human docs use Squad for that fielded group. Company stays Company: it is still the full roster.

## Acceptance

- A search of `Source` for `Party` as the fielded group finds nothing. Identifiers, comments, and `TEXT()` labels say Squad. `Company` stays `Company`.
- The module folder is `Source/SolidCore1/Squad/`. `USolidParty` is `USolidSquad`. `SolidPartyBuilder` is `SolidSquadBuilder`. `SolidPartyCamera` is `SolidSquadCamera`. `SolidPartyDrill` is `SolidSquadDrill`. Include paths and the Build.cs module list match.
- Event and game-state names follow: `MaxPartySize` is `MaxSquadSize`, `SetMaxPartySize` is `SetMaxSquadSize`, `PartyCount` is `SquadCount`, `PartySize` on a change is `SquadSize`, and accessors such as `GetParty` / `GetMaxPartySize` match.
- Automation names follow: `SolidCore1.PartyBuilder.*` is `SolidCore1.SquadBuilder.*`, and any other `Party` suite prefix for this group is `Squad`. The suite lines in `README.md` match.
- The F10 menu shows **Build squad**. The builder page title and any other on-screen Party copy for this group say Squad.
- `README.md`, `AGENTS.md`, and open files under `Planning/` use Squad for the fielded group. Shipped files under `Planning/Done/` that describe that group use Squad too, including the former Captain + Party story title and path when they still say Party.
- Open stories that are themselves about the builder keep their intent. Their titles and wording say Squad (for example Build squad, squad builder) so a later agent does not reintroduce Party.
- Existing tests keep passing. Where a rename forces a test edit, the commit says so. Behavior is unchanged: who walks with the captain, caps from town events, and battle-plan assignment work as before.

## Implementation notes

- Company is not Party. Do not rename `USolidCompany`, the Company scrollbox on the builder, or "the company" in docs.
- Prefer one commit that renames code, tests, UI strings, and docs together so `main` never mixes the two words for the same thing.
- Unreal asset paths under `/Game/` are out of scope unless a Blueprint or input mapping literally embeds the word Party for this group. Enhanced Input and menu enums in C++ are in scope.
- `SOLID_BUILD_ID` bumps on the product push that ships the rename, as usual.
