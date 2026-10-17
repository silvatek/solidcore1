# Content pipeline

Fab listings restored via Launcher Add to Project. Recorded paths in `tools/fab-assets.json` are tried first. A miss falls back to a search: Viking and grass by name under `/Game/Viking` or `/Game/Fab`, companion bodies `Meshes/base_mesh/SKM_base_mesh_ue` inside `Medieval_Assassin_Girl` and `Mesh/SK_Elf_Blacksmith` inside `Elf_Blacksmith`, anywhere under Content. A find is written back into the manifest. `tools/fab_doctor.bat`. Packs are gitignored (attribution README kept).
