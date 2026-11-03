#!/usr/bin/env python3
"""Find and categorise cell references. No Unreal.

Scans Source/ (headers, sources, and notes) and tools/dump-chart.bat.
A hit is the whole word cell or cells, or a CamelCase segment such as CellX.
Each hit is World map cell, Hex cell, Lair cell, or Other.

Run: python3 tools/cell_census.py [repo] [groups.json] [rows.json]
Tests: python -m unittest tools/cell_census_test.py
"""
from __future__ import annotations

import json
import os
import re
import sys
from collections import Counter, defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
pat = re.compile(r"cell", re.I)
ident = re.compile(r"[A-Za-z0-9_]+")

def accept_span(text, start):
    m = None
    for im in ident.finditer(text):
        if im.start() <= start < im.end():
            m = im
            break
    if not m:
        return None
    word = m.group()
    rel = start - m.start()
    if word.lower() in ("cell", "cells"):
        return word
    ch = word[rel]
    if rel == 0:
        return word
    prev = word[rel-1]
    if ch.isupper() and (prev == "_" or prev.islower() or prev.isdigit()):
        return word
    return None

WORLD, HEX, LAIR, OTHER = "World map cell", "Hex cell", "Lair cell", "Other"

def has_any(low, words):
    return any(w in low for w in words)

def fallback(p, text, token, low, lineno):
    if "DungeonLevel" in p:
        return HEX, "Hex on the Meagre cave floor"
    if p.endswith("SolidAutoPlaythroughTests.cpp") and "one cell" in low:
        return LAIR, "Square on the lair floor plan"
    if "Playthrough/SolidAutoPlaythrough.cpp" in p:
        if "MazeCols" in text or "LairCells" in text or (759 <= lineno <= 920):
            return LAIR, "Square on the lair floor plan"
        return HEX, "Hex on a stronghold floor"
    if p.endswith("SolidAutoPlaythrough.h"):
        return HEX, "Hex on a stronghold floor"
    hex_markers = (
        "fourteen-cell", "fourteen cell", "catalogue row", "one cell each",
        "cell by the door", "cell nearest", "floor cell", "cells share",
        "keeps 11", "keeps 12", "every cell that fits", "pointy-top",
        "not on this floor", "feet on the cell", "cell coordinates",
        "entrance has a cell", "captain has a cell", "outside every cell",
        "entrance cell", "bed cells", "off the floor", "off every cell",
        "own cell", "meagrecells", "cell off the floor",
    )
    world_markers = (
        "->cell", "setstrongholdcell", "getstrongholdcell", "startingcell",
        "128 by 64", "taller cell", "shorter cell", "next cell", "cell edge",
        "cell border", "cell's terrain", "in the cell", "to this cell", "north of the cell",
        "remembers its cell", "keeps the cell", "leaves the cell", "new cell",
        "its cell", "on that site", "starting column", "starting row",
        "fintpoint", "acceptsenter",
    )
    if has_any(low, hex_markers):
        return HEX, "Hex on a stronghold floor"
    if has_any(low, world_markers):
        return WORLD, "Column and row on the 128 by 64 world chart"
    if "/Strongholds/" in p or "/Tests/Strongholds/" in p:
        if token == "Cell":
            return WORLD, "Map cell of a stronghold"
        return HEX, "Hex on a stronghold floor"
    if "/Player/" in p and token == "Cell":
        return WORLD, "Map cell chosen for the new stronghold"
    return "REVIEW", p

