# Battle plans (SC1-0095 / SC1-0101)

Status: Done

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
