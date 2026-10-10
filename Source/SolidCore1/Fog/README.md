# Exploration fog (fog-of-war)

Goal: the world starts shrouded; fog clears only where the **pawn has been** (trail), independent of camera orbit / pitch / zoom.

Code in this folder: `SolidTerrainFog.*`. Tests stay in `Source/SolidCore1/Tests/` (`SolidTerrainFogTests.cpp`, `SolidTerrainFogMeshTests.cpp`).

**Data (kept):** `FSolidTerrainPoint.Fog` on `USolidTerrainMap`. Initial fill and trail clear use the same bands — ≤25 m → `0`, 25–50 m → `0.5`, >50 m → `1`. The map starts fully fogged. `CenterExplorationFogOn` measures the initial distance from the **player** once they are placed. The player is moved to the WorldMap Z town, but fog is not keyed off that cell. Runtime: `ApplyExplorationFogAround` as the pawn moves.

**Current visual (SC1-0076):** marching-squares curtains at **clear|fogged (~25 m)** and **half|full (~50 m, white)**. Programmatic `BLEND_Translucent` unlit mist. Height fog **off**.

## Approaches tried and rejected

| Approach | Builds (approx.) | Why rejected |
|----------|------------------|--------------|
| **Exponential Height Fog driven by local pawn `Fog`** | ~SC1-0056…0061, **0068** | Global atmosphere. When the pawn stands on clear trail (`Fog=0`), density goes to 0 and **the whole world looks clear** — no distant fog-of-war. |
| **Height fog + camera / look-direction probes** | ~SC1-0065…0067 | `GetPlayerViewPoint` / forward probes made mist pop in/out when orbiting or looking at clear ground. UE’s fog **StartDistance is camera-relative**, so a “25 m clear bubble” moves with the boom, not the pawn. |
| **Height fog + omnidirectional ring around pawn** (no look probe) | SC1-0067 | Amount could stay stable, but StartDistance / height falloff still made the **look** of mist change with camera pose. Still not spatial FoW. |
| **Opaque solid fog volumes / roofs** (FlatCol boxes) | ~SC1-0057…0060 | Read as **solid grey/white slabs or snow** on the ground, not mist. |
| **Opaque wall-only / prism banks** (still FlatCol) | ~SC1-0063…0065 | Less “snow roof,” but still **solid walls**; spring-arm sometimes collided until fog meshes forced `NoCollision` / ignore `ECC_Camera`. |
| **Dense fog lattice** (fins/prisms in every fogged cell) | SC1-0069 | Many overlapping layers → looks fully opaque even with translucent mats; heavy overdraw / FPS. |
| **Axis-aligned clear\|fogged grid edges only** | SC1-0071…0072 | Translucent panels, but **stair-step corner gaps** let you see through the curtain. Superseded by marching squares (SC1-0073). |
| **JumpPad glow MIDs as “translucent” fog** | SC1-0069…0071 | Load successfully but ignore Opacity / read as **solid white emissive panels**. MID cannot change BlendMode. |
| **Half-fog via fake translucency on opaque mats** | mid series | Opacity parameters ignored on opaque parents → still fully opaque. |
| **Half-fog geometric dither** (checkerboard pillars, ~50% cells empty) | SC1-0063+ | Mitigated opacity for lattice cells; superseded by boundary curtains. |
| **Height fog “thicker mist” tuning only** (density / max opacity / extinction) | SC1-0058…0065 | Could look misty in places, but never fixed **spatial trail clearing** vs **camera dependence**. |

**Do not reintroduce** camera/view sampling for fog amount, height-fog StartDistance as a 25 m clear radius, or dense per-cell fog lattices. Prefer boundary geometry in `SolidTerrainFog` tied to `TerrainPoint.Fog`.