def classify(path, text, token, lineno):
    p = path.replace("\\", "/")
    low = text.lower()
    if token in ("PackCell", "ChestCell", "LastCell", "FirstEmptyPackCell"):
        return OTHER, "Inventory slot on the pack or a chest"
    if token in ("FogCells", "SlotForCell"):
        return OTHER, "Square on the fog curtain, aligned to the 2 m terrain points"
    if token in ("CellX", "CellY") and "SolidTerrainFog.cpp" in p:
        return OTHER, "Square on the fog curtain, aligned to the 2 m terrain points"
    if token == "EastCell" and "SolidArrangeSign.cpp" in p:
        return HEX, "Hex used to hang the Arrange sign on the cave wall"
    if token == "CellCm" and "ArrangeSign" in p:
        return OTHER, "2.4 cm square of the Arrange-sign letter grid"
    if token == "CellCm":
        return LAIR, "Width of one lair corridor square, 200 cm"
    if token in ("CellOf", "IsOpenCell", "LairCells", "RunCells", "OpenCells"):
        return LAIR, "Square on the lair floor plan"
    if token == "CellCenter":
        if "SolidLair" in text or "/Lairs/" in p:
            return LAIR, "Centre of a lair square"
        if "/Tower/" in p:
            return HEX, "Centre of a floor hex"
        return WORLD, "World position of a map-cell centre"
    if token == "CellCenterXY":
        return WORLD, "World position of a map-cell centre"
    if token == "DoorCell" and p.endswith("Strongholds/Tower/SolidTower.cpp"):
        return HEX, "Hex index left empty for the tower door"
    if token == "DoorCell":
        return WORLD, "Map cell stored on the tower shell"
    if token in (
        "MeagreEntranceCell", "MeagreCaptainCell", "MeagreCells", "FindCell", "SpanCell",
        "PieceOnCell", "ArrangeCells", "UsedCells", "CellKey", "CaptainCell", "bCaptainCell",
        "EntranceCell", "EntranceCells", "StandCells", "CellCount",
    ):
        return HEX, "Hex on a stronghold floor"
    if token in (
        "StartingCell", "StrongholdCell", "GetStrongholdCell", "SetStrongholdCell",
        "StrongholdChoiceCell", "GetStrongholdChoiceCell", "CellOfTerrainPoint",
        "ChooseLandingCell", "CellExtentAlongExitCm", "HalfCell", "CellW", "CellH",
        "GridCellPoint", "RoadCellPoint", "BridgeCellPoint", "CellCentre",
        "CellEastCm", "CellNorthCm", "HalfCellCm", "WorldMapCellWorld",
        "StrongholdTestCellWorld", "FromCell", "ToCell", "OriginCell", "ImperiousCell",
        "HumbleCell", "ImpressiveCell", "FortCell", "MovedCell", "SiteCell",
        "MapTowerCell", "RexCell", "RexCellTwo", "OpeningCell", "InCell", "GrassCell",
        "TowerCell", "AverageCell", "CellPoint", "CellGround", "CaveCell",
        "FSolidEventsNumberedCellTest", "NumberedCell", "CellPaint",
        "FSolidTerrainGridCellPaintTest", "cellX", "cellY",
    ):
        return WORLD, "Column and row on the 128 by 64 world chart"
    if token == "bCell":
        if "Dump" in p or "Commandlet" in p:
            return WORLD, "True when a mark sits on a map-cell centre"
        return HEX, "True when the arrange pointer is on a floor hex"
    if "/Inventory/" in p or "/Tests/Inventory/" in p:
        return OTHER, "Inventory slot on the pack, a chest, or a crafting grid"
    if ("pack cell" in low or "chest cell" in low or ("crafting" in low and "cell" in low)):
        if "furniture" not in low and "hex" not in low and "floor" not in low:
            return OTHER, "Inventory slot on the pack or a chest"
    if "empty item is an empty cell" in low:
        return OTHER, "Empty storage slot on a placed piece"
    if "marks stay off the cells" in low:
        return OTHER, "Inventory grid squares under the heading"
    if "ArrangeSign" in p and token == "CellCm":
        return OTHER, "2.4 cm square of the Arrange-sign letter grid"
    if has_any(low, (
        "solidstrongholdgrid", "fhex", "::fcell", "stair cell", "hole cell", "landing cell",
        "door cell", "anchor cell", "span cell", "furniture cell", "hex tile", "ground hex",
        "captain's cell", "captain cell", "neighboring cell", "kept cells", "cells that fit",
        "one-hex", "hex grid", "floor hex", "meagre floor", "fourteen cell",
    )) or ("hex" in low and "cell" in low):
        return HEX, "Hex on a stronghold floor"
    if token == "InCells" and "FCell" in text:
        return HEX, "Hex on a stronghold floor"
    if has_any(low, (
        "solidlair", "laircells", "tunnel cell", "lair's plan", "lair plan", "open cell",
        "maze", "corridor",
    )):
        return LAIR, "Square on the lair floor plan"
    if "/Lairs/" in p or "/Tests/Lairs/" in p:
        if "strongholdcell" in low or "map cell" in low:
            return WORLD, "Column and row on the 128 by 64 world chart"
        return LAIR, "Square on the lair floor plan"
    if "full-fog" in low:
        return OTHER, "Terrain points past the fog clear radius"
    if has_any(low, (
        "map cell", "cell centre", "cell center", "painted cell", "town cell", "forest cell",
        "river cell", "starting cell", "numbered cell", "z cell", "cellx", "celly",
        "chart cell", "north-west cell", "cell step", "cells marked", "world-map cell",
        "world map cell", "biome", "grassland cell",
    )):
        return WORLD, "Column and row on the 128 by 64 world chart"
    if "per-cell" in low or "fogged cell" in low or "lattice" in low or "volume cell" in low or "curtain cell" in low or "chunk edge" in low:
        return OTHER, "Fog or terrain-point grid, not the biome rectangle"
    if "total cells" in low or "terrainpoint cells" in low:
        return OTHER, "Terrain points called cells"
    if "cells and positions name the chunks" in low:
        return OTHER, "Comment about chunks to rebuild; the function takes positions"
    if "/Fog/" in p:
        if "that cell" in low or "z cell" in low:
            return WORLD, "The old spawn cell on the world chart"
        return OTHER, "Fog grid, not the biome rectangle"
    if "/Furnishings/" in p:
        return HEX, "Hex on a stronghold floor"
    if p.endswith("SolidBuildId.h"):
        return WORLD, "The chart's old biome grid"
    if "/Terrain/" in p or "/Tests/Terrain/" in p or "/Towns/" in p or "/Tests/Towns/" in p or "/Menus/" in p or "/Tests/Menus/" in p or "/Tests/Monster/" in p or "/Tests/Vegetation/" in p or "SolidWorldChart" in p or p.endswith("dump-chart.bat"):
        return WORLD, "Column and row on the 128 by 64 world chart"
    if "/Company/" in p or "/Tests/Events/" in p:
        return WORLD, "Column and row on the 128 by 64 world chart"
    if "/Player/" in p and "under him" in low:
        return HEX, "Floor hex under the captain while arrange mode hides him"
    if "/HUD/" in p or "/Game/" in p:
        if "FHex" in text or "FCell" in text:
            return HEX, "Hex on a stronghold floor"
        return WORLD, "Column and row on the 128 by 64 world chart"
    if "/Playthrough/" in p:
        if "FCell" in text or "Hex" in text:
            return HEX, "Hex on a stronghold floor"
        if "Lair" in text or "SolidLair" in text:
            return LAIR, "Square on the lair floor plan"
    if "/Strongholds/" in p or "/Tests/Strongholds/" in p:
        if has_any(low, ("acceptsenter", "another cell is not the gate", "north-south map cell")):
            return WORLD, "Column and row on the 128 by 64 world chart"
        if "SolidDungeon.h" in p and "to this cell" in low:
            return WORLD, "Column and row on the 128 by 64 world chart"
        if "FCell" in text or "FHex" in text:
            return HEX, "Hex on a stronghold floor"
        if "FIntPoint" in text or "map cell" in low or ".Cell" in text:
            return WORLD, "Column and row on the 128 by 64 world chart"
    if p.endswith(".md"):
        if "hex" in low or "fourteen-cell" in low:
            return HEX, "Hex on a stronghold floor"
        if "fog" in low or "lattice" in low:
            return OTHER, "Fog grid"
        return WORLD, "Column and row on the 128 by 64 world chart"
    return fallback(p, text, token, low, lineno)


