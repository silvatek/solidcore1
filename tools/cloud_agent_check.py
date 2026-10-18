#!/usr/bin/env python3
"""SolidCore1 cloud-agent checks — no Unreal required.

Mirrors USolidWorldMap grid loading and a few agent workflow guards.
Exit 1 on errors; warnings print but do not fail.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAP_WIDTH = 128
MAP_HEIGHT = 64

# Matches USolidWorldMap::IsGridMarker (letters, digits, biome/punctuation markers).
SPECIAL_MARKERS = set("~.o#*^!")


def is_grid_marker(ch: str) -> bool:
    if len(ch) != 1:
        return False
    if ch.isalpha() or ch.isdigit():
        return True
    return ch in SPECIAL_MARKERS


def load_world_map_grid(text: str) -> list[str]:
    grid: list[str] = []
    for raw in text.splitlines():
        line = raw.strip()
        if len(line) != MAP_WIDTH:
            continue
        if all(is_grid_marker(ch) for ch in line):
            grid.append(line)
            if len(grid) == MAP_HEIGHT:
                break
    return grid


def check_world_map(errors: list[str], warnings: list[str]) -> None:
    path = ROOT / "Content" / "WorldMap.txt"
    if not path.is_file():
        errors.append(f"missing {path.relative_to(ROOT)}")
        return

    text = path.read_text(encoding="utf-8")
    grid = load_world_map_grid(text)
    if len(grid) != MAP_HEIGHT:
        errors.append(
            f"WorldMap grid is {len(grid)}x{MAP_WIDTH if grid else 0}, "
            f"expected {MAP_HEIGHT}x{MAP_WIDTH}"
        )
        return

    bang_cells: set[tuple[int, int]] = set()
    for row_i, row in enumerate(grid, start=1):
        for col_i, ch in enumerate(row, start=1):
            if ch == "!":
                bang_cells.add((row_i, col_i))

    described: set[tuple[int, int]] = set()
    for raw in text.splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line.startswith("!"):
            continue
        match = re.match(r"!\s*(\d+)\s*,\s*(\d+)\s*=", line)
        if match:
            described.add((int(match.group(1)), int(match.group(2))))

    missing = sorted(bang_cells - described)
    if missing:
        cells = ", ".join(f"{r},{c}" for r, c in missing)
        warnings.append(
            f"WorldMap ! cells missing description lines (row,column from 1,1): {cells}"
        )


def check_build_id(errors: list[str]) -> None:
    path = ROOT / "Source" / "SolidCore1" / "Game" / "SolidBuildId.h"
    if not path.is_file():
        errors.append(f"missing {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    id_match = re.search(
        r'#define\s+SOLID_BUILD_ID\s+TEXT\("SC1-(\d{4})"\)', text
    )
    note_match = re.search(
        r'#define\s+SOLID_BUILD_NOTE\s+TEXT\("([^"]*)"\)', text
    )
    if not id_match:
        errors.append("SOLID_BUILD_ID must be TEXT(\"SC1-NNNN\") in SolidBuildId.h")
    if not note_match or not note_match.group(1).strip():
        errors.append("SOLID_BUILD_NOTE must be a non-empty TEXT(\"...\") in SolidBuildId.h")


def check_planning(errors: list[str], warnings: list[str]) -> None:
    planning = ROOT / "Planning"
    readme = ROOT / "README.md"
    if not planning.is_dir():
        errors.append("missing Planning/")
        return
    if not readme.is_file():
        errors.append("missing README.md")
        return

    on_disk = {p.name for p in planning.glob("*.md") if p.name != "README.md"}
    todo_section = readme.read_text(encoding="utf-8").split("## Done", 1)[0]
    listed = set(re.findall(r"Planning/([\w-]+\.md)", todo_section))

    unlisted = sorted(on_disk - listed)
    missing = sorted(listed - on_disk)
    if unlisted:
        errors.append(
            "Planning stories not listed in README.md To-do: " + ", ".join(unlisted)
        )
    if missing:
        errors.append(
            "README.md To-do links missing files in Planning/: " + ", ".join(missing)
        )

    # Soft check: Done/ files should not also sit in Planning/.
    done = ROOT / "Planning" / "Done"
    if done.is_dir():
        dupes = sorted({p.name for p in done.glob("*.md")} & on_disk)
        if dupes:
            warnings.append(
                "stories present in both Planning/ and Planning/Done/: "
                + ", ".join(dupes)
            )


def check_forbidden_patterns(errors: list[str]) -> None:
    source = ROOT / "Source"
    if not source.is_dir():
        errors.append("missing Source/")
        return

    material_hits: list[str] = []
    tobject_hits: list[str] = []
    test_call = re.compile(r"\bTest(?:Not)?Null\s*\(")

    for path in source.rglob("*"):
        if path.suffix.lower() not in {".cpp", ".h", ".hpp", ".c", ".cc"}:
            continue
        rel = path.relative_to(ROOT).as_posix()
        try:
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError as exc:
            errors.append(f"cannot read {rel}: {exc}")
            continue

        for i, line in enumerate(lines, start=1):
            code = line.split("//", 1)[0]
            if "MaterialEditingLibrary" in code:
                material_hits.append(f"{rel}:{i}")
            # Bare GetStaticMesh() yields TObjectPtr; .Get() is fine.
            if test_call.search(code) and re.search(
                r"GetStaticMesh\(\)(?!\s*\.Get\s*\()", code
            ):
                tobject_hits.append(f"{rel}:{i}")

    if material_hits:
        errors.append(
            "MaterialEditingLibrary is forbidden (SC1-0042). Found in: "
            + ", ".join(material_hits[:8])
        )
    if tobject_hits:
        errors.append(
            "Do not pass GetStaticMesh() (TObjectPtr) to TestNotNull/TestNull; "
            "bind a raw T* or call .Get(). Found in: "
            + ", ".join(tobject_hits[:8])
        )


def main() -> int:
    errors: list[str] = []
    warnings: list[str] = []

    check_world_map(errors, warnings)
    check_build_id(errors)
    check_planning(errors, warnings)
    check_forbidden_patterns(errors)

    for message in warnings:
        print(f"WARNING: {message}")
    for message in errors:
        print(f"ERROR: {message}")

    if errors:
        print(f"cloud_agent_check: {len(errors)} error(s), {len(warnings)} warning(s)")
        return 1

    print(f"cloud_agent_check: ok ({len(warnings)} warning(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