def apply_deliverable_corrections(groups):
    """Post-classifier fixes from the same agent session (JSON + canvas patches)."""
    for g in groups:
        f, tok = g["file"], g["token"]
        if f.endswith("SolidTownSignTests.cpp") and tok in ("CellX", "CellY"):
            g["category"] = WORLD
            g["note"] = "Column and row on the 128 by 64 world chart"
        if f.endswith("SolidArrangeSign.cpp") and tok == "EastCell":
            g["category"] = HEX
            g["note"] = "Hex used to hang the Arrange sign on the cave wall"
        if f.endswith("SolidGameMode.cpp") and g.get("line") == 850 and tok == "Cells":
            g["category"] = HEX
            g["note"] = "Floor hexes collected for arrange"
        if f.endswith("SolidDungeon.h") and g.get("line") == 45 and tok == "cell":
            g["category"] = WORLD
            g["note"] = "Column and row on the 128 by 64 world chart"
        if f.endswith("SolidFortressInteriorTests.cpp") and g.get("line") == 111 and tok == "cell":
            g["category"] = WORLD
            g["note"] = "A different map cell is not this fortress gate"
        if f.endswith("SolidDungeonExteriorTests.cpp") and g.get("line") == 321 and tok == "cell":
            g["category"] = WORLD
            g["note"] = "North-south map cell is the taller one"


def scan_files(repo_root):
    files = []
    source = os.path.join(repo_root, "Source")
    if os.path.isdir(source):
        for dirpath, _, fnames in os.walk(source):
            for fn in fnames:
                if fn.endswith((".h", ".cpp", ".md")):
                    files.append(os.path.join(dirpath, fn))
    dump = os.path.join(repo_root, "tools/dump-chart.bat")
    if os.path.isfile(dump):
        files.append(dump)
    return files


def run_census(repo_root, apply_corrections=True):
    rows = []
    for path in scan_files(repo_root):
        rel = os.path.relpath(path, repo_root)
        with open(path, encoding="utf-8", errors="replace") as f:
            for i, line in enumerate(f, 1):
                seen = set()
                for m in pat.finditer(line):
                    tok = accept_span(line, m.start())
                    if not tok or tok in seen:
                        continue
                    seen.add(tok)
                    cat, note = classify(rel, line, tok, i)
                    rows.append(
                        {
                            "file": rel,
                            "line": i,
                            "token": tok,
                            "category": cat,
                            "note": note,
                            "text": line.strip()[:200],
                        }
                    )

    groups_map = {}
    for r in rows:
        key = (r["category"], r["file"], r["token"], r["note"])
        g = groups_map.get(key)
        if not g:
            groups_map[key] = {
                "category": r["category"],
                "file": r["file"],
                "token": r["token"],
                "note": r["note"],
                "count": 0,
                "line": r["line"],
                "sample": r["text"],
            }
            g = groups_map[key]
        g["count"] += 1
        g["line"] = min(g["line"], r["line"])

    groups = sorted(
        groups_map.values(),
        key=lambda g: (g["category"], g["file"], g["line"], g["token"]),
    )
    if apply_corrections:
        apply_deliverable_corrections(groups)

    folder = defaultdict(lambda: Counter())
    for r in rows:
        parts = r["file"].split("/")
        if "Tests" in parts:
            area = "Tests/" + parts[parts.index("Tests") + 1]
        elif parts[0] == "tools":
            area = "tools"
        elif "SolidCore1Editor" in parts:
            area = "Editor"
        elif len(parts) > 2:
            area = parts[2]
        else:
            area = parts[0]
        folder[r["category"]][area] += 1

    by_cat = Counter(r["category"] for r in rows)
    return rows, groups, by_cat, folder


def main():
    repo_root = ROOT
    out_groups = None
    out_rows = None
    if len(sys.argv) > 1:
        repo_root = sys.argv[1]
    if len(sys.argv) > 2:
        out_groups = sys.argv[2]
    if len(sys.argv) > 3:
        out_rows = sys.argv[3]

    rows, groups, by_cat, folder = run_census(repo_root)
    review = sum(1 for r in rows if r["category"] == "REVIEW")
    print("total", len(rows))
    print(dict(by_cat))
    print("REVIEW", review)
    print("groups", len(groups))

    payload = {
        "total": len(rows),
        "byCategory": dict(by_cat),
        "folders": {k: dict(v) for k, v in folder.items()},
        "groups": groups,
    }
    if out_groups:
        with open(out_groups, "w", encoding="utf-8") as f:
            json.dump(payload, f)
        print("wrote", out_groups)
    if out_rows:
        with open(out_rows, "w", encoding="utf-8") as f:
            json.dump(rows, f)
        print("wrote", out_rows)


if __name__ == "__main__":
    main()